/**

Svar på delfråga a:


Svar på delfråga b:


Svar på delfråga c:


Svar på delfråga d:


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wrap/atomics.h"
#include "wrap/synch.h"
#include "wrap/thread.h"

// Antal trådar som ska användas.
#define NUM_THREADS 4

// Funktion som kan parallelliseras.
typedef int (*work_function)(int);

// Datastruktur som delas mellan trådar och beskriver vad som ska göras och hur
// långt vi har kommit.
struct work_queue {
	// Vad ska göras?
	work_function fn;

	// Indata
	int *input;

	// Utdata.
	int *output;

	// Storlek på datan.
	int size;

	// Nästa element i 'data' som ska behandlas.
	struct lock pos_lock;
	int pos;
};

struct semaphore exit_program_sema;

// Funktion som startas av 'run_in_parallel'. Hämtar arbete från 'work_queue'
// och bearbetar ett element i taget.
void worker_function(void *param) {
	struct work_queue *queue = param;
	work_function fn = queue->fn;

	while (true) {
		// Hitta ett element att bearbeta!
		lock_acquire(&queue->pos_lock);
		int index = queue->pos;
		queue->pos++;
		lock_release(&queue->pos_lock);

		// Är vi klara?
		if (index >= queue->size)
			break;

		// Utför arbetet!
		queue->output[index] = fn(queue->input[index]);
	}
	sema_up(&exit_program_sema);
}

// Kör funktionen 'fn' parallellt. Ska fungera som 'run_in_serial', men fördelar
// arbetet på ett antal olika trådar.
void run_in_parallel(work_function fn, int *input, int *output, int size) {
	struct work_queue queue;
	queue.fn = fn;
	queue.input = input;
	queue.output = output;
	queue.size = size;
	queue.pos = 0;
	lock_init(&queue.pos_lock);
	sema_init(&exit_program_sema, 0);

	for (int i = 0; i < NUM_THREADS; i++)
		thread_create("worker", 0, worker_function, &queue);

	for (int i = 0; i < NUM_THREADS; ++i)
	{
		sema_down(&exit_program_sema);
	}
}

// Visar hur 'run_in_parallel' bör bete sig.
void run_in_serial(work_function fn, int *input, int *output, int size) {
	for (int i = 0; i < size; i++)
		output[i] = fn(input[i]);
}


/**
 * Huvudprogram. Endast för att visa hur vi kan köra programmet. Vid rättning
 * kommer eventuella modifikationer härifrån och nedåt att ignoreras.
 */

// Enkel kvadrering.
int square(int x) {
	return x * x;
}

// Fibonaccisekvensen implementerat rekursivt.
int fib(int x) {
	if (x <= 1)
		return x;
	else
		return fib(x - 1) + fib(x - 2);
}

#define DATA_SIZE 40

int main() {
	int input[DATA_SIZE] = { 0 };
	int output[DATA_SIZE] = { 0 };
	for (int i = 0; i < DATA_SIZE; i++)
		input[i] = i;

	// Kör fi	onacci parallellt.
	run_in_parallel(&fib, input, output, DATA_SIZE);
	
	// Kan även testa att köra "square" i stället.
	//run_in_parallel(&square, input, output, DATA_SIZE);

	printf("Resultat:\n");
	for (int i = 0; i < DATA_SIZE; i++)
		printf("%2d: %10d\n", input[i], output[i]);

	return 0;
}
