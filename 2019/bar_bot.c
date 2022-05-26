/**

Svar på delfråga a:

	Problemet som uppstår är t.ex. att två trådar kan kolla hur mycket
	som finns av ingredienser till en drink samtidigt och därmed avgöra
	att det finns nog att för att göra drinken. Men i vissa fall kommer
	en tråd att använda det sista av en eller flera ingredienser och 
	därför kommer den andra tråden (som passerat kollen) att inte ha tillgång
	till rätt antal ingredienser.

Svar på delfråga b:


Svar på delfråga c:


Svar på delfråga d:
	Deadlock kan inte uppstå eftersom det görs en swap med avseende på index storlek. Därav
	uppfylls inte kravet "circular wait".x

*/

#include <stdio.h>
#include <string.h>
#include "wrap/atomics.h"
#include "wrap/synch.h"
#include "wrap/thread.h"

#define MAX_INGREDIENTS 20
#define MAX_RECIPES 20

// En ingrediens som finns i baren.
struct ingredient {
	// Namn.
	const char *name;

	// Mängd.
	int amount;

	struct lock ing_lock_redient;

	// Ingrediensens index i arrayen.
	int index;
};

// Ett recept. Varje recept består av exakt två ingredienser.
struct recipe {
	// Namn.
	const char *name;

	// Typ och mängd av den första ingrediensen.
	const char *ingredient1;
	int amount1;

	// Typ och mängd av den andra ingrediensen.
	const char *ingredient2;
	int amount2;
};

// Alla ingredienser vi har i baren för närvarande.
struct ingredient supply[MAX_INGREDIENTS];
int supply_count;

struct ingredient *find_ingredient(const char *name);

// Alla recept vi kan tillaga i baren.
struct recipe recipes[MAX_RECIPES];
int recipe_count;

struct recipe *find_recipe(const char *name);

// Initiera baren. Anropas en gång i början av programmet.
void init_bar() {
	supply_count = 0;
	recipe_count = 0;
}

// Lägg till en ingrediens som finns i baren. Anropar vi "add_supply" på en
// ingrediens som redan finns så ökas den tillgängliga mängden av det element
// som redan finns, annars läggs ett nytt element till i "supply".
// Vi antar att den här funktionen inte anropas parallellt med andra funktioner i programmet.
void add_supply(const char *name, int amount) {
	// Finns varan redan?
	struct ingredient *found = find_ingredient(name);
	if (found) {
		found->amount += amount;
		return;
	}

	// Fanns inte... Initiera ett nytt element!
	int id = supply_count;
	supply[id].name = name;
	supply[id].amount = amount;
	supply[id].index = id;
	lock_init(&supply[id].ing_lock_redient);
	supply_count++;
}

// Lägg till ett recept.
// Vi antar att den här funktionen inte anropas parallellt med andra funktioner i programmet.
void add_recipe(const char *name, const char *ing1, int amount1, const char *ing2, int amount2) {
	struct recipe *recipe = &recipes[recipe_count];
	recipe->name = name;
	recipe->ingredient1 = ing1;
	recipe->amount1 = amount1;
	recipe->ingredient2 = ing2;
	recipe->amount2 = amount2;

	recipe_count++;
}

// Hitta en ingrediens. Returnerar NULL om ingen hittades.
struct ingredient *find_ingredient(const char *name) {
	for (int i = 0; i < supply_count; i++) {
		if (strcmp(name, supply[i].name) == 0) {
			return &supply[i];
		}
	}
	return NULL;
}

// Hitta ett recept. Returnerar NULL om inget hittades.
struct recipe *find_recipe(const char *name) {
	for (int i = 0; i < recipe_count; i++) {
		if (strcmp(name, recipes[i].name) == 0) {
			return &recipes[i];
		}
	}
	return NULL;
}

// Laga till en drink! Returnera 'true' om det lyckades, annars 'false'.
// Denna funktion kan anropas av flera trådar samtidigt, eftersom vi har flera
// barrobotar i baren!
bool make_drink(const char *name) {
	struct recipe *recipe = find_recipe(name);

	// Finns receptet?
	if (recipe == NULL)
		return false;

	// Finns ingredienserna?
	struct ingredient *ing1 = find_ingredient(recipe->ingredient1);
	struct ingredient *ing2 = find_ingredient(recipe->ingredient2);

	if (ing1->index > ing2->index)
	{
		struct ingredient* temp = ing1;
		ing1 = ing2;
		ing2 = temp;
	}

	//Kritisk sektion 1 börjar. Resurs ing1 & ing2
	lock_acquire(&ing1->ing_lock_redient);
	lock_acquire(&ing2->ing_lock_redient);
	if (ing1 == NULL || ing2 == NULL)
	{	
		lock_release(&ing1->ing_lock_redient);
		lock_release(&ing2->ing_lock_redient);
		return false;
	}

	// Notera: Vi antar att ing1 != ing2.

	// Finns det tillräckligt mycket av allt?
	if (ing1->amount < recipe->amount1)
	{
		lock_release(&ing1->ing_lock_redient);
		lock_release(&ing2->ing_lock_redient);
		return false;
	}
	if (ing2->amount < recipe->amount2)
	{
		lock_release(&ing1->ing_lock_redient);
		lock_release(&ing2->ing_lock_redient);
		return false;
	}
		

	// Allt verkar stämma! Vi kan göra drinken!
	ing2->amount -= recipe->amount2;
	ing1->amount -= recipe->amount1;
	lock_release(&ing1->ing_lock_redient);
	lock_release(&ing1->ing_lock_redient);
	//Kritisk sektion 1 slutar.

	return true;
}


/**
 * Huvudprogram. Endast för att visa hur vi kan köra programmet. Vid rättning
 * kommer eventuella modifikationer härifrån och nedåt att ignoreras.
 */

void check(const char *drink) {
	printf("Gör %s... ", drink);
	if (make_drink(drink))
		printf("OK!\n");
	else
		printf("Gick inte bra...\n");
}

int main() {
	init_bar();
	add_supply("mjölk", 100); // 100 cl mjölk
	add_supply("blåbär", 50); // 50 st blåbär
	add_supply("hallon", 50); // 50 st hallon
	add_supply("kakao", 10); // 10 cl kakao

	add_recipe("Blåbärsmjölk", "blåbär", 5, "mjölk", 10);
	add_recipe("Varm choklad", "mjölk", 10, "kakao", 1);
	add_recipe("Chokladblåbär", "kakao", 1, "blåbär", 5);

	// Gör lite drinkar!
	check("Blåbärsmjölk");
	check("Varm choklad");
	check("Chokladblåbär");
	check("Blåbärsmjölk");
	check("Varm choklad");
	check("Blåbärsmjölk");
	check("Varm choklad");
	check("Chokladblåbär");
	check("Blåbärsmjölk");
	check("Varm choklad");
	check("Blåbärsmjölk");
	check("Blåbärsmjölk");
	check("Blåbärsmjölk");
	check("Blåbärsmjölk");

	return 0;
}
