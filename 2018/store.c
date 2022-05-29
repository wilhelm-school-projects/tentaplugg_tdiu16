/*

Svar på delfråga (a):

	Fyra villkor:
		Circular wait:
			- 	Detta betyder att alla resurser har en resurs och vill ta en. Detta sker i en cirkel
				vilket innebär att alla väntar på varandra. Om Circular wait uppfylls, uppfylls de
				andra 3 också.

		no preemption of resources:
			- 	Det är bara processen som har en resurs som kan släppa denna resurs. Alltså
				kan inte en annan process ta denna resurs.

		hold and wait:
			- 	Det finns process(er) som håller en resurs och väntar på att få ta en annan.

		mutual exclusion:
			-	Om en process har en resurs har den låst denna och ingen annan kan få tillgång till den
				vilket innebär att endast denna process kan låsa upp den.

		Ett scenario som kan uppstå är t.ex:

			1 resurs av kaffe, te respektive bulle

			Det finns 3 processer: p1, p2 & p3
			p1: kaffe + bulle
			p2: te 	  + bulle 
			p3: bulle + kaffe

		p1 tar kaffe -> p2 tar bulle -> p3 tar te
			följt av
		p1 vill ta bulle -> p2 vill ta te -> p3 vill ta kaffe

		I den andra "ronden" kommer alla att behöva vänta för att kunna ta sin
		och detta medför att ingen kommer kunna lämna ifrån sig sin => deadlock!

		mutual exclusion är uppfyllt: den resurs en process håller är låst
		
		hold and wait är uppfyllt: det finns processer som håller en resurs och väntar på en annan
		
		no preemption of recources: endast processen som håller resursen kan släppa den
		
		circular wait: de vill ta och håller resurser i en cirkel

*/

#include "wrap/thread.h"
#include "wrap/synch.h"
#include "wrap/atomics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PRODUCTS 32

// Representerar en produkt som finns i affären.
struct product {
	// Namn på produkten. Om namnet är NULL är platsen ledig.
	const char *name;

	// Antal av denna produkt som finns i lagret.
	int in_stock;

	// Lås för denna produkten.
	struct lock stock_lock;
};

// Alla produkter i affären för närvarande.
struct product products[MAX_PRODUCTS];

// Hjälpfunktion för att hitta en produkt.
int find_product(const char *name) {
	for (int i = 0; i < MAX_PRODUCTS; i++) {
		if (!products[i].name)
			continue;

		// Jämför strängarna. Om de är lika, returnera 'i'.
		// Se 'man strcmp' för detaljer.
		if (strcmp(products[i].name, name) == 0)
			return i;
	}

	return -1;
}

// En kund vill köpa två olika produkter, 'buy1' och
// 'buy2'. Enligt din affärsidé får du bara sälja om du har båda
// produkterna i lager.
bool buy(const char *buy1, const char *buy2) {
	int id1 = find_product(buy1);
	int id2 = find_product(buy2);

	if (id1 < 0 || id2 < 0 || id1 == id2)
		return false;

	/*
		Följande löser circular wait eftersom när när en process
		frågar efter bulle och sedan kaffe, kommer denna process,
		skiljt från de andra, ta bulle först och sedan kaffe. Därav
		kommer denna att behöva vänta på att ta sin första resurs direkt
		och alltså inte roffa åt sig en resurs som den inte kommer 
		kunna släppa. 
	
	*/

	if (id1 < id2)
	{
		lock_acquire(&products[id1].stock_lock);
		lock_acquire(&products[id2].stock_lock);
	}
	else
	{
		lock_acquire(&products[id2].stock_lock);
		lock_acquire(&products[id1].stock_lock);
	}

	bool ok = products[id1].in_stock > 0
		&& products[id2].in_stock > 0;

	if (ok) {
		products[id1].in_stock--;
		products[id2].in_stock--;
	}

	lock_release(&products[id1].stock_lock);
	lock_release(&products[id2].stock_lock);

	return ok;
}


/**
 * Huvudprogram. Endast för att visa hur vi kan köra
 * programmet. Vid rättning kommer eventuella modifikationer
 * härifrån och nedåt att ignoreras.
 */

void setup_products(void) {
	products[0].name = "Kaffe";
	products[0].in_stock = 2;
	products[1].name = "Te";
	products[1].in_stock = 2;
	products[2].name = "Bulle";
	products[2].in_stock = 3;

	for (int i = 0; i < MAX_PRODUCTS; i++)
		lock_init(&products[i].stock_lock);
}

void do_buy(const char *a, const char *b) {
	printf("Köpte %5s och %5s: %5s\n", a, b, buy(a, b) ? "true" : "false");
}

int main(void) {
	setup_products();

	do_buy("Kaffe", "Bulle");	
	do_buy("Te", "Bulle");		
	do_buy("Kaffe", "Bulle");	
	do_buy("Te", "Bulle");		
	return 0;
}
