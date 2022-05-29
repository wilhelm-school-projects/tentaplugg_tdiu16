/*

Svar på deluppgift (a):

	1.	Det kan vara så att idea_add har lagt till 2 idéer och därav kan 2 trådar
		göra sema_down i idea_get. Här skulle de kunna få samma random värde och därför
		få samma plats i ideas-arrayen. På så vis kommer de kunna få samma idé. Det är
		även viktigt att båda trådar hinner ta strängen innan någon av de sätter platsen
		till null.

	2. 	Det kan vara så att idea_add anropas från två trådar "samtidigt". Den första tråden
		lägger till en idé "lätt tenta" med index 0 (eftersom det är den första). sedan kommer
		den andra tråden in och får samma index och har en annan idé "svår tenta" och lägger till
		den. Eftersom samma index använts kommer den tidigare idén skrivas över och endast 
		"svår tenta" finns kvar.

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
	struct lock idea_locks[BUFFER_SIZE];
	struct lock search_idea_lock;

	// Antal platser som är fyllda i 'ideas'.
	int count;
};

// Initiera buffern.
void idea_init(struct idea_buffer *buffer) {
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		buffer->ideas[i] = NULL;
		lock_init(&buffer->idea_locks[i]); 
	}
	lock_init(&buffer->search_idea_lock);
	sema_init(&buffer->sema, 0);
	buffer->count = 0;
}

// Lägg till en ny idé på en ledig plats i buffern. Om buffern är
// full returneras 'false'.
bool idea_add(struct idea_buffer *buffer, const char *idea) {
	// Hitta en ledig plats.
	// Kritisk sektion börjar, resurs: ideas
	int found = BUFFER_SIZE;
	lock_acquire(&buffer->search_idea_lock);
	for (int i = 0; i < BUFFER_SIZE; i++) {
		lock_acquire(&buffer->idea_locks[found]);
		if (buffer->ideas[i] == NULL) {
			found = i;
			break;
		}
		lock_release(&buffer->idea_locks[found]);
	}
	lock_release(&buffer->search_idea_lock);

	// Kritisk sektion börjar, resurs: ideas
	// Fullt?
	if (found >= BUFFER_SIZE)
	{
		lock_release(&buffer->idea_locks[found]);
		return false;
	}

	// Sätt in i buffern.
	// Kritisk sektion börjar, resurs: ideas
	buffer->ideas[found] = idea;
	lock_release(&buffer->idea_locks[found]);
	// Kritisk sektion slutar, resurs: ideas
	
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
	// Kritisk sektion börjar, resurs: ideas
	while (true) {
		lock_acquire(&buffer->idea_locks[pos]);
		pos = (pos + 1) % BUFFER_SIZE;
		if (buffer->ideas[pos] != NULL)
		{
			break;
		}
		lock_release(&buffer->idea_locks[pos]);
	}

	const char *result = buffer->ideas[pos];
	buffer->ideas[pos] = NULL;
	lock_release(&buffer->idea_locks[pos]);
	// Kritisk sektion slutar, resurs: ideas

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











































































































































































