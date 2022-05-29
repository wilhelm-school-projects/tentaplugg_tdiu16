/*

Svar på delfråga (a):

		Det som kan hända är att do_work gör sema_up på semaforen och sedan sker trådbyte.
		I och med detta kan det vara så att wait_sum fortfarande kör och gör sema_down (vilket alltså går)
		och sedan skrivs värdet ut i main. detta värde kommer troligtvis ha ett felaktigt värde
		eftersom do_work inte la in något värde i "result"-variabeln innan den gjorde sema_up.
 
		Det är även möjligt att segmentation fault kan uppstå eftersom wait_sum gör free på worker
		och sedan i do_work försöker vi accessa detta minne då vi läggs in i processorn igen. 
 */

#include "wrap/thread.h"
#include "wrap/synch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct worker {
	const char *file;
	int result;

	struct semaphore done;
};

// Funktion som körs i en separat tråd. Öppnar en fil,
// läser 100 tal från den och beräknar summan.
void do_work(void *data) {
	struct worker *d = data;

	// Öppna filen.
	FILE *f = fopen(d->file, "r");
	if (f == NULL) {
		sema_up(&d->done);
		d->result = -1;
		return;
	}

	// Läs och summera 100 tal (vi antar att det går bra).
	int sum = 0;
	for (int i = 0; i < 100; i++) {
		// Läs in ett tal:
		int number = 0;
		fscanf(f, "%d", &number);
		sum += number;
	}

	// Stäng filen och rapportera att allt gick bra!
	fclose(f);
	d->result = sum;		
	sema_up(&d->done);
}

// Startar en tråd som summerar de 100 första talen i filen 'file'
// i bakgrunden. Anropa 'wait_worker' för att hämta resultatet.
struct worker *start_sum(const char *file) {
	struct worker *data = malloc(sizeof(*data));
	data->file = file;
	data->result = 0;
	sema_init(&data->done, 0);

	thread_create("worker", 0, &do_work, data);
	return data;
}

// Väntar på att summeringstråden blir klar och returnerar resultatet
// från beräkningen.
int wait_sum(struct worker *worker) {
	sema_down(&worker->done);
	int result = worker->result;
	free(worker);
	return result;
}


/**
 * Testprogram. Du ska inte behöva ändra något härifrån och nedåt.
 */


int main() {
	struct worker *worker = start_sum("numbers.txt");

	// Gör lite annat arbete...

	int result = wait_sum(worker);
	printf("Resultat: %d\n", result);

	return 0;
}
