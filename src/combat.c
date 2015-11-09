#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "headers/combat.h"
#include "headers/menus.h"


int combat(soul_t *ptr)
{
	int n;
	char ch, string[sizeof(long) + 1], *tmp;

	printf("\n\tLevel of dummy [1 - 9]: ");
	fgets(string, (sizeof(long) + 1), stdin);
	n = strtol(string, &tmp, 10);

	printf("\n\t[a]rcher, [m]age, [w]arrior...\n");
	printf("\tType of mob [a, m, w]: ");
	ch = getchar();
	getchar();	// Captures newline from previous; So you don't skip a round.

	if((n > 0 && n < 10 ) && (ch == 'a' || ch == 'm' || ch == 'w'))
	{
		struct soul *mPtr = malloc(sizeof(soul_t));
		soul_create(mPtr, "Training Dummy", "WAF", ch, n);
	
		if(round_start(ptr, mPtr))	// Start combat.
			round_post(ptr, mPtr, 0); // 0 returned from round_start. (player lived)
		else 
			round_post(ptr, mPtr, 1); // 1 returned from round_start. DEATH OF PLAYER
	
		free(mPtr);
	} else { 
		printf("\n Bad options supplied!\n");
		getchar();
		tools("pause", NULL);
	}

	return 0;
}


int round_start(soul_t *player, soul_t *npc)
{
	soul_t *a_, *d_;	//a_ = attacker, d_ = defender
	byte n;	// byte n is what will be returned to the function that calls on this.
	int w;	// Used in the switch statement 

	/*b_buff_t buf, *pbuf;
	pbuf = &buf;

	memset(&buf.buff, 0, sizeof(buf.buff));
	buf.start = &buf.buff[0];
	buf.end   = &buf.buff[MAXCHAR - 1];
	buf.pos   = 0; */

	n = -1;	// n is returned at the completion of this function.

	player->attr.range_c = player->attr.range; 	// Start of round to set current range
	npc->attr.range_c = npc->attr.range;		//  to the max range of the mob/person.
	
	tools("clear", NULL);
	//printf("Player hp: %d, Monster: %d\n", player->hp_c, npc->hp_c);

	while(player->hp_c != 0 && npc->hp_c != 0)
	{
		player->hp = ((player->attr.strength * 3) + 50);
		menus(player, 11);	// 11 for *new* menu.
		menus(npc, 1);		// 33 for *new* menu.

		player->dmg = damage_calc(player);	// Calculate players damage for the round.
		npc->dmg = damage_calc(npc);

		//printf(" [ DEBUG ]  %c Damage: %d\n", player->type, player->dmg);
		//printf(" [ DEBUG ]  %c Damage: %d\n", npc->type, npc->dmg);

		player->def = defense_calc(player);
		npc->def = defense_calc(npc);

		//printf(" [ DEBUG ]  %c Defense: %d\n", player->type, player->def);
		//printf(" [ DEBUG ]  %c Defense: %d\n", npc->type, npc->def);

		if(player->speed > npc->speed)	// Figure out who is attacker and who is the 
		{				//  defender for this round. This pointer will
			a_ = player;		//  be passed to the attacker/defender functions
			d_ = npc;		//  in lieu of the the actual initialised structs.
		} else {
			a_ = npc;
			d_ = player;
		}

		switch(w = range_count(a_, d_))		// Checks the range between each mob. 
		{					// If:   The attack range is greater than the
			case 1:				//  defenders max range, it will attack
				attack(a_, d_);	//  without being attacked.
				break;			// Performs the same check for defender.
			case 2:	
				attack(d_, a_);	// Defender attacks if has higher range.
				break;
			case -1:
				break;
			default:
				attack(a_, d_);	// Attackers attack.
				if(d_->hp_c == 0)	// Placed to prevent excess
					break;		//  turn from happening.
				attack(d_, a_);	// Defender's attack.
		}
		
		if(a_->hp_c == 0 || d_->hp_c ==0)	// A check for HP, to end the round.
			break;

		//fputs("\0337", stdout);	// Part of *new* menu.
		//fputs("\033[0;4H\r", stdout); // Part of *new* menu.
		menus(player, 2); 		// 3  for *new* menu.
		menus(npc, 11);			// 12 for *new* menu.
		//fputs("\0338", stdout);	// Part of *new* menu

		player->speed = (rand() % (sizeof(byte) - 10));	// Calculate speeds for next round.
		npc->speed = (rand() % (sizeof(byte) - 20));

		tools("menu", player);
	}

	if(player->hp_c != 0)	// At the end of round, player is not dead...
		n = 1;		// Return players survival.
	else if(npc->hp_c != 0)	// Else if, player is dead...
		n = 0;		// Return that you have fallen :(
	else
		n = -1;		// Unhandled error check.

	tools("pause", NULL);

	return n;
}


