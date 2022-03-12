#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/uio.h>

#include "golden_sun_utils.h"

// come up with a per element binary bitfield number for each character
#define DJINN_MAX 7

// function - count how many djinn are left available in an element
int count_unallocated(uint32_t djinn){
  int result = 0;
  for(size_t i=0; i<8*sizeof(uint32_t); ++i){
    result += (djinn & (0x1 << i)) > 0;
  }
  return result;
}


void assign_djinn(uint32_t* djinn_pool, uint32_t* djinn_ally, int qty){
  // figure out which element
  int element = rand() % ELEMENTS;
  while (count_unallocated(djinn_pool[element]) < qty){
   element = (element + 1) % ELEMENTS;
  }

  for(size_t i=0; i<qty; ++i){
    // find a free djinn of that element
    int id = rand() % DJINN_MAX;
    uint32_t bin = (0x1 << id);
    while ( (bin & djinn_pool[element]) == 0){
      id = (id + 1) % DJINN_MAX;
      bin = (0x1 << id);
    }
    
    // assign it
    djinn_ally[element] |= bin;

    // mark as in use
    djinn_pool[element] &= ~bin;
  }
}


uint32_t byte_flipper(uint32_t input){
  uint32_t result = 0;
  uint8_t* input_ptr = (uint8_t*) &input;
  uint8_t* output_ptr = (uint8_t*) &result;

  for(size_t i=0; i<3; ++i){
    *(output_ptr + 2 - i) = *(input_ptr + i);
  }
  return result;
}


void write_djinn(pid_t pid, uint8_t* wram_ptr, uint32_t* djinn_ally){

  for(size_t a=0; a<ALLIES; ++a){
    uint8_t* ally_ptr = wram_ptr + MEMORY_OFFSET_ALLIES + a*sizeof(Unit);
    ssize_t n_written;
    struct iovec local[1];
    struct iovec remote[1];

    // write djinn_have
    local[0].iov_base = djinn_ally+a*ELEMENTS;
    local[0].iov_len = 16;
    remote[0].iov_base = ally_ptr + offsetof(struct Unit, djinn_have);
    remote[0].iov_len = 16;
    n_written = process_vm_writev(pid, local, 1, remote, 1, 0);
    assert(n_written == 16);

    // write djinn_set
    remote[0].iov_base = ally_ptr + offsetof(struct Unit, djinn_set);
    n_written = process_vm_writev(pid, local, 1, remote, 1, 0);
    assert(n_written == 16);

    // compute the djinn quantities
    uint8_t djinn_qty[4];
    for(size_t e=0; e<ELEMENTS; ++e){
      djinn_qty[e] = count_unallocated(djinn_ally[ELEMENTS*a + e]);
    }

    // write djinn_qty_total
    local[0].iov_base = djinn_qty;
    local[0].iov_len = 4;
    remote[0].iov_base = ally_ptr + offsetof(struct Unit, djinn_qty_total);
    remote[0].iov_len = 4;
    n_written = process_vm_writev(pid, local, 1, remote, 1, 0);
    assert(n_written == 4);

    // write djinn_qty_set
    remote[0].iov_base = ally_ptr + offsetof(struct Unit, djinn_qty_set);
    n_written = process_vm_writev(pid, local, 1, remote, 1, 0);
    assert(n_written == 4);
  }
}


int main(int argc, char* argv[]){
  // generate random seed
  // optional input argument is seed from time
  if (argc == 2){
    printf("Random seed: %s\n", argv[1]);
    srand(atoi(argv[1]));
  }
  else{
    printf("Generating random seed\n");
    srand(time(0));
  }
  

  // randomize
  uint32_t djinn_pool[ELEMENTS];
  djinn_pool[ELEMENT_VENUS] =   0b1111111;
  djinn_pool[ELEMENT_MERCURY] = 0b1111111;
  djinn_pool[ELEMENT_MARS] =    0b1111111;
  djinn_pool[ELEMENT_JUPITER] = 0b1111111;

  uint32_t djinn_alloc[ALLIES*ELEMENTS];
  for (size_t i=0; i<ALLIES*ELEMENTS; ++i){
    djinn_alloc[i] = 0;
  }

  // assign each ally 3 djinn
  for (size_t i=0; i<ALLIES; ++i){
    assign_djinn(djinn_pool, djinn_alloc+ELEMENTS*i, 3);
    assign_djinn(djinn_pool, djinn_alloc+ELEMENTS*i, 3);
  }

  // now assign each ally 1 djinn
  for (size_t i=0; i<ALLIES; ++i){
    assign_djinn(djinn_pool, djinn_alloc+ELEMENTS*i, 1); 
  }

  // print the final result
  for (size_t i=0; i<ALLIES; ++i){
    printf("ally %ld djinn: %u  %u  %u  %u\n", i, djinn_alloc[ELEMENTS*i+0], djinn_alloc[ELEMENTS*i+1], djinn_alloc[ELEMENTS*i+2], djinn_alloc[ELEMENTS*i+3]);
    }

  // find game memory
  pid_t pid = find_pid();
  uint8_t* wram_ptr = find_wram(pid);
  
  // write random values to game memory
  for (size_t i=0; i<ALLIES*ELEMENTS; ++i){
    djinn_alloc[i] = byte_flipper(djinn_alloc[i]);
  }
  for (size_t i=0; i<ALLIES; ++i){
    printf("ally %ld djinn: %u  %u  %u  %u\n", i, djinn_alloc[ELEMENTS*i+0], djinn_alloc[ELEMENTS*i+1], djinn_alloc[ELEMENTS*i+2], djinn_alloc[ELEMENTS*i+3]);
  }

  write_djinn(pid, wram_ptr, djinn_alloc);

  return 0;
}
