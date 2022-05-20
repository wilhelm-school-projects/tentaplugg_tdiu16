#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "wrap/atomics.h"
#include "wrap/synch.h"
#include "wrap/thread.h"
#include "wrap/timer.h"

// Diverse deklarationer som behövs nedan.
struct worker;
static void worker_main(void *data);

/**
 * Representerar en trådpool. Alltså en eller flera trådar som hanterar frågor.
 */
struct thread_pool {
    // Antal trådar.
    int num_workers;

    // Array av worker-trådar.
    struct worker *workers;

    struct semaphore avail_worker_sema;
};


/**
 * Representerar en tråd i servern.
 */
struct worker {
    // Trådens ID i servern (från 0 och uppåt).
    int id;

    // Fråga som tråden ska hantera, eller håller på att hantera för närvarande.
    // Om "to_process" sätts till en tom sträng ska tråden avslutas.
    const char *to_process;
    struct lock to_process_lock;

    bool ready;

    struct semaphore request_sema;
    // Pekare till den "thread_pool" som tråden hör till.
    struct thread_pool *owner;
};

// Tom sträng vi använder för att signalera att en tråd ska avslutas.
const char *const EMPTY = "";

/**
 * Funktion som anropas för att hantera en fråga. Vi antar att det är okej att
 * köra den här funktionen från flera trådar samtidigt. Det är här all logik för
 * vår hemsida kommer implementeras senare.
 */
void process_request(int id, const char *request) {
    printf("Request %s handled by thread %d\n", request, id);
    // Simulera att det tar lite tid.
    timer_msleep(100);
}


/**
 * Skapa en trådpool.
 *
 * Skapar också ett antal trådar som hanterar de frågor som skickas till
 * trådpoolen.
 */
struct thread_pool *pool_create(int num_workers) {
    struct thread_pool *p = malloc(sizeof(struct thread_pool));

    p->workers = malloc(sizeof(struct worker)*num_workers);
    p->num_workers = num_workers;
    sema_init(&p->avail_worker_sema, 0);
    for (int i = 0; i < num_workers; i++) {
        p->workers[i].id = i;
        p->workers[i].to_process = NULL;
        p->workers[i].owner = p;
        p->workers[i].ready = false;
        sema_init(&p->workers[i].request_sema, 0);
        lock_init(&p->workers[i].to_process_lock);

        thread_new(&worker_main, &p->workers[i]);
    }
    return p;
}


/**
 * Funktion som körs för varje tråd. Tråden väntar på att det ska komma en
 * fråga, och hanterar sedan frågan. Tråden körs till någon skickar den
 * speciella strängen EMPTY till tråden. EMPTY tas inte bort ur "to_process" för
 * att "pool_destroy" ska fungera korrekt.
 */
static void worker_main(void *data) {
    struct worker *w = data;
    bool exit = false;
    

    //  Meddela att "jag" är redo för arbete
    //  Vänta på att få en request
    //  Gör request
    //  repeat 
    //  Om request som fås är EMPTY avslutas loop

    while (!exit) {
        
        w->ready = true;                            // Redo att ta arbete
        sema_up(&w->owner->avail_worker_sema);      // Signalera att en arbetare är tillgänglig
        sema_down(&w->request_sema);

        w->ready = false;                           // Har fått arbete
        
        lock_acquire(&w->to_process_lock);
        const char *request = w->to_process;
        lock_release(&w->to_process_lock);

        // Ska vi avsluta?
        if (request == EMPTY) {
            exit = true;
        } else {
            process_request(w->id, request);
        }
    }
}


/**
 * Hantera en fråga.
 *
 * Hittar en tråd som inte har något att göra för tillfället och ger frågan till
 * den tråden. Funktionen väntar inte på att frågan har hanterats färdigt.
 *
 * Om det inte finns någon tråd som kan hantera frågan just nu, väntar
 * implementationen tills en är ledig.
 */
void pool_handle_request(struct thread_pool *pool, const char *request) {
    int id = 0;

    //  1. Konstantera att det finns minst en arbetare tillgänglig 
    //  2. Hitta en ledig worker
    //  3. Ge den en request

    sema_down(&pool->avail_worker_sema);
    for (int i = 0; i < 4; ++i)
    {
        if (pool->workers[i].ready)                             // Hitta redo arbeteare
        {   
            lock_acquire(&pool->workers[i].to_process_lock);    // Ta kontroll över to_process
            sema_up(&pool->workers[i].request_sema);            // Meddela att det finns arbete 
            id = i;
            break;
        }
    }

    struct worker *worker = &pool->workers[id];

    // Ja, be den att hantera frågan.
    worker->to_process = request;
    lock_release(&worker->to_process_lock);
}


/**
 * Förstör en trådpool.
 *
 * Vi antar att ingen annan tråd försöker anropa (eller håller på att köra)
 * "pool_handle_request" på ett objekt som håller på att förstöras, eller har
 * förstörts.
 *
 * Implementationen ska garantera att alla frågor som tidigare har skickats till
 * trådpoolen med "pool_handle_request" ska slutföras. Om någon fortfarande är
 * på gång ska implementationen vänta på att alla blir klara.
 */
struct semaphore program_sema;

void pool_destroy(struct thread_pool *pool) {
    // Be trådarna att stänga av sig, genom att skicka den speciella strängen
    // EMPTY till alla trådar.
    for (int i = 0; i < pool->num_workers; i++)
        pool_handle_request(pool, EMPTY);

    free(pool->workers);
    free(pool);
    sema_up(&program_sema);
}


/**
 * Huvudprogram.
 *
 * Eventuella ändringar här nedanför kommer att ignoreras vid rättning av
 * uppgiften. Du får däremot ändra något nedanför ifall du vill testa din
 * lösning.
 */

const char *requests[] = {
    "/get_exam?course=TDIU16",
    "/login?user=kim&password=secret",
    "/logout",
    "/index.html",
    "/submit.php?question=42",
};


int main(void) {
    struct thread_pool *pool = pool_create(4);
    sema_init(&program_sema, 0);
    // Skicka massvis med frågor.
    for (int i = 0; i < 10; i++) {
        for (int j = 0; requests[j]; j++) {
            pool_handle_request(pool, requests[j]);
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        sema_down(&program_sema);
    }
    pool_destroy(pool);

    return 0;
}
