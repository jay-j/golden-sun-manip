#define _GNU_SOURCE
#include <stdio.h>
#include <inttypes.h> // for stdint.h and printf macros
#include <assert.h>
#include <sys/uio.h>
#include <stddef.h> // for offsetof()

#include "golden_sun.h"
#include "golden_sun_utils.h"

// print what address to punch in PINCE to manipulate djinn data
void print_djinn_aid(uint8_t* wram_ptr){
  // heap_start + offset_to_isaac + offset_to_djinn
  uint8_t* unit_ptr = wram_ptr + MEMORY_OFFSET_ALLIES;

  for (size_t i=0; i<4; ++i){
    printf("character base location = %p\n", unit_ptr + sizeof(Unit)*i);
    printf("health current location = %p\n", unit_ptr + offsetof(struct Unit, health_current) + sizeof(Unit)*i);

    printf("djinn venus have (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_have) + sizeof(Unit)*i);
    printf("djinn venus set  (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_set) + sizeof(Unit)*i);

    printf("djinn venus qty total = %p\n", unit_ptr + offsetof(struct Unit, djinn_qty_total) + sizeof(Unit)*i);
    printf("djinn venus qty set   = %p\n", unit_ptr + offsetof(struct Unit, djinn_qty_set) + sizeof(Unit)*i);

    /*
    printf("djinn jupiter have (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_have) + sizeof(Unit)*i);
    printf("djinn jupiter set  (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_set) + sizeof(Unit)*i);
    printf("djinn jupiter qty total = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_qty_total) + sizeof(Unit)*i);
    printf("djinn jupiter qty set   = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_qty_set) + sizeof(Unit)*i);
    */
  }
}


void print_battle_action_unknowns(Battle_Action* action){
  uint8_t* a = (uint8_t*) action;
  size_t u[] = {offsetof(struct Battle_Action, unknown1), offsetof(struct Battle_Action, unknown2),
                offsetof(struct Battle_Action, unknown3), offsetof(struct Battle_Action, unknown4),
                offsetof(struct Battle_Action, unknown5), offsetof(struct Battle_Action, unknown6),
                offsetof(struct Battle_Action, unknown7), offsetof(struct Battle_Action, unknown8),
                offsetof(struct Battle_Action, unknown9)
  };

  for (size_t i=0; i<9; ++i){
    uint8_t value = *(a + u[i]);
    if (value > 0){
      printf("%02x  ", *(a + u[i]));
    }
    else{
      printf("__  ");
    }
  }
  printf("\n");
}


void printf_red(){
  printf("\033[0;31m");
}

void printf_yellow(){
  printf("\033[0;33m");
}

void printf_purple(){
  printf("\033[0;35m");
}

void printf_blue(){
  printf("\033[0;34m");
}

void printf_reset(){
  printf("\033[0m");
}

// array to help with element printing
void (*printf_element[])() = {printf_yellow, printf_blue, printf_red, printf_purple};

int main(int argc, char* argv[]){

  // printf("character array is size.... %ld\n", sizeof(Unit));
  assert(sizeof(Unit) == 332);

  // try and convert command line argument into pid
  pid_t pid = find_pid();

  // start of wram is the origin
  uint8_t* wram_ptr = find_wram(pid, MEMORY_TYPE_WRAM_BOARD);

  Unit allies[4]; 
  get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ALLIES, allies, 4);
  for(int i=0; i<4; ++i){
    printf("character: %5s \thealth: %u \tstatus: %u\t agility: %u\n", allies[i].name, allies[i].health_current, allies[i].battle_status, allies[i].agility_max);
  }
  printf("\n");
  
  uint32_t weakness[ELEMENTS];
  for(int i=0; i<ELEMENTS; ++i){
    weakness[i] = 0;
  }
  uint32_t weakness_min = 99999999;
  Unit enemies[5];
  get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ENEMY, enemies, 5);
  printf("ENEMIES:                                     venus  mrcry  mars   jupiter\n");
  for (int i=0; i<5; ++i){
    if (enemies[i].health_current == 0){
      continue;
    }
    printf("%12s \thealth: %u \tresistances: ", enemies[i].name, enemies[i].health_current);
    for (int e=0; e<ELEMENTS; ++e){
      (*printf_element[e])();
      printf("%3u    ", enemies[i].elemental_max[e].resistance);
      weakness[e] += enemies[i].health_current * enemies[i].elemental_max[e].resistance;
      printf_reset();
    }
    printf("\n");
  }
  printf("\n");
  printf("enemy health sum: %u\n", health_total(enemies, 5));

  // figure out which element is weakest
  for (int e=0; e<ELEMENTS; ++e){
    if (weakness[e] < weakness_min){
      weakness_min = weakness[e];
    }
  }

  if (health_total(enemies, 5) > 0){
    printf("                    venus   mrcry    mars   jupiter\n");
    printf("Health*Resistances: ");
    for (int e=0; e<ELEMENTS; ++e){
      (*printf_element[e])();
      printf("%5u   ", 100*weakness[e]/weakness_min);
      printf_reset();
    }
    printf("   (normalized)\n\n");
  }

  //printf("isaac unknown stuff\n");
  //golden_sun_print_unknowns(allies);
  uint8_t* wram_ptr_chip = find_wram(pid, MEMORY_TYPE_WRAM_CHIP);
  Battle_Menu_Navigation info;
  get_battle_menu_navigation(pid, wram_ptr, wram_ptr_chip, &info);
  printf("Battle menu state? %u   Character: %u\n", info.menu_active, info.character);
  printf("L0: %u   L1: %u   L2: %u   L2_djinn: %u   Target: %u\n", info.menu_l0, info.menu_l1, info.menu_l2, info.menu_l2_djinn, info.target);

  Export_Djinn_List ed[ALLIES];
  get_djinn(pid, wram_ptr, allies, ed);
  
  printf("\nDJINN STATUSES:\n");
  for (size_t a=0; a<ALLIES; ++a){
    printf("Character: %lu (%u djinn)\n", a, ed[a].quantity);
    for (size_t d=0; d<ed[a].quantity; ++d){
      switch (ed[a].djinn[d].element){
        case 0:
          printf_yellow();
          break;
        case 1:
          printf_blue();
          break;
        case 2:
          printf_red();
          break;
        case 3:
          printf_purple();
          break;
        default:
          printf_reset();
      }
      printf("  %s: %u\n", djinn_get_name(ed[a].djinn[d]), ed[a].djinn[d].status);
      printf_reset();
    }
  }


  Battle_Action actions[BATTLE_ACTION_QUEUE_MAX_LENGTH];
  get_battle_action_queue(pid, wram_ptr, actions);

  /*
  for(size_t i=0; i<BATTLE_ACTION_QUEUE_MAX_LENGTH; ++i){
    printf("Character: %u \tAction: 0x%02x %02x \tTarget: 0x%02x (falloff %02x)\tDjinn Element: %u  \t ", 
       actions[i].actor_id, actions[i].action_type, actions[i].command, actions[i].target, actions[i].falloff, actions[i].element);
    print_battle_action_unknowns(actions+i);
  }
  */

  return 0;
}
