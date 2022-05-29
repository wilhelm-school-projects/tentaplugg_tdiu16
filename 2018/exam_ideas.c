/*

Svar på deluppgift (a):

	while (buffer->count == 0)
		;

	while (buffer->ideas[pos] == NULL) {
		pos = (pos + 1) % BUFFER_SIZE;
	}

Svar på deluppgift (b):

	busy-wait bör undvikas eftersom när programmet körs i en busy-wait
	kommer den inte göra något vettigt. Alltså det enda processen gör är
	att kolla om den kan ta resursen den väntar på. Istället skulle en 
	annan process kunna köra och göra vettiga saker.

*/

#include "wrap/synch.h"
#include "wrap/thread.h"
#include "wrap/atomics.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 32

struct idea_buffer {
	// Alla idéer i buffern. Tomma platser representeras av NULL.
	const char *ideas[BUFFER_SIZE];

	struct semaphore sema;
	struct lock buffer_lock;

	// Antal platser som är fyllda i 'ideas'.
	int count;
};

// Initiera buffern.
void idea_init(struct idea_buffer *buffer) {
	for (int i = 0; i < BUFFER_SIZE; i++)
		buffer->ideas[i] = NULL;
	sema_init(&buffer->sema, 0);
	buffer->count = 0;
}

// Lägg till en ny idé på en ledig plats i buffern. Om buffern är
// full returneras 'false'.
bool idea_add(struct idea_buffer *buffer, const char *idea) {
	// Hitta en ledig plats.
	int found = BUFFER_SIZE;
	for (int i = 0; i < BUFFER_SIZE; i++) {
		if (buffer->ideas[i] == NULL) {
			found = i;
			break;
		}
	}

	// Fullt?
	if (found >= BUFFER_SIZE)
		return false;

	// Sätt in i buffern.
	buffer->ideas[found] = idea;
	buffer->count++;
	sema_up(&buffer->sema);

	return true;
}

// Ta bort ett element ur buffern. Elementen plockas ut i
// slumpmässig ordning. Om det inte finns några idéer ska
// funktionen vänta på att nya idéer läggs till.
const char *idea_get(struct idea_buffer *buffer) {
	
	sema_down(&buffer->sema);
	// while (buffer->count == 0)
	// 	;
	// buffer->count--;

	// Hitta ett element. Börja med att slumpa fram ett element,
	// om det är tomt tittar vi på nästa och så vidare. (Notera:
	// dålig slump, men den duger)
	int pos = rand() % BUFFER_SIZE;
	while (buffer->ideas[pos] == NULL) {
		pos = (pos + 1) % BUFFER_SIZE;
	}

	const char *result = buffer->ideas[pos];
	buffer->ideas[pos] = NULL;

	return result;
}



/**
 * Huvudprogram. Endast för att visa hur vi kan köra
 * programmet. Vid rättning kommer eventuella modifikationer
 * härifrån och nedåt att ignoreras.
 */

const char *sample_ideas[] = {
	"Bagerier", "Kortspel", "Tärningar", "Mazariner", NULL
};

void worker(void *param) {
	struct idea_buffer *buffer = param;

	for (int i = 0; sample_ideas[i]; i++) {
		idea_add(buffer, sample_ideas[i]);
	}
}

int main(void) {
	srand(time(NULL));

	struct idea_buffer buffer;
	idea_init(&buffer);

	thread_create("worker", 0, &worker, &buffer);

	for (int i = 0; sample_ideas[i]; i++) {
		printf("Idé: %s\n", idea_get(&buffer));
	}

	return 0;
}
