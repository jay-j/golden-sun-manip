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

int main(int argc, char* argv[]){
  if (argc != 2){
    printf("Usage: sudo ./scan\n");
  }

  printf("character array is size.... %ld\n", sizeof(Unit));
  assert(sizeof(Unit) == 332);

  // try and convert command line argument into pid
  pid_t pid = find_pid();

  // start of wram is the origin
  uint8_t* wram_ptr = find_wram(pid);

  Unit allies[4]; 
  get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ALLIES, allies, 4);
  for(int i=0; i<4; ++i){
    printf("character: %s    health: %u   status: %u\n", allies[i].name, allies[i].health_current, allies[i].battle_status);
  }

  Unit enemies[5];
  get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ENEMY, enemies, 5);
  for (int i=0; i<5; ++i){
    printf("enemy: %s        health: %u    status: %u\n", enemies[i].name, enemies[i].health_current, enemies[i].battle_status);
  }

  //printf("isaac unknown stuff\n");
  //golden_sun_print_unknowns(allies);

  uint8_t ready = get_battle_menu(pid, wram_ptr);
  printf("Battle menu? %u\n", ready);

  uint8_t cmd = get_battle_menu_character_init(pid, wram_ptr);
  uint8_t char_id = get_battle_menu_character_id(pid, wram_ptr);
  printf("Character %u menu state? %u\n", char_id, cmd);

  Export_Djinn ed;
  get_djinn(pid, wram_ptr, allies, ed);

  return 0;
}
