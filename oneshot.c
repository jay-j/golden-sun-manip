#define _GNU_SOURCE
#include <stdio.h>
#include <inttypes.h> // for stdint.h and printf macros
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/uio.h>
#include <stddef.h> // for offsetof()

#include "golden_sun.h"
#include "golden_sun_export.h"

// https://stackoverflow.com/questions/63669606/reading-memory-of-another-process-in-c-without-ptrace-in-linux
// use these functions to read and write without requiring the process to pause
// process_vm_readv()
// process_vm_writev()

// assumes name limit of 5 characters
#define NAME_P1 "Isaac"
#define NAME_P2 "Garet"

void get_unit_data(pid_t pid, void* start_ptr, Unit* units, size_t unit_n){

  struct iovec local[unit_n]; // how I want the data in this process
  for(size_t i=0; i<unit_n; ++i){
    local[i].iov_base = units+i;
    local[i].iov_len = sizeof(Unit);
  }

  struct iovec remote[1]; // how data is in the other process 
  remote[0].iov_base = start_ptr;
  remote[0].iov_len = unit_n*sizeof(Unit);

  ssize_t n_read = process_vm_readv(pid, local, unit_n, remote, 1, 0);
  assert(n_read == unit_n*sizeof(Unit));

  // fix the endian-ness of the djinn
  for (size_t i=0; i<unit_n; ++i){
    for (size_t e=0; e<4; ++e){
      uint32_t* dj = &units[i].djinn_venus_have + e;
      *dj = djinn_to_x86(*dj);
      dj = &units[i].djinn_venus_set + e;
      *dj = djinn_to_x86(*dj);
    }
  }
}

uint8_t battle_menu(pid_t pid, uint8_t* wram_ptr){
  uint8_t result = 0;
  struct iovec local[1];
  local[0].iov_base = &result;
  local[0].iov_len = 1;

  struct iovec remote[1];
  remote[0].iov_base = wram_ptr + MEMORY_OFFSET_BATTLE_MENU;
  remote[0].iov_len = 1;

  ssize_t n_read = process_vm_readv(pid, local, 1, remote, 1, 0);
  assert(n_read == 1);
  return result;
 }
 

