#include "golden_sun_utils.h"
#include "golden_sun.h"

uint8_t get_byte(pid_t pid, uint8_t* wram_ptr, int64_t offset){
  uint8_t result = 0;
  struct iovec local[1];
  local[0].iov_base = &result;
  local[0].iov_len = 1;

  struct iovec remote[1];
  remote[0].iov_base = wram_ptr + offset;
  remote[0].iov_len = 1;

  ssize_t n_read = process_vm_readv(pid, local, 1, remote, 1, 0);
  assert(n_read == 1);
  return result;
}  


// copy from game-wram Unit structure to the more concise game state structure ally
void export_copy_allies_single(Unit* unit, ExportAlly* send){
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


void export_copy_allies(pid_t pid, uint8_t* wram_ptr, Unit* unit, ExportAlly* send){
  for (uint8_t i=0; i<4; ++i){
    uint8_t char_id = party_order_to_character_id(pid, wram_ptr, i);
    export_copy_allies_single(unit+char_id, send+i);
  }
} 


void export_copy_enemies_single(Unit* unit, ExportEnemy* send){
  // level thru pp_base
  memcpy(send, &unit->level, offsetof(struct Unit, _unknown1) - offsetof(struct Unit, level) );

  // attack_base thru luck_base
  memcpy(&send->attack_base, &unit->attack_base, offsetof(struct Unit, _unknown2) - offsetof(struct Unit, attack_base));

  // power_venus_base thru luck_max
  memcpy(&send->elemental_base, &unit->elemental_base, offsetof(struct Unit, _unknown3) - offsetof(struct Unit, elemental_base));

  // elemental collision maxes only
  memcpy(&send->elemental_max, &unit->elemental_max, offsetof(struct Unit, psy) - offsetof(struct Unit, elemental_max));

  // class through end
  memcpy(&send->class, &unit->class, offsetof(struct Unit, _unknown10) - offsetof(struct Unit, class));
}
  
void export_copy_enemies(Unit* unit, ExportEnemy* send){
  for (size_t i=0; i<5; ++i){
    export_copy_enemies_single(unit+i, send+i);
  }
}

void print_data_ally(ExportAlly* ally){
  printf("Level: %u   Class: %u\n", ally->level, ally->class);
  printf("Health: %u / %u     PP: %u / %u\n", ally->health_current, ally->health_max, ally->pp_current, ally->pp_max);
}

// binary encoding stored djinn have an endian issue, fix it here
uint32_t djinn_to_x86(uint32_t x){
  uint8_t* byte = (uint8_t*) &x;
  uint32_t result = (byte[2]) + (byte[1] << 1) + (byte[0] << 2);
  return result;
}


void golden_sun_print_unknowns(Unit* unit){
  Unknown unknowns[9];

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
  unknowns[7].offset = 322;
  unknowns[7].length = 2;

  // end stuff no idea
  unknowns[8].offset = 328;
  unknowns[8].length = 4;


  for (size_t i=0; i<9; ++i){
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
    if (units[i].battle_status > 0){
      health += units[i].health_current;
    }
  }
  return health;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// determine party order
uint8_t party_order_to_character_id(pid_t pid, void* wram_ptr, uint8_t position){
  uint8_t characters[4];
  struct iovec local[1];
  local[0].iov_base = characters;
  local[0].iov_len = 4;
  
  struct iovec remote[1];
  remote[0].iov_base = wram_ptr + MEMORY_OFFSET_ALLIES_ORDER;
  remote[0].iov_len = 4;

  ssize_t n_read = process_vm_readv(pid, local, 1, remote, 1, 0);
  assert(n_read == 4);

  return characters[position];
}


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


////////////////////////////////////////////////////////////////////////////////////////////////

// TODO what..
uint8_t get_battle_menu(Battle_Action* actions){
  uint8_t result = 0x01;
  for (size_t i=0; i<BATTLE_ACTION_QUEUE_MAX_LENGTH; ++i){
    if (actions[i].unknown4 == 0x80){ // normal readiness case
      result &= 0x01;
    }
    else if (actions[i].action_type == 0x08){ // if character is alseep
      result &= 0x01;
    }
    else{
      result &= 0x00;
    }
  }
  return result;
}

uint8_t get_battle_menu_character_init(pid_t pid, uint8_t* wram_ptr){
  uint8_t result = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_L0);
  return result;
}

uint8_t get_battle_menu_character_id(pid_t pid, uint8_t* wram_ptr){
  // printf("wram = %p .... battle menu character id at %p\n", wram_ptr, wram_ptr+MEMORY_OFFSET_BATTLE_MENU_CHARACTER_ID);
  uint8_t result = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_CHARACTER_ID);
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
  //printf("looking for %s...\n", needle);

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

// 0x01983a08 this static memory address lists the moving address for wram start
uint8_t* find_wram(pid_t pid, uint64_t type){

  // read line in /proc/$pid/maps to figure out where the [heap] is
  char filename_map[64];
  sprintf(filename_map, "/proc/%d/maps", pid);
  FILE* fd_map = fopen(filename_map, "r");
  
  char line_raw[128];
  char line_prev[128];
  char* fgets_status;
  fgets_status = fgets(line_raw, sizeof(line_raw), fd_map);
  while (fgets_status != NULL){
    if (strstr(line_raw, "[heap]") != NULL){
      printf("heap line in /proc/[pid]/maps: %s", line_raw);
      printf("  the previous line is %s", line_prev);
      break;
    }
    memcpy(line_prev, line_raw, 128);
    fgets_status = fgets(line_raw, sizeof(line_raw), fd_map);
  }
  assert(fgets_status != NULL);
  
  char* line_working = line_prev;
  char* mem_ref_str = strtok_r(line_working, "-", &line_working);
  uint64_t mem_ref = strtol(mem_ref_str, NULL, 16);
  fclose(fd_map);

  uint64_t mem_adr = 0;
  uint8_t* mem_adr_ptr = (uint8_t*) &mem_adr;

  struct iovec local[1];
  local[0].iov_base = mem_adr_ptr;
  local[0].iov_len = 4;
  struct iovec remote[1];
  remote[0].iov_base = (uint8_t*) type + mem_ref; // TODO magic value
  remote[0].iov_len = 4;
  ssize_t n_read = process_vm_readv(pid, local, 1, remote, 1, 0);
  assert(n_read == 4);

  // printf("wram prediction: 0x%lx\n   ", mem_adr); 

  uint8_t* game_ptr = (uint8_t*) mem_adr;
  printf("WRAM located at %p\n", game_ptr);

  return game_ptr;
}


// deconfuse djinn from wram, write it in a memory blob to send
void get_djinn(pid_t pid, uint8_t* wram_ptr, Unit* allies, Export_Djinn_List* export_djinn){

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
  /*
  for(size_t i=0; i<ALLIES; ++i){
    Unit* ally = allies+i;
    Export_Djinn_List* list = export_djinn+i;
    printf("Djinn Status for %s, they have %u djinn\n", ally->name, list->quantity);
    for(size_t d=0; d<list->quantity; ++d){
      printf("  djinn:%u   element:%u   status:%u\n", list->djinn[d].id, list->djinn[d].element, list->djinn[d].status);
    }
  }
  */

}


void get_battle_action_queue(pid_t pid, uint8_t* wram_ptr, Battle_Action* actions){
  struct iovec local[1];
  local[0].iov_base = actions;
  local[0].iov_len = BATTLE_ACTION_QUEUE_MAX_LENGTH*sizeof(Battle_Action);

  struct iovec remote[1];
  remote[0].iov_base = wram_ptr + MEMORY_OFFSET_BATTLE_ACTION_QUEUE;
  remote[0].iov_len = local[0].iov_len;

  ssize_t n_read = process_vm_readv(pid, local, 1, remote, 1, 0);
  assert(n_read == remote[0].iov_len);
}


void get_battle_menu_navigation(pid_t pid, uint8_t* wram_ptr, uint8_t* wram_ptr_chip, Battle_Menu_Navigation* info){
  info->menu_active = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU);
  info->character = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_CHARACTER_ID);
  info->menu_l0 = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_L0);
  info->menu_l1 = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_L1);
  info->menu_l2 = get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_L2_MINOR) + 
                  get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU_L2_MAJOR);
  info->menu_l2_djinn = get_byte(pid, wram_ptr_chip, MEMORY_OFFSET_BATTLE_MENU_DJINN_MINOR) +
                        get_byte(pid, wram_ptr_chip, MEMORY_OFFSET_BATTLE_MENU_DJINN_MAJOR);
  info->target = get_byte(pid, wram_ptr_chip, MEMORY_OFFSET_BATTLE_MENU_TARGET);

}