void round_post(soul_t *player, soul_t *npc, byte result)
{
	tools("clear", NULL);
	
	if(result == 0)		// Survived the round, calc gold etc.
	{
		printf(" NPC Gold: " BYEL "%.2f\n" RESET, npc->gold);
		printf(" Player Gold: " BYEL "%.2f\n" RESET, player->gold);

		/*  Calculate player obtained gold (includes luck) */
		player->gold += (npc->gold + (rand() % (player->luck * 2)));
		printf(" New player gold: " BYEL "%.2f\n" RESET, player->gold);


	} else if(result == 1)	// Player death.
		player->hp_c = player->hp;	// Prepare next round by maxing player HP again.
	else
		printf("\nWTF ERROR OCCURED!\n");	// Back to that w = -1 unhandled error.

	tools("pause", NULL);
}


int damage_calc(soul_t *ptr)
{
	int n, o_mod;		// n is the retuned value. o_mod is used to randomize dmg more.
	char c; 		// c (class) is used to get the proper damage function.
	double wis, dex, str;	// Holds the attributes of the ptr being passed.
	
	c   = ptr->attr.cls;		// Class
	wis = ptr->attr.wisdom;		// Wisdom
	dex = ptr->attr.dexterity;	// Dexterity
	str = ptr->attr.strength;	// Strength
	o_mod = 5;			// Modulus for damage.

	switch(c)
	{
		case 'a':	// Archer
			n = ((rand() % (int)((dex * 1.5) + (wis / 2.0))) + (rand() % o_mod));
			break;
		case 'm':	// Mage
			n = ((rand() % (int)((wis * 2.0) + (dex / 2.0))) + (rand() % o_mod));
			break;
		case 'w':	// Warrior
			n = ((rand() % (int)((str * 2.0) + dex)) + (rand() % o_mod));
			break;
		default:	// Possible error, used for detection.  
			printf("\n [ ERROR ] An error occured in 'damage_calc' function.");
			printf("\n [ ERROR ] Class: %c\n", c);
			n = -1;
			tools("pause", NULL);
	}
	//printf(" [ DEBUG ]   n = %d\n", n);

	if(ptr->type == 'm')
		n = (double)n * (3.0 / 4.0);
	//printf(" [ DEBUG ]   n = %d\n", n);

	return n;
}


int range_count(soul_t *a_, soul_t *d_)
{
	int n, ar, dr, ar_c, dr_c, t;

	ar   = a_->attr.range;		// Attackers range (max)
	dr   = d_->attr.range;		// Defenders range (max)
	ar_c = a_->attr.range_c;	// Attackers range (current)
	dr_c = d_->attr.range_c;	// Defenders range (currecnt)

	t= ar + dr;		// Tiles away. (0 + 0) means in melee range.
	n = -1;			// Set return value to error.
	
	if(t != 0)		// If not in melee range...
	{
		if(ar_c > dr)	// If the attackers current is larger than the max of defender
		{
			ar_c--;	// Decrease the distance by 1 (THIS SHOULD BE CHANGED WHERE DEFENDER
			n = 1;	//  MOVES, set return value.
		} else if(dr_c > ar)	// Defender current is greater than attackers max.
		{
			dr_c--;	// Decrease the distance.
			n = 2;	// return the value.
		} else
		{
			n = 0;
			/*ar_c--;
			dr_c--;			Depreciated.
			n = -1; */
		} 
	} else
	{
		n = 0;	// Both in range.
	}

	a_->attr.range_c = ar_c;	// Refresh the values back
	d_->attr.range_c = dr_c;	//  to the structures.
	

	return n;
}


