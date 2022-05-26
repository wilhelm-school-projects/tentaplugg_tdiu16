/**

Svar på delfråga a:
	Busy-wait är när en tråd väntar på att få göra något genom att loopa
	i en loop tills denna tillåts att göra detta. Varför detta är dåligt är 
	eftersom den måste ligga och köras i processorn trots att den inte gör 
	något vettigt. Det bör just därför undvikas och kan göra med t.ex. lås
	eller semaforer som alltså tillåter tråden att bli preemptad från processorn
	och därför låta andra trådar köra.   

Svar på delfråga b:
	Se kommentarer i koden.

Svar på delfråga c:
	Se semafor i koden.

Svar på delfråga d:
	I övrigt är koden trådsäker eftersom det enda stället som delar data mellan trådar nu är tråd-
	säkert med hjälp av semaforer.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wrap/atomics.h"
#include "wrap/synch.h"
#include "wrap/thread.h"

/**
 * Ett future-objekt.
 *
 * Representerar ett värde som håller på att beräknas någonstans. Detta objekt
 * har två operationer: "future_set" och "future_get". Funktionen "future_get"
 * försöker hämta det värde som finns lagrat i future-objektet. Om det inte
 * finns ska den vänta på att en annan tråd anropar "future_set" för att lagra
 * värdet. "future_get" kan anropas flera gånger.
 *
 * Motsvarande objekt finns i C++ (std::future), men den fungerar lite
 * annorlunda och är lite mer komplicerad.
 */
struct future {
	// Värdet som lagras inuti future-objektet.
	int value;

	struct semaphore sema; 

	// Är värdet satt ännu?
	bool has_value;
};

// Initiera ett future-objekt.
void future_init(struct future *f) {
	f->has_value = false;
	sema_init(&f->sema, 0);
}

// Sätt värdet i ett future-objekt. Vi antar att "future_set" bara anropas en
// gång per future-objekt.
void future_set(struct future *f, int value) {
	
	f->value = value;
	f->has_value = true;
	sema_up(&f->sema);
}

// Hämta värdet som lagrades tidigare med hjälp av "future_set". Om det inte
// finns ett värde ännu ska "future_get" vänta på att ett värde sätts.
int future_get(struct future *f) {
	
	sema_down(&f->sema);
	sema_up(&f->sema);

	return f->value;
}


/**
 * Exempel för hur en funktion kan köras i en annan tråd, och hur resultatet kan
 * hanteras med hjälp av en "future".
 *
 * Denna kod är given för att visa hur koden i uppgiften kan användas, och är
 * inte en del av uppgiften. Koden nedan kommer inte att rättas, så gör ingen
 * synkronisering där!
 */

// Funktionspekare.
typedef int (*future_function)(int);

// Data delad mellan den startade tråden och originaltråden.
struct thread_data {
	// Det future-objekt vi ska lagra datan i.
	struct future *future;

	// Funktionen som ska köras.
	future_function execute;

	// Parameter till funktionen.
	int parameter;
};

// Funktionen som anropas från en annan tråd.
static void call_future_worker(void *data) {
	struct thread_data *shared = data;

	// Anropa funktionen...
	int result = shared->execute(shared->parameter);

	// ...och spara resultatet.
	future_set(shared->future, result);

	free(shared);
}

// Anropa en funktion i en separat tråd och ge tillbaka en future som kan
// användas för att få tag på resultatet.
struct future *call_future(future_function execute, int parameter) {
	struct thread_data *data = malloc(sizeof(struct thread_data));
	struct future *future = malloc(sizeof(struct future));
	future_init(future);

	data->future = future;
	data->execute = execute;
	data->parameter = parameter;
	thread_create("worker", 0, &call_future_worker, data);

	return future;
}

// Dålig implementation av fibonacci.
int fib(int v) {
	if (v <= 1)
		return v;
	else
		return fib(v - 2) + fib(v - 1);
}

int main() {
	// Fibonacci är så långsam att beräkna... Vi kan göra något annat samtidigt med en future!
	struct future *result1 = call_future(&fib, 35);
	struct future *result2 = call_future(&fib, 40);

	printf("Nu beräknas fibonacci i bakgrunden!\n");

	printf("fib(35) = %d\n", future_get(result1));
	printf("fib(40) = %d\n", future_get(result2));
	printf("fib(35) + fib(40) = %d\n", future_get(result1) + future_get(result2));

	free(result1);
	free(result2);
	return 0;
}