// TODO support TLA Djinn
const char* djinn_get_name(Export_Djinn_Item djinn){

  // index with (num_per_element)*element + num_within_element
  const char* djinn_names[] = {"Flint", "Granite", "Quartz", "Vine", "Sap", "Ground", "Bane", 
                               "Fizz", "Sleet", "Mist", "Spritz", "Hail", "Tonic", "Dew", 
                               "Forge", "Fever", "Corona", "Scorch", "Ember", "Flash", "Torch",
                               "Gust", "Breeze", "Zephyr", "Smog", "Kite", "Squall", "Luff"}; 
  uint32_t id = djinn.id;
  uint32_t djinn_per_element = 7;
  
  // convert binary id into sequential index
  uint32_t index = 0;
  while (id > 1){
    id = id >> 1;
    index += 1;
  }
  
  // adjust index for the elements
  index += djinn_per_element * djinn.element;
  
  // lookup and return the string
  return djinn_names[index]; 
}


const char* psyenergy_get_name(Psyenergy psy){
  
  // TODO some colorization based on element

  // psyenergy_names index based on psy.spell
  char* pn[256];
  pn[0x00] = "_";
  pn[0x01] = "Attack";
  pn[0x02] = "Defend";
  
  pn[0x03] = "Quake";
  pn[0x04] = "Earthquake";
  pn[0x05] = "Quake Sphere";
  pn[0x06] = "Spire";
  pn[0x07] = "Clay Spire";
  pn[0x08] = "Stone Spire";
  pn[0x09] = "Gaia";
  pn[0x0A] = "Mother Gaia";
  pn[0x0B] = "Grand Gaia";
  pn[0x0C] = "Growth";
  pn[0x0D] = "Mad Growth";
  pn[0x0E] = "Wild Growth";
  pn[0x0F] = "Thorn";
  pn[0x10] = "Briar";
  pn[0x11] = "Nettle";
  
  pn[0x18] = "Frost";
  pn[0x19] = "Tundra";
  pn[0x1A] = "Glaceir";
  pn[0x1B] = "Ice";
  pn[0x1C] = "Ice Horn";
  pn[0x1D] = "Ice Missile";
  pn[0x1E] = "Prism";
  pn[0x1F] = "Hail Prism";
  pn[0x20] = "Freeze Prism";
  pn[0x21] = "Douse";
  pn[0x22] = "Drench";
  pn[0x23] = "Deluge";
  pn[0x24] = "Froth";
  pn[0x25] = "Froth Sphere";
  pn[0x26] = "Froth Spiral";

  pn[0x2D] = "Flare";
  pn[0x2E] = "Flare Wall";
  pn[0x2F] = "Flare Storm";
  pn[0x30] = "Fire";
  pn[0x31] = "Fireball";
  pn[0x32] = "Inferno";
  pn[0x33] = "Volcano";
  pn[0x34] = "Eruption";
  pn[0x35] = "Proclasm";
  pn[0x36] = "Blast";
  pn[0x37] = "Mad Blast";
  pn[0x38] = "Fiery Blast";
  pn[0x39] = "Blast";
  pn[0x3A] = "Nova";
  pn[0x3B] = "Supernova";
  
  
  pn[0x42] = "Bolt";
  pn[0x43] = "Flash Bolt";
  pn[0x44] = "Blue Bolt";
  pn[0x45] = "Ray";
  pn[0x46] = "Storm Ray";
  pn[0x47] = "Destruct Ray";
  pn[0x48] = "Plasma";
  pn[0x49] = "Shine Plasma";
  pn[0x4A] = "Spark Plasma";
  pn[0x4B] = "Slash";
  pn[0x4C] = "Wind Slash";
  pn[0x4D] = "Sonic Slash";
  pn[0x4E] = "Whirlwind";
  pn[0x4F] = "Tornado";
  pn[0x50] = "Tempest";

  pn[0x5A] = "Cure";
  pn[0x5B] = "Cure Well";
  pn[0x5C] = "Potent Cure";
  pn[0x5D] = "Ply";
  pn[0x5E] = "Ply Well";
  pn[0x5F] = "Pure Ply";
  pn[0x60] = "Wish";
  pn[0x61] = "Wish Well";
  pn[0x62] = "Pure Wish";
  pn[0x63] = "Cure Poison";
  pn[0x64] = "Restore";
  pn[0x65] = "Revive";
  pn[0x66] = "Impact";
  pn[0x67] = "High Impact";
  pn[0x68] = "Dull";

  pn[0x69] = "Blunt";
  pn[0x6A] = "Guard";
  pn[0x6B] = "Protect";
  pn[0x6C] = "Impair";
  pn[0x6D] = "Debilitate";
  pn[0x6E] = "Ward";
  pn[0x6F] = "Resist";
  pn[0x70] = "Weaken";
  pn[0x71] = "Enfeeble";
  pn[0x72] = "Taint";
  pn[0x73] = "Poison";
  pn[0x74] = "Delude";
  pn[0x75] = "Confuse";
  pn[0x76] = "Charm";
  pn[0x77] = "Paralyze";
  
  pn[0x78] = "Sleep"; 
  pn[0x79] = "Bind"; 
  pn[0x7A] = "Haunt"; 
  pn[0x7B] = "Curse"; 
  pn[0x7C] = "Condem"; 
  pn[0x7D] = "Drain"; 
  pn[0x7E] = "Psy Drain"; 
  pn[0x7F] = "Break"; 
  pn[0x80] = "Regenerate"; 
  pn[0x81] = "Reflect"; 

  pn[0x8C] = "Move"; 
  pn[0x8D] = "Mind Read"; 
  pn[0x8E] = "Force"; 
  pn[0x8F] = "Lift"; 
  pn[0x90] = "Reveal"; 
  pn[0x91] = "Halt"; 
  pn[0x92] = "Cloak"; 
  pn[0x93] = "Carry"; 
  pn[0x94] = "Catch"; 
  pn[0x95] = "Retreat"; 
  pn[0x96] = "Avoid"; 

  pn[0xA0] = "Dragon Cloud"; 
  pn[0xA1] = "Demon Night"; 
  pn[0xA2] = "Helm Splitter"; 
  pn[0xA3] = "Quick Strike"; 
  pn[0xA4] = "Rockfall"; 
  pn[0xA5] = "Rockslide"; 
  pn[0xA6] = "Avalanche"; 
  pn[0xA7] = "Lava Shower"; 
  pn[0xA8] = "Molten Bath"; 
  pn[0xA9] = "Magama Storm"; 
  pn[0xAA] = "Demon Spear"; 

  pn[0xAB] = "Angel Spear"; 
  pn[0xAC] = "Guardian"; 
  pn[0xAD] = "Protector"; 
  pn[0xAE] = "Magic Shell"; 
  pn[0xAF] = "Magic Shield"; 
  pn[0xB0] = "Death Plunge"; 
  pn[0xB1] = "Shuriken"; 
  pn[0xB2] = "Annihilation"; 
  pn[0xB3] = "Punji"; 
  pn[0xB4] = "Punji Trap"; 

  pn[0xB5] = "Punji Strike"; 
  pn[0xB6] = "Fire Bomb"; 
  pn[0xB7] = "Cluster Bomb"; 
  pn[0xB8] = "Carpet Bomb"; 
  pn[0xB9] = "Gale"; 
  pn[0xBA] = "Typhoon"; 
  pn[0xBB] = "Hurricane"; 
  pn[0xBC] = "Thunderclap";

  pn[0xBD] = "Thunderbolt"; 
  pn[0xBE] = "Thunderstorm"; 
  pn[0xBF] = "Mist"; 
  pn[0xC0] = "Ragnarok"; 
  pn[0xC1] = "Cutting Edge"; 
  pn[0xC2] = "Heat Wave"; 
  pn[0xC3] = "Astral Blast"; 
  pn[0xC4] = "Planet Diver"; 
  
  
  if (pn[psy.spell] == NULL){
    psy.spell = 0;
  }

  return pn[psy.spell];
  
}
