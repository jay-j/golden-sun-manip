// these are how the game stores things in ram; in general don't modify
#ifndef GOLDEN_SUN_H
#define GOLDEN_SUN_H

#include <stdint.h>

#define ELEMENTS 4
#define ELEMENT_VENUS 0
#define ELEMENT_MERCURY 1
#define ELEMENT_MARS 2
#define ELEMENT_JUPITER 3

#pragma pack(push, 1)
typedef struct {
  uint8_t spell;
  uint8_t source;
  uint16_t zeros;
} Psyenergy;

typedef struct{
  uint8_t item;
  uint8_t status;
} Item;

typedef struct{
  uint16_t power;
  uint16_t resistance;
} ElementalAffinity;

#define ALLIES 4
#define MEMORY_OFFSET_ALLIES_ORDER 0x438
#define MEMORY_OFFSET_ALLIES 0x500
#define MEMORY_OFFSET_ENEMY 0x30878
#define ENEMIES_MAX 5
typedef struct Unit {
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

  uint8_t _unknown8[2];

  uint8_t evil_spirit;
  uint8_t premonition; // "downed in N turns"

  uint8_t _unknown9[2];

  uint8_t extra_moves;
  uint8_t frozen; // "unable to move"

  uint8_t boost_agility_duration;
  uint8_t boost_agility;

  uint8_t _unknown10[4];

} Unit;


typedef struct Djinn_Queue_Item {
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
#define DJINN_QUEUE_MAX_LENGTH 64
typedef Djinn_Queue_Item Djinn_Queue[DJINN_QUEUE_MAX_LENGTH]; // TODO not enough for the full TLA party?
// queue counter is 256 bytes after queue start (0x02000354)
#define MEMORY_OFFSET_DJINN_QUEUE_LENGTH 0x354

#define MEMORY_OFFSET_BATTLE_MENU 0x31054

#define MEMORY_OFFSET_BATTLE_ACTION_QUEUE 0x30338
#define BATTLE_ACTION_QUEUE_MAX_LENGTH (ALLIES+5)
typedef struct Battle_Action {
  // ALLIES: true (not party order) character id
  // ENEMIES: 0x80.. 0x81.. ordered as they apper in battle left to right
  // 0xff if downed or empty slot
  uint8_t actor_id;

  uint8_t unknown1[3];

  // after the command phase of battle, the queue is sorted on this value
  uint8_t agility; 

  uint8_t unknown2;

  // 00 = attack, 01 = psyenergy, 02 = item, 03 = defend, 04 = monster only?
  // 05 = djinn,  06 = summon
  // 08 = asleep
  uint8_t action_type;

  // modifier?? always seems to be zero
  uint8_t unknown3;

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

  uint8_t unknown4[5];

} Battle_Action;  
// is a 16-byte (0x10) size structure
// TODO make some way of determining unknowns? macros????
// 


#pragma pack(pop)

#endif // header guard