void attack(soul_t *attacker, soul_t *defender)
{
	int t_dmg; 		// Temp damage conversion
	char a_sign[13];
	char d_sign[13];	// Sign {+,-} that will be used.

	memset(&a_sign, 0, sizeof(a_sign));
	memset(&d_sign, 0, sizeof(d_sign));

	//a_sign = '&';	// Will initialize and also represent an error.
	//d_sign = '&';

	if(attacker->dmg > defender->def)
		t_dmg = attacker->dmg - defender->def;
	else
		t_dmg = 0;

	if(attacker->type == 'p') {
		// 36 CYAN   31 RED
		stat_check(attacker, 0);
		strncpy(a_sign, "\x1B[36;1m+\033[0m", sizeof(a_sign));
		strncpy(d_sign, "\x1B[31;1m-\033[0m", sizeof(d_sign));

	} else {
		strncpy(a_sign, "\x1B[31;1m-\033[0m", sizeof(a_sign));
		strncpy(d_sign, "\x1B[36;1m+\033[0m", sizeof(d_sign));
	}

	if(defender->def > attacker->dmg || t_dmg == 0) {
		printf("\t[%s]  %s performed a " KCYN "full block" RESET " of %s's attack!\n\n", a_sign, defender->name, attacker->name);

	} else if(t_dmg >= defender->hp_c) {		// Kill defender if damage is greater than health.
		printf("\t[%s]  %s did " BRED "%d" RESET " damage to %s (" BRED "%d" RESET " overkill),\n\t\t %s is dead...\n\n", 
				a_sign, attacker->name, t_dmg, defender->name, (t_dmg - defender->hp_c), defender->name);
		defender->hp_c = 0;	// Set the defenders HP to reflect.
		menus(attacker, 2);	// Post the death menu.

	} else if(t_dmg < defender->hp_c) {
		attacker->dmg -= defender->def;
		defender->hp_c -= attacker->dmg;
		printf("\t[%s]  %s performed " BRED "%d" RESET " damage to %s.\n", a_sign, attacker->name, attacker->dmg, defender->name);
		if(defender->def != 0)
			printf("\t  [%s]  %s blocked " BBLU "%d" RESET " damage!\n\n", d_sign, defender->name, defender->def);

	} else {
		printf("ERROR: Something happened?\n");
		tools("pause", NULL);
	}
}


int defense_calc(soul_t *ptr)
{
	int n, mod;
	int wis, dex, str;

	wis = ptr->attr.wisdom;
	dex = ptr->attr.dexterity;
	str = ptr->attr.strength;

	switch(ptr->attr.cls)
	{
		case 'a':
			mod = 3;
			n = (((rand() % (str + dex)) + (rand() % mod)) / 2);
			break;
		case 'm':
			mod = 1;
			n = (((rand() % (wis + dex)) + (rand() % mod)) / 2);
			break;
		case 'w':
			mod = 5;
			n = (((rand() % (str + dex)) + (rand() % mod)) / 2);
			break;
		default:
			n = -1;
			printf("An error has occured apparently....");
			tools("pause", NULL);
	}

//	if(ptr->type == 'm')
		n = (double)n * (3.0 / 5.0);

	return n;
}


