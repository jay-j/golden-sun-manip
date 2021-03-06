// these are how the game stores things in ram; in general don't modify
#ifndef GOLDEN_SUN_H
#define GOLDEN_SUN_H
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#include <stdint.h>

#define ELEMENTS 4
#define ELEMENT_VENUS 0
#define ELEMENT_MERCURY 1
#define ELEMENT_MARS 2
#define ELEMENT_JUPITER 3

typedef struct __attribute__((__packed__)) {
  uint8_t spell;
  uint8_t source; // 0x80 = class, 0x40 = item, 0x00 = universal
  uint16_t zeros;
} Psyenergy;

typedef struct __attribute__((__packed__)) {
  uint8_t item;
  uint8_t status;
} Item;

typedef struct __attribute__((__packed__)) {
  uint16_t power;
  uint16_t resistance;
} ElementalAffinity;

#define ALLIES 4
#define MEMORY_OFFSET_ALLIES_ORDER 0x438
#define MEMORY_OFFSET_ALLIES 0x500
#define MEMORY_OFFSET_ENEMY 0x30878
#define ENEMIES_MAX 5
typedef struct __attribute__((__packed__)) Unit {
  char name[15];
  uint8_t level;
  uint16_t health_base; 
  uint16_t pp_base;
  uint8_t _unknown1[4];
  uint16_t attack_base;
  uint16_t defense_base;
  uint16_t agility_base;
  uint8_t luck_base;
  uint8_t _unknown2[5];
  // uint8_t : 5; // TODO consider using unnamed bitfields to pad the memory instead

  ElementalAffinity elemental_base[ELEMENTS]; // power, resistance
  
  uint16_t health_max;
  uint16_t pp_max;
  uint16_t health_current;
  uint16_t pp_current;

  uint16_t attack_max; // TODO are these max or current (after temporary debuffs)? 
  uint16_t defense_max;
  uint16_t agility_max;
  uint8_t luck_max;

  uint8_t _unknown3[5];

  ElementalAffinity elemental_max[ELEMENTS]; // power, resistance

  Psyenergy psy[32];
  Item item[15];

  uint32_t djinn_have[ELEMENTS];
  uint32_t djinn_set[ELEMENTS];
  uint8_t _unknown4[2];
  uint8_t djinn_qty_total[ELEMENTS];
  uint8_t djinn_qty_set[ELEMENTS];

  uint8_t _unknown5[4];

  uint32_t experience; // might be uint24_t?? 

  uint8_t _unknown6[1]; 

  uint8_t class;

  // all the following are battle status stuff
  uint8_t battle_status;     // 00 if dead, 01 if alive enemy, 02 if alive ally
  uint8_t defending;         // defending or granite djinn
  uint8_t boost_power[ELEMENTS];  // qty is how many djinn
  uint8_t cursed_item;
  uint8_t poisoned;
  uint8_t boost_attack_duration;
  uint8_t boost_attack; // can be positive (e.g. 0x02) or negative (e.g. 0xfe)
  uint8_t boost_defense_duration;
  uint8_t boost_defense;
  uint8_t boost_resistance_duration;
  uint8_t boost_resistance;
  uint8_t delusion_duration;

  uint8_t _unknown7[1];

  uint16_t stunned; // TODO this doesn't seem right

  uint8_t psyseal_duration; // TODO check
  uint8_t psyseal; 

  uint8_t regenerate_duration; // unused battle effect. zero if disabled. if >0 represents the number of turns the ally recovers health
  uint8_t reflect_duration; // unused battle effect. zero if disabled. if >0 represents the number of turns remaining for the effect to last

  uint8_t evil_spirit;
  uint8_t premonition; // "downed in N turns"

  uint8_t _unknown9[2];

  uint8_t extra_moves;
  uint8_t frozen; // "unable to move"

  uint8_t boost_agility_duration;
  uint8_t boost_agility;

  uint8_t _unknown10[4];
  // TODO look at list of unacquirable psyenergies to see if those correspond to any of these statuses

} Unit;


typedef struct __attribute__((__packed__)) Djinn_Queue_Item {
  // the element of the djinn
  // 00=venus, 01=mercury, 02=mars, 03=jupiter
  uint8_t element;

  // zero-indexed djinn serial number within element (not the binary encoding used in character data)
  uint8_t djinn; 

  // independent of current party order TODO check TLA
  // 00=Isaac, 01=Garet, 02=Ivan, 03=Mia 
  uint8_t owner;

  // ff = standby, positive values are turns/ordering during recovery
  uint8_t status;
} Djinn_Queue_Item;

// starting at WRAM 0x02000254
#define MEMORY_OFFSET_DJINN_QUEUE 0x254
#define MEMORY_OFFSET_DJINN_QUEUE_LENGTH 0x354
#define DJINN_QUEUE_MAX_LENGTH 64
typedef Djinn_Queue_Item Djinn_Queue[DJINN_QUEUE_MAX_LENGTH]; // TODO not enough for the full TLA party?


