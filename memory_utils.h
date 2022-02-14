// anything that deals with direct memory access to the game process
#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // for stdint.h and printf macros
#include <assert.h>
#include <string.h>
#include <sys/uio.h> // for process memory manipulation

#include "golden_sun.h"
#include "golden_sun_utils.h"

void get_unit_data(pid_t pid, void* start_ptr, Unit* units, size_t unit_n);
uint8_t get_battle_menu(pid_t pid, uint8_t* wram_ptr);

// the names to use for the memory search. assumes name limit of 5 characters
#define NAME_P1 "Isaac"
#define NAME_P2 "Garet"
char* strstr_n(char* haystack_start, size_t haystack_n, char* needle, size_t needle_n);
uint8_t* find_wram(pid_t pid);
pid_t find_pid();

void get_djinn(pid_t pid, uint8_t* wram_ptr, Unit* allies, Export_Djinn export_djinn);

#endif // header guard
