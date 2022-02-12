#ifndef GOLDEN_SUN_H
#define GOLDEN_SUN_H

#include <stdint.h>

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

#define ALLIES 4
#define MEMORY_OFFSET_ALLIES 0x500
#define MEMORY_OFFSET_ENEMY 0x30878
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

  uint16_t power_venus_base;
  uint16_t resistance_venus_base;
  uint16_t power_mercury_base;
  uint16_t resistance_mercury_base;
  uint16_t power_mars_base;
  uint16_t resistance_mars_base;
  uint16_t power_jupiter_base;
  uint16_t resistance_jupiter_base;
  
  uint16_t health_max;
  uint16_t pp_max;

  uint16_t health_current;
  uint16_t pp_current;

  uint16_t attack_max; // TODO are these max or current (after temporary debuffs)? 
  uint16_t defense_max;
  uint16_t agility_max;
  uint8_t luck_max;

  uint8_t _unknown3[5];

  uint16_t power_venus_max; 
  uint16_t resistance_venus_max;
  uint16_t power_mercury_max;
  uint16_t resistance_mercury_max;
  uint16_t power_mars_max;
  uint16_t resistance_mars_max;
  uint16_t power_jupiter_max;
  uint16_t resistance_jupiter_max;

  Psyenergy psy[32];
  Item item[15];

  uint32_t djinn_venus_have;
  uint32_t djinn_mercury_have;
  uint32_t djinn_mars_have;
  uint32_t djinn_jupiter_have;

  uint32_t djinn_venus_set;
  uint32_t djinn_mercury_set; 
  uint32_t djinn_mars_set;
  uint32_t djinn_jupiter_set;

  uint8_t _unknown4[2];

  uint8_t djinn_venus_qty_total;
  uint8_t djinn_mercury_qty_total;
  uint8_t djinn_mars_qty_total;
  uint8_t djinn_jupiter_qty_total;

  uint8_t djinn_venus_qty_set;
  uint8_t djinn_mercury_qty_set;
  uint8_t djinn_mars_qty_set;
  uint8_t djinn_jupiter_qty_set;

  uint8_t _unknown5[4];

  uint32_t experience; // might be uint24_t?? 

  uint8_t _unknown6[1]; 

  uint8_t class;
  

  // all the following are battle status stuff
  uint8_t battle_status;     // 00 if dead, 01 if alive enemy, 02 if alive ally
  uint8_t defending;         // defending or granite djinn
  uint8_t boost_power_venus;  // qty is how many djinn
  uint8_t boost_power_mercury; // qty is how many djinn
  uint8_t boost_power_mars;   // qty is how many djinn
  uint8_t boost_power_jupiter; // qty is how many djinn
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


#pragma pack(pop)

#define MEMORY_OFFSET_BATTLE_MENU 0x31054

// TODO write function to parse the djinn structs

// todo write unknowns array. with memory pointer offset and number of bytes. so I can just loop through all of them.
typedef struct {
  size_t offset;
  size_t length;
} Unknown;

Unknown unknowns[10];

void golden_sun_print_unknowns(Unit* unit){

  // todo populate unknowns array somewhere else..
  unknowns[0].offset = 20;
  unknowns[0].length = 4;
  unknowns[1].offset = 31;
  unknowns[1].length = 5;
  unknowns[2].offset = 67;
  unknowns[2].length = 5;

  // not sure about this one, it is adjacent to the djinn section
  unknowns[3].offset = 278;
  unknowns[3].length = 2;

  // back to zero clue
  unknowns[4].offset = 288;
  unknowns[4].length = 4;
  unknowns[5].offset = 296;
  unknowns[5].length = 1;
  
  // some kind of battle status
  unknowns[6].offset = 313;
  unknowns[6].length = 1;
  unknowns[7].offset = 318;
  unknowns[7].length = 2;
  unknowns[8].offset = 322;
  unknowns[8].length = 2;

  // end stuff no idea
  unknowns[9].offset = 328;
  unknowns[9].length = 4;


  for (size_t i=0; i<10; ++i){
    printf("Unknown %ld:  ", i);
    for (size_t j=0; j<unknowns[i].length; ++j){
      uint8_t* ptr = (uint8_t*) unit;
      printf("%hhx  ", *(ptr+unknowns[i].offset+j)); // todo pre-padding? 
    }
    printf("\n");
  }
}

#endif // header guard