// print what address to punch in PINCE to manipulate djinn data
void print_djinn_aid(uint8_t* wram_ptr){
  // heap_start + offset_to_isaac + offset_to_djinn
  uint8_t* unit_ptr = wram_ptr + MEMORY_OFFSET_ALLIES;

  for (size_t i=0; i<4; ++i){
    printf("character base location = %p\n", unit_ptr + sizeof(Unit)*i);
    printf("health current location = %p\n", unit_ptr + offsetof(struct Unit, health_current) + sizeof(Unit)*i);

    printf("djinn venus have (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_venus_have) + sizeof(Unit)*i);
    printf("djinn venus set  (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_venus_set) + sizeof(Unit)*i);

    printf("djinn venus qty total = %p\n", unit_ptr + offsetof(struct Unit, djinn_venus_qty_total) + sizeof(Unit)*i);
    printf("djinn venus qty set   = %p\n", unit_ptr + offsetof(struct Unit, djinn_venus_qty_set) + sizeof(Unit)*i);

    /*
    printf("djinn jupiter have (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_have) + sizeof(Unit)*i);
    printf("djinn jupiter set  (binary) = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_set) + sizeof(Unit)*i);
    printf("djinn jupiter qty total = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_qty_total) + sizeof(Unit)*i);
    printf("djinn jupiter qty set   = %p\n", unit_ptr + offsetof(struct Unit, djinn_jupiter_qty_set) + sizeof(Unit)*i);
    */
  }

 }


char* strstr_n(char* haystack_start, size_t haystack_n, char* needle, size_t needle_n){
  printf("looking for %s...\n", needle);

  char* haystack = haystack_start; // current
  char* haystack_end = haystack_start + haystack_n;
  int done = 0;
  while (done == 0){
    // move the haystack to point at the first occurance of the first letter of the needle
    haystack = memchr(haystack, (int) needle[0], haystack_end - haystack);

    // see if the rest of the word follows
    if (memcmp(haystack, needle, needle_n) == 0){
      done = 1;
      break;
    }
    else{
      ++haystack;
    }

    if (haystack == haystack_end){
      haystack = NULL;
      break;
    }
  }

  return haystack;
 }


// look through the entire heap memory of this process to find where the allied characters are
// wram starts 1280 bytes before that
uint8_t* find_wram(pid_t pid){

  // read line in /proc/$pid/maps to figure out where the [heap] is
  char filename_map[64];
  sprintf(filename_map, "/proc/%d/maps", pid);
  FILE* fd_map = fopen(filename_map, "r");

  char line_raw[128];
  char* fgets_status;
  fgets_status = fgets(line_raw, sizeof(line_raw), fd_map);
  while (fgets_status != NULL){
    if (strstr(line_raw, "[heap]") != NULL){
      printf("found the heap line!\n%s", line_raw);
      break;
    }
    fgets_status = fgets(line_raw, sizeof(line_raw), fd_map);
  }
  assert(fgets_status != NULL);

  char* line_working = line_raw;
  char* heap_start_str = strtok_r(line_working, "-", &line_working);
  char* heap_end_str = strtok_r(line_working, " ", &line_working);
  printf("the heap goes from 0x%s to 0x%s\n", heap_start_str, heap_end_str);

  uint64_t heap_start = strtol(heap_start_str, NULL, 16);
  uint64_t heap_end = strtol(heap_end_str, NULL, 16);
  uint64_t heap_length = heap_end - heap_start;
  printf("heap! %ld to %ld  (%ld)\n", heap_start, heap_end, heap_length);
  fclose(fd_map);

  // now look for that area in /proc/$pid/mem 
  uint8_t* target_ptr =(uint8_t*) heap_start; 
  size_t buffer_length = heap_length; // read this many bytes
  char* buff_full = malloc(heap_length * sizeof(char));
  struct iovec local[1];
  local[0].iov_base = buff_full;
  local[0].iov_len = buffer_length;

  struct iovec remote[1];
  remote[0].iov_base = target_ptr;
  remote[0].iov_len = buffer_length;

  ssize_t n_read = process_vm_readv(pid, local, 1, remote, 1, 0);
  printf("Memory to search for characters = %ld bytes\n", n_read);

  int search_complete = 0;
  char* buff = buff_full; 
  char* buff_end = buff + heap_length;
  char* found_p1;
  char* found_p2;
  while (search_complete == 0){
    found_p1 = strstr_n(buff, buff_end - buff, NAME_P1, 5);
    assert(found_p1 != NULL);
    printf("Found %s at %p\n", NAME_P1, found_p1);

    found_p2 = strstr_n(buff, buff_end - buff, NAME_P2, 5);
    printf("Found %s at %p\n", NAME_P2, found_p2);
    assert(found_p2 != NULL);

    if (found_p2 - found_p1 == sizeof(Unit)){
      printf("search complete!\n");
      search_complete = 1;
    }
    else{
      buff = found_p1 + 1;
    }
  }

  size_t offset = found_p1 - buff_full; // heap plus this many bytes is where the character data is
  // printf("offset! %ld bytes from heap start to character_1 data\n", offset);

  free(buff_full);

  uint8_t* game_ptr = target_ptr + offset - 1280; // 1280 is offset in bytes from the character to start of WRAM in GS1
  printf("WRAM located at %p\n", game_ptr);
  //print_djinn_aid(game_ptr);

  return game_ptr;
}



int main(int argc, char* argv[]){
  if (argc != 2){
    printf("Usage: sudo ./scan 1234\n");
  }

  printf("character array is size.... %ld\n", sizeof(Unit));
  assert(sizeof(Unit) == 332);

  // try and convert into pid
  pid_t pid = atoi(argv[1]);
  assert(pid != 0);
  printf("got process pid %d\n", pid);

  // use the party data as the game origin.. because why not?
  uint8_t* wram_ptr = find_wram(pid);

  Unit allies[4]; 
  get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ALLIES, allies, 4);
  for(int i=0; i<4; ++i){
    printf("character: %s    health: %u   status: %u\n", allies[i].name, allies[i].health_current, allies[i].battle_status);
    //printf("      venus  : %"PRIu32"    %"PRIu32"    %u    %u\n", allies[i].djinn_venus_have, allies[i].djinn_venus_set, allies[i].djinn_venus_qty_total, allies[i].djinn_venus_qty_set);
    //printf("      mercury: %"PRIu32"    %"PRIu32"    %u    %u\n", allies[i].djinn_mercury_have, allies[i].djinn_mercury_set, allies[i].djinn_mercury_qty_total, allies[i].djinn_mercury_qty_set);
    //printf("      mars   : %"PRIu32"    %"PRIu32"    %u    %u\n", allies[i].djinn_mars_have, allies[i].djinn_mars_set, allies[i].djinn_mars_qty_total, allies[i].djinn_mars_qty_set);
    //printf("      jupiter: %"PRIu32"    %"PRIu32"    %u    %u\n", allies[i].djinn_jupiter_have, allies[i].djinn_jupiter_set, allies[i].djinn_jupiter_qty_total, allies[i].djinn_jupiter_qty_set);
  }

  Unit enemies[5];
  get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ENEMY, enemies, 5);
  for (int i=0; i<5; ++i){
    printf("enemy: %s        health: %u    status: %u\n", enemies[i].name, enemies[i].health_current, enemies[i].battle_status);
  }

  //printf("isaac unknown stuff\n");
  //golden_sun_print_unknowns(allies);

  uint8_t ready = battle_menu(pid, wram_ptr);
  printf("Battle menu? %u\n", ready);

  Export_Djinn ed;
  export_djinn(pid, wram_ptr, allies, ed);

  return 0;
}
