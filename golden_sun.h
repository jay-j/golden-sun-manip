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

typedef uint16_t djinn;

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

  uint8_t _unknown4[2];
  djinn djinn_venus_have;
  uint8_t _unknown5[2];
  djinn djinn_mercury_have;
  uint8_t _unknown6[2];
  djinn djinn_mars_have;
  uint8_t _unknown7[2];
  djinn djinn_jupiter_have;

  uint8_t _unknown8[2];
  djinn djinn_venus_set;
  uint8_t _unknown9[2];
  djinn djinn_mercury_set;
  uint8_t _unknown10[2];
  djinn djinn_mars_set;
  uint8_t _unknown11[2];
  djinn djinn_jupiter_set;

  uint8_t _unknown12[2];

  uint8_t djinn_venus_qty_total;
  uint8_t djinn_mercury_qty_total;
  uint8_t djinn_mars_qty_total;
  uint8_t djinn_jupiter_qty_total;

  uint8_t djinn_venus_qty_set;
  uint8_t djinn_mercury_qty_set;
  uint8_t djinn_mars_qty_set;
  uint8_t djinn_jupiter_qty_set;

  uint8_t _unknown13[4];

  uint32_t experience; // might be uint24_t?? 

  uint8_t _unknown14[1]; 

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

  uint8_t _unknown15[1];

  uint16_t stunned; // TODO this doesn't seem right

  uint8_t psyseal_duration; // TODO check
  uint8_t psyseal; 

  uint8_t _unknown16[2];

  uint8_t evil_spirit;
  uint8_t premonition; // "downed in N turns"

  uint8_t _unknown17[2];

  uint8_t extra_moves;
  uint8_t frozen; // "unable to move"

  uint8_t boost_agility_duration;
  uint8_t boost_agility;

  uint8_t _unknown18[4];

} Unit;

#pragma pack(pop)


// TODO write function to parse the djinn structs

// todo write unknowns array. with memory pointer offset and number of bytes. so I can just loop through all of them.
typedef struct {
  size_t offset;
  size_t length;
} Unknown;

Unknown unknowns[18];

void golden_sun_print_unknowns(Unit* unit){

  // todo populate unknowns array somewhere else..
  unknowns[0].offset = 20;
  unknowns[0].length = 4;
  unknowns[1].offset = 31;
  unknowns[1].length = 5;
  unknowns[2].offset = 67;
  unknowns[2].length = 5;

  // these are probably used for more djinn in TLA
  unknowns[3].offset = 246;
  unknowns[3].length = 2;
  unknowns[4].offset = 250;
  unknowns[4].length = 2;
  unknowns[5].offset = 254;
  unknowns[5].length = 2;
  unknowns[6].offset = 258;
  unknowns[6].length = 2;
  unknowns[7].offset = 262;
  unknowns[7].length = 2;
  unknowns[8].offset = 266;
  unknowns[8].length = 2;
  unknowns[9].offset = 270;
  unknowns[9].offset = 2;
  unknowns[10].offset = 274;
  unknowns[10].length = 2;

  // not sure about this one, it is adjacent to the djinn section
  unknowns[11].offset = 278;
  unknowns[11].length = 2;

  // back to zero clue
  unknowns[12].offset = 288;
  unknowns[12].length = 4;
  unknowns[13].offset = 296;
  unknowns[13].length = 1;
  
  // some kind of battle status
  unknowns[14].offset = 313;
  unknowns[14].length = 1;
  unknowns[15].offset = 318;
  unknowns[15].length = 2;
  unknowns[16].offset = 322;
  unknowns[16].length = 2;

  // end stuff no idea
  unknowns[17].offset = 328;
  unknowns[17].length = 4;


  for (size_t i=0; i<18; ++i){
    printf("Unknown %ld:  ", i);
    for (size_t j=0; j<unknowns[i].length; ++j){
      uint8_t* ptr = (uint8_t*) unit;
      printf("%hhx  ", *(ptr+unknowns[i].offset+j)); // todo pre-padding? 
    }
    printf("\n");
  }
}

#endif // header guard