//////////////////////////////////////////////////////////////////////////////////////
// Battle Menu - These are "UI" elements, for navigating the battle menu without graphics

// if zero, NOT listening for player input in battle
#define MEMORY_OFFSET_BATTLE_MENU 0x31054

// Data in one byte. Gives 255 on the "fight, run, status menu".
// gives 0=Isaac, 1=Garret, 2=Ivan, 3=Mia when an action is being selected for their character in battle
// They seem to give gibberish results in between turns
// This is character locked (keeps character even if party order is altered). And keeps them even if the character gets a duplicate turn.
#define MEMORY_OFFSET_BATTLE_MENU_CHARACTER_ID 0x35A24

// are we at an individual character battle menu? e.g. reset action state?
// 00 if active
// f0 if in pre-menu (or other menus - caution)
// f1 if in watching the battle view mode?
#define MEMORY_OFFSET_BATTLE_MENU_L0 0x347E1

// Single byte integer in battle. L1; fight/run, attack/psy/summon, etc.
#define MEMORY_OFFSET_BATTLE_MENU_L1 0x35C0C

// Single byte integers in battle. L2; which psyenergy, which summon, vertical menu position (not page). Does not work for djinn.
// use with MINOR+MAJOR; MINOR jumps by one, MAJOR jumps by 5 in psyenergy, 4 in summons, etc.
#define MEMORY_OFFSET_BATTLE_MENU_L2_MINOR 0x35974
#define MEMORY_OFFSET_BATTLE_MENU_L2_MAJOR 0x35978

// Single byte integers in battle menu, representing the djinn the player is in the process of selecting. 
// Use with MINOR+MAJOR
// TODO note these are IRAM not WRAM offsets
#define MEMORY_OFFSET_BATTLE_MENU_DJINN_MINOR 0x7CEC
#define MEMORY_OFFSET_BATTLE_MENU_DJINN_MAJOR 0x7C98

// Main target of the proposed action. Does not distinguish between allies and enemies; Will be 0-3...
// TODO note these are IRAM not WRAM offsets
#define MEMORY_OFFSET_BATTLE_MENU_TARGET 0x7C8C

//////////////////////////////////////////////////////////////////////////////////////
// These variables store the final actions (after selection is complete).
// Once the player turn is complete, enemy actions are added to the list. 
// Then the list is sorted by agility and actions are taken.
#define MEMORY_OFFSET_BATTLE_ACTION_QUEUE 0x30338
#define BATTLE_ACTION_QUEUE_MAX_LENGTH (ALLIES+ENEMIES_MAX)
typedef struct __attribute__((__packed__)) Battle_Action {
  // ALLIES: true (not party order) character id
  // ENEMIES: 0x80.. 0x81.. ordered as they apper in battle left to right
  // 0xff if downed or empty slot
  uint8_t actor_id;

  uint8_t unknown1;
  uint8_t unknown2;
  uint8_t unknown3;

  // after the command phase of battle, the queue is sorted on this value
  uint8_t agility; 

  // defaults to 0x80 for the whole queue... except for characters that are asleep (other statuses..unknown)
  // once a character command is queued, changes to 0x00 for that character (but never back to 0x80). 0x27 for granite
  // enemies get turned to 0x00 once the command phase of battle ends.
  uint8_t unknown4;

  // 00 = attack, 01 = psyenergy, 02 = item, 03 = defend, 04 = monster only?
  // 05 = djinn,  06 = summon
  // 08 = asleep
  uint8_t action_type;

  uint8_t unknown5;

  // read from the psyenergy save list! 
  // the djinn number.. power per element. e.g vine is 3 because it is 0b 1000
  // for summons there is a different counting system than in the "save file" spec
  uint8_t command;

  // djinn element (per normal elemental rules). not psyenergy related
  uint8_t element;

  // 0x00..0x03 for alies. in party order (not character data order)
  // 0x80, 0x81, 0x82... for enemies (ordering left to right)
  // this is some kind of index; remains even if the original enemy is gone
  uint8_t target; 

  // queue refreshed to 0x00 each turn
  // once a character has queued a move for the first time, turns to 0x01
  // enemies remain 0x00 until they have taken their turn, then 0x01
  uint8_t unknown6;

  // 0xff for no falloff (all targets equally)
  // 0x01 for single target, 0x02 for 3-unit target, 0x03 for 5-person target
  // 0x04 for 7-person target, 0x06 for summon falloff
  uint8_t falloff;

  uint8_t unknown7;
  uint8_t unknown8;
  uint8_t unknown9;

} Battle_Action; // 16 bytes total (stride 0x10) 

// got WRAM 0x63bc2f0 and this at 0x63bc7da
#define MEMORY_OFFSET_ARENA_BATTLES_WON (0x4EA)

#endif // header guard