int stat_check(soul_t *ptr, int special)
{
	struct _attr_t *s = &ptr->attr;

	int stat_total, stat_chance, num;

	
	stat_total = (s->wisdom + s->dexterity + s->strength);
	num = (rand() % (765 * 3));

	if(((s->wisdom + 1) != 256) || ((s->strength + 1) != 256) || ((s->dexterity + 1) != 256))
	{
		if(stat_total > 760)
			stat_chance = 1;
		else if(stat_total > 755)
			stat_chance = 2;
		else if(stat_total > 740)
			stat_chance = 2;
		else if(stat_total > 710)
			stat_chance = 4;
		else if(stat_total > 670)
			stat_chance = 8;
		else if(stat_total > 620)
			stat_chance = 16;
		else if(stat_total > 555)
			stat_chance = 32;
		else if(stat_total > 480)
			stat_chance = 64;
		else if(stat_total > 330)
			stat_chance = 128;
		else
			stat_chance = 256;

		if(stat_chance > num && special == 0) {
			stat_gain(ptr);
			return 1;
		}
		else if(stat_chance > num && special == 1)
			return 1;
		else 
			return 0;
	} else
		return 0;

}

void stat_gain(soul_t *ptr)
{
	byte primary, secondary, tertiary;
	char *pri_name, *sec_name, *ter_name;

	switch(ptr->attr.cls)
	{
		case 'm':
			primary   = ptr->attr.wisdom;
			secondary = ptr->attr.dexterity;
			tertiary  = ptr->attr.strength;
			pri_name  = "Wisdom";
			sec_name  = "Dexterity";
			ter_name  = "Strength";
			break;
		case 'a':
			primary   = ptr->attr.dexterity;
			secondary = ptr->attr.strength;
			tertiary  = ptr->attr.wisdom;
			pri_name  = "Dexterity";
			sec_name  = "Strength";
			ter_name  = "Wisdom";
			break;

		case 'w':
			primary   = ptr->attr.strength;
			secondary = ptr->attr.dexterity;
			tertiary  = ptr->attr.wisdom;
			pri_name  = "Strength";
			sec_name  = "Dexterity";
			ter_name  = "Wisdom";
			break;

		default:
			printf("An error has occured in [stat_gain]!\n");
			tools("pause", NULL);
	}

	if(!ptr->attr.p_lck)
	{
		printf("\t[" BYEL "+" RESET "]  " BYEL "%s" RESET " [%d] has increased to ", pri_name, primary);
		primary++;
		printf("%d.\n", primary);

		if(ptr->attr.cls == 'm')
			ptr->attr.wisdom = primary;
		else if(ptr->attr.cls == 'a')
			ptr->attr.dexterity = primary;
		else if(ptr->attr.cls == 'w')
			ptr->attr.strength = primary;
		else
			printf("An error occured in [stat_gain]!\n");

	} else if(!ptr->attr.s_lck)
	{
		printf("\t[" BYEL "+" RESET "]  " BYEL "%s" RESET " [%d] has increased to ", sec_name, secondary);
		secondary++;
		printf("%d.\n", secondary);

		if(ptr->attr.cls == 'm')
			ptr->attr.dexterity = secondary;
		else if(ptr->attr.cls == 'a')
			ptr->attr.strength = secondary;
		else if(ptr->attr.cls == 'w')
			ptr->attr.dexterity = secondary;
		else
			printf("An error occured in [stat_gain]!\n");

	} else if(!ptr->attr.t_lck)
	{
		printf("\t[" BYEL "+" RESET "]  " BYEL "%s" RESET " [%d] has increased to ", ter_name, tertiary);
		tertiary++;
		printf("%d.\n", tertiary);

		if(ptr->attr.cls == 'm')
			ptr->attr.strength = tertiary;
		else if(ptr->attr.cls == 'a')
			ptr->attr.wisdom = tertiary;
		else if(ptr->attr.cls == 'w')
			ptr->attr.wisdom = tertiary;
		else
			printf("An error occured in [stat_gain]!\n");

	} else
		printf("Total unknown error in [stat_gain]!\n");
}

