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
    
    struct semaphore worker_sema;

    struct semaphore exit_sema;

    struct semaphore to_process_sema[4];
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
    
    sema_init(&p->exit_sema, 0);

    p->workers = malloc(sizeof(struct worker)*num_workers);
    p->num_workers = num_workers;

    for (int i = 0; i < num_workers; i++) {
        p->workers[i].id = i;
        p->workers[i].to_process = EMPTY;
        p->workers[i].owner = p;
        p->workers[i].ready = false;
        sema_init(&p->worker_sema, 0);
        lock_init(&p->workers[i].to_process_lock);

        sema_init(&p->to_process_sema[i], 0);

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

    while (!exit) 
    {
        sema_up(&w->owner->worker_sema);
        w->ready = true;

        sema_down(&w->owner->to_process_sema[w->id]);
        lock_acquire(&w->to_process_lock);
        const char *request = w->to_process;

        // Ska vi avsluta?
        if (request == EMPTY) 
        {
            exit = true;
        } 
        else 
        {
            process_request(w->id, request);
            // Meddela att vi är klara.
            w->to_process = NULL;
            lock_release(&w->to_process_lock);
        }
    }
    sema_up(&w->owner->exit_sema);
    lock_release(&w->to_process_lock);
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

    // for (int i = 0; i < 4; ++i)
    // {
    //     sema_down(&pool->worker_sema[i]);
    //     id = i;
    // }
    // struct worker *worker = &pool->workers[id];

    // // Är tråden ledig?
    // lock_acquire(&worker->to_process_lock);
    // if (worker->to_process == NULL) {
    //     // Ja, be den att hantera frågan.
    //     worker->to_process = request;
    //     sema_up(&pool->to_process_sema[i]);
    //     lock_release(&worker->to_process_lock);
    //     return;
    // }
    // lock_release(&worker->to_process_lock);

    sema_down(&pool->worker_sema);
    for (int i = 0; i < 4; ++i)
    {
        if (pool->workers[i].ready)
        {
            struct worker *worker = &pool->workers[i];
            
            // Är tråden ledig?
            lock_acquire(&worker->to_process_lock);
            if (worker->to_process == NULL) {
                // Ja, be den att hantera frågan.
                worker->to_process = request;
                sema_up(&pool->to_process_sema[i]);
                lock_release(&worker->to_process_lock);
                return;
            }
            lock_release(&worker->to_process_lock);
        }
    } 

    // for (int i = 0; i < 4; ++i)
    // {
    //     struct worker *worker = &pool->workers[i];

    //     // Är tråden ledig?
    //     lock_acquire(&worker->to_process_lock);
    //     if (worker->to_process == NULL) {
    //         // Ja, be den att hantera frågan.
    //         worker->to_process = request;
    //         sema_up(&pool->to_process_sema[i]);
    //         lock_release(&worker->to_process_lock);
    //         return;
    //     }
    //     lock_release(&worker->to_process_lock);
    // }
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

void pool_destroy(struct thread_pool *pool) {
    // Be trådarna att stänga av sig, genom att skicka den speciella strängen
    // EMPTY till alla trådar.
    
    printf("här\n");
    for (int i = 0; i < pool->num_workers; i++)
        pool_handle_request(pool, EMPTY);

    
    for (int i = 0; i < 4; ++i)
    {
        sema_down(&pool->exit_sema);
    }
    free(pool->workers);
    free(pool);
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
    // Skicka massvis med frågor.
    for (int i = 0; i < 10; i++) {
        for (int j = 0; requests[j]; j++) {
            pool_handle_request(pool, requests[j]);
        }
    }
    pool_destroy(pool);

    return 0;
}