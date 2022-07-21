// repackage data to make it a cleaner input set
#ifndef GOLDEN_SUN_EXPORT_H
#define GOLDEN_SUN_EXPORT_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stddef.h> // for offsetof()
#include <stdlib.h>
#include <assert.h>
#include <string.h> // for memcpy
#include <sys/uio.h> // for process memory manipulation
#include <errno.h>

#include "golden_sun.h"

uint8_t party_order_to_character_id(pid_t pid, void* wram_ptr, uint8_t position);
void get_unit_data(pid_t pid, void* start_ptr, Unit* units, size_t unit_n);
uint8_t get_battle_menu(Battle_Action* actions);
uint8_t get_battle_menu_character_init(pid_t pid, uint8_t* wram_ptr);
uint8_t get_battle_menu_character_id(pid_t pid, uint8_t* wram_ptr);
uint8_t get_byte(pid_t pid, uint8_t* wram_ptr, int64_t offset);

char* strstr_n(char* haystack_start, size_t haystack_n, char* needle, size_t needle_n);

// These are some magic values.. for me the location of wram can be found at a static offset,
// within the memory chunk on the line above the heap in /proc/[pid]/maps for the emulator process.
#define MEMORY_TYPE_WRAM_BOARD 0x2D3A08 
#define MEMORY_TYPE_WRAM_CHIP 0x2D3A10 
uint8_t* find_wram(pid_t pid, uint64_t type);
pid_t find_pid();

typedef struct __attribute__((__packed__)) ExportAlly{
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

  Psyenergy psy[32]; // TODO change to PsyenergyCompact format
  // uint8_t psy[32]; ?? change to just the spell, ditch the source value also?

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

void export_copy_allies(pid_t pid, uint8_t* wram_ptr, Unit* unit, ExportAlly* send);
void export_copy_allies_single(Unit* unit, ExportAlly* send);
void print_data_ally(ExportAlly* ally);

typedef struct __attribute__((__packed__)) ExportEnemy{
  uint8_t level;
  uint16_t health_base;
  uint16_t pp_base;
  uint16_t attack_base;
  uint16_t defense_base;
  uint16_t agility_base;
  uint8_t luck_base;

  ElementalAffinity elemental_base[ELEMENTS];
  
  uint16_t health_max; // TODO check if these ever change in battle
  uint16_t pp_max;
  uint16_t health_current;
  uint16_t pp_current;

  uint16_t attack_max;
  uint16_t defense_max;
  uint16_t agility_max;
  uint8_t luck_max;

  ElementalAffinity elemental_max[ELEMENTS];

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

} ExportEnemy;

void export_copy_enemies_single(Unit* unit, ExportEnemy* send);
void export_copy_enemies(Unit* unit, ExportEnemy* send);

// information about each djinn
typedef struct __attribute__((__packed__)) Export_Djinn_Item {
  uint8_t element;
  uint8_t status; // 0=set, small positive=recovery, 255=standby
  uint32_t id; // binary encoded, unique per element
} Export_Djinn_Item;

// djinn element order is venus, mercury, mars, jupiter
typedef struct __attribute__((__packed__)) Export_Djinn_List{
  uint16_t quantity;
  Export_Djinn_Item djinn[9];
} Export_Djinn_List;

// a djinn list for every character
// typedef Export_Djinn_List Export_Djinn[ALLIES];

typedef struct __attribute__((__packed__)) ExportAction {
  uint8_t actor;
  uint8_t action_type;
  uint8_t action_cmd;
  uint8_t djinn_element;
  uint8_t target;
} ExportAction;


typedef struct __attribute__((__packed__)) Battle_Menu_Navigation {
  uint8_t menu_active; // nonzero = looking for player input
  uint8_t character;   // independent of order; 0=Isaac always
  uint8_t menu_l0;
  uint8_t menu_l1;
  uint8_t menu_l2;
  uint8_t menu_l2_djinn;
  uint8_t target;
} Battle_Menu_Navigation;

// the memory blobs to save as ML observation and action space
typedef struct __attribute__((__packed__)) ML_Observation_Space {
  ExportAlly allies[ALLIES];
  ExportEnemy enemies[ENEMIES_MAX];
  Export_Djinn_List djinn[ALLIES];
  Battle_Menu_Navigation menu_nav;
} ML_Observation_Space;

typedef struct __attribute__((__packed__)) ML_Action_Space {
  int8_t dpad_up;
  int8_t dpad_down;
  int8_t dpad_left;
  int8_t dpad_right;
  int8_t button_a;
} ML_Action_Space;

void get_djinn(pid_t pid, uint8_t* wram_ptr, Unit* allies, Export_Djinn_List* export_djinn);


// todo write unknowns array. with memory pointer offset and number of bytes. so I can just loop through all of them.
typedef struct {
  size_t offset;
  size_t length;
} Unknown;

void golden_sun_print_unknowns(Unit* unit);

uint32_t djinn_to_x86(uint32_t x);

uint32_t health_total(Unit* units, size_t n);
void get_battle_action_queue(pid_t pid, uint8_t* wram_ptr, Battle_Action* actions);

void export_action_state(Battle_Action* actions_raw, ExportAction* actions_export);

void get_battle_menu_navigation(pid_t pid, uint8_t* wram_ptr, uint8_t* wram_ptr_chip, Battle_Menu_Navigation* info);

const char* djinn_get_name(Export_Djinn_Item djinn);

#endif // header guards
