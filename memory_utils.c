// anything that deals with direct memory access to the game process
#include "memory_utils.h"

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
      uint32_t* dj = &units[i].djinn_have[e];
      *dj = djinn_to_x86(*dj);
      dj = &units[i].djinn_set[e];
      *dj = djinn_to_x86(*dj);
    }
  }
}

uint8_t get_battle_menu(pid_t pid, uint8_t* wram_ptr){
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

////////////////////////////////////////////////////////////////////////////////////////////////

pid_t find_pid(){
  char pidline[1024];
  char* pid_str;  

  FILE* fd = popen("pidof mednafen", "r");
  char* result = fgets(pidline, 1024, fd);
  assert(result != NULL);
  pid_str = strtok(pidline, " ");

  pid_t pid = atoi(pid_str); // ASSUME it is the first process with that name
  assert(pid != 0);
  printf("got mednafen process pid: %d\n", pid);

  pclose(fd);
  return pid;
}


char* strstr_n(char* haystack_start, size_t haystack_n, char* needle, size_t needle_n){
  printf("looking for %s...\n", needle);

  char* haystack = haystack_start; // current
  char* haystack_end = haystack_start + haystack_n;
  int done = 0;
  while (done == 0){
    // move the haystack to point at the first occurance of the first letter of the needle
    haystack = memchr(haystack, (int) needle[0], haystack_end - haystack);
    if (haystack == NULL){
      break;
    }

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
      printf("heap line in /proc/[pid]/maps: %s", line_raw);
      break;
    }
    fgets_status = fgets(line_raw, sizeof(line_raw), fd_map);
  }
  assert(fgets_status != NULL);

  char* line_working = line_raw;
  char* heap_start_str = strtok_r(line_working, "-", &line_working);
  char* heap_end_str = strtok_r(line_working, " ", &line_working);
  printf("  the heap goes from 0x%s to 0x%s\n", heap_start_str, heap_end_str);

  uint64_t heap_start = strtol(heap_start_str, NULL, 16);
  uint64_t heap_end = strtol(heap_end_str, NULL, 16);
  uint64_t heap_length = heap_end - heap_start;
  printf("  %ld to %ld  (%ld)\n", heap_start, heap_end, heap_length);
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
  char* solution = NULL;
  //while (search_complete == 0){
  while (buff_end - buff > 800){
    found_p1 = strstr_n(buff, buff_end - buff, NAME_P1, 5);
    if (found_p1 == NULL){
      break;
    }
    assert(found_p1 != NULL);
    printf("Found %s at %p\n", NAME_P1, found_p1);

    found_p2 = strstr_n(buff, buff_end - buff, NAME_P2, 5);
    printf("Found %s at %p\n", NAME_P2, found_p2);
    assert(found_p2 != NULL);

    if (found_p2 - found_p1 == sizeof(Unit)){
      printf("search complete!\n");
      search_complete += 1;
      solution = found_p1;
    }
    //else{
      buff = found_p1 + 1;
    //}
  }

  assert(solution != NULL); // remember to sudo or this will show up
  printf("found %d solutions\n", search_complete);

  size_t offset = solution - buff_full; // heap plus this many bytes is where the character data is

  free(buff_full);

  uint8_t* game_ptr = target_ptr + offset - 1280; // 1280 is offset in bytes from the character to start of WRAM in GS1
  printf("WRAM located at %p\n", game_ptr);
  //print_djinn_aid(game_ptr);

  return game_ptr;
}


// deconfuse djinn from wram, write it in a memory blob to send
void get_djinn(pid_t pid, uint8_t* wram_ptr, Unit* allies, Export_Djinn export_djinn){

  // look at characters to figure out how many djinn they have
  for (size_t i=0; i<ALLIES; ++i){
    Unit* ally = allies + i;
    // printf("Processing Djinn for %s\n", ally->name);
    // printf(" (venus: %u  ", ally->djinn_venus_have);
    // printf("mercury: %u  ", ally->djinn_mercury_have);
    // printf("mars: %u  ", ally->djinn_mars_have);
    // printf("jupiter: %u)\n", ally->djinn_jupiter_have);

    export_djinn[i].quantity = 0;

    // for each element - this drives order!
    for (uint8_t e=0; e<ELEMENTS; ++e){
      uint8_t qty = ally->djinn_qty_total[e];
      uint32_t this_element = ally->djinn_have[e];
      //printf("  element %u expect quantity %u\n", e, qty);

      // for each djinn. within elements these are sorted by the byte order of their id
      // searching for options in the correct order means they are pre-placed in my list in the right order
      size_t qty_found = 0; // within this element
      size_t d=0; 
      while( qty_found<qty ){
        // try and add this djinn. 
        uint32_t this_djinn = 0x1 << d;
        if ((this_element & this_djinn) > 0){
          export_djinn[i].djinn[export_djinn[i].quantity + qty_found].element = e;
          export_djinn[i].djinn[export_djinn[i].quantity + qty_found].status = 0; // detect standby/recovery later
          export_djinn[i].djinn[export_djinn[i].quantity + qty_found].id = this_djinn;
          ++qty_found;
          //printf("    found element %u, status %u, id %u\n", e, 0, this_djinn);
        }
        ++d;
        assert( d < 31);
      } 
      export_djinn[i].quantity += qty; // store how many total djinn this player has
    }    
  }
    
  // grab a copy of the djinn recovery queue 
  Djinn_Queue queue;
  uint16_t queue_length;
  struct iovec local[2];
  local[0].iov_base = queue; 
  local[0].iov_len = DJINN_QUEUE_MAX_LENGTH*sizeof(Djinn_Queue_Item);
  local[1].iov_base = &queue_length;
  local[1].iov_len = sizeof(queue_length);

  struct iovec remote[1];
  remote[0].iov_base = wram_ptr + MEMORY_OFFSET_DJINN_QUEUE;
  remote[0].iov_len = local[0].iov_len + local[1].iov_len;

  ssize_t n_read = process_vm_readv(pid, local, 2, remote, 1, 0);
  assert(n_read == remote[0].iov_len);
  //printf("copied memory with djinn queue data. there are %u in queue\n", queue_length);

  // process the recovery queue - figure out which character and element and djinn, and set the recovery status in my structure 
  for (uint32_t q=0; q<queue_length; ++q){
    //printf("Queue!  element:%u  djinn:%u  owner:%u  status:%u\n", queue[q].element, queue[q].djinn, queue[q].owner, queue[q].status);

    uint32_t djinn_id = 0x1;
    djinn_id = djinn_id << queue[q].djinn;

    // find that djinn id in the player+element combo, then store the status data
    for (uint8_t d=0; d<export_djinn[queue[q].owner].quantity; ++d){
      if (djinn_id == export_djinn[queue[q].owner].djinn[d].id){
        if (queue[q].element == export_djinn[queue[q].owner].djinn[d].element){
          export_djinn[queue[q].owner].djinn[d].status = queue[q].status;
          break;
        }
      }
    }
  }

  // debug only - print the results!
  for(size_t i=0; i<ALLIES; ++i){
    Unit* ally = allies+i;
    Export_Djinn_List* list = export_djinn+i;
    printf("Djinn Status for %s, they have %u djinn\n", ally->name, list->quantity);
    for(size_t d=0; d<list->quantity; ++d){
      printf("  djinn:%u   element:%u   status:%u\n", list->djinn[d].id, list->djinn[d].element, list->djinn[d].status);
    }
  }

}
