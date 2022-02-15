// repackage data to make it a cleaner input set
#ifndef GOLDEN_SUN_EXPORT_H
#define GOLDEN_SUN_EXPORT_H

#include<stdio.h>
#include<stdint.h>
#include<stddef.h> // for offsetof()
#include<string.h> // for memcpy
#include "golden_sun.h"

#pragma pack(push, 1)

typedef struct ExportAlly{
  uint8_t level;
  uint16_t health_base; 
  uint16_t pp_base;
  uint16_t attack_base;
  uint16_t defense_base;
  uint16_t agility_base;
  uint8_t luck_base;

  ElementalAffinity elemental_base[ELEMENTS];
  
  uint16_t health_max;
  uint16_t pp_max;
  uint16_t health_current;
  uint16_t pp_current;

  uint16_t attack_max;
  uint16_t defense_max;
  uint16_t agility_max;
  uint8_t luck_max;

  ElementalAffinity elemental_max[ELEMENTS];

  Psyenergy psy[32];

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

} ExportAlly;

void export_copy_ally(Unit* unit, ExportAlly* send);

// information about each djinn
typedef struct Export_Djinn_Item {
  uint8_t element;
  uint8_t status; // 0=set, small positive=recovery, 255=standby
  uint32_t id; // binary encoded, unique per element
} Export_Djinn_Item;

// djinn element order is venus, mercury, mars, jupiter
typedef struct Export_Djinn_List{
  uint16_t quantity;
  Export_Djinn_Item djinn[9];
} Export_Djinn_List;

// a djinn list for every character
typedef Export_Djinn_List Export_Djinn[4];


// todo write unknowns array. with memory pointer offset and number of bytes. so I can just loop through all of them.
typedef struct {
  size_t offset;
  size_t length;
} Unknown;

void golden_sun_print_unknowns(Unit* unit);

uint32_t djinn_to_x86(uint32_t x);

uint32_t health_total(Unit* units, size_t n);

#pragma pack(pop)
#endif // header guards
