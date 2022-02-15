#include "golden_sun_utils.h"


// copy from game-wram Unit structure to the more concise game state structure ally
void export_copy_ally(Unit* unit, ExportAlly* send){
  // level thru pp_base
  memcpy(send, &unit->level, offsetof(struct Unit, _unknown1) - offsetof(struct Unit, level) );

  // attack_base thru luck_base
  memcpy(&send->attack_base, &unit->attack_base, offsetof(struct Unit, _unknown2) - offsetof(struct Unit, attack_base));

  // power_venus_base thru luck_max
  memcpy(&send->elemental_base, &unit->elemental_base, offsetof(struct Unit, _unknown3) - offsetof(struct Unit, elemental_base));

  // power_venus_max thru psyenergy
  memcpy(&send->elemental_max, &unit->elemental_max, offsetof(struct Unit, item) - offsetof(struct Unit, elemental_max));

  // class through end
  memcpy(&send->class, &unit->class, offsetof(struct Unit, _unknown10) - offsetof(struct Unit, class));
}


// binary encoding stored djinn have an endian issue, fix it here
uint32_t djinn_to_x86(uint32_t x){
  uint8_t* byte = (uint8_t*) &x;
  uint32_t result = (byte[2]) + (byte[1] << 1) + (byte[0] << 2);
  return result;
}


void golden_sun_print_unknowns(Unit* unit){
  Unknown unknowns[10];

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


uint32_t health_total(Unit* units, size_t n){
  uint32_t health = 0;
  for (size_t i=0; i<n; ++i){
    health += units[i].health_current;
  }
  return health;
}
