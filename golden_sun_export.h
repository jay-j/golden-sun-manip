// repackage data to make it a cleaner input set
#ifndef GOLDEN_SUN_EXPORT_H
#define GOLDEN_SUN_EXPORT_H

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

  uint16_t power_venus_base;
  uint16_t resistance_venus_base;
  uint16_t power_mercury_base;
  uint16_t resistance_mercury_base;
  uint16_t power_mars_base;
  uint16_t resistance_mars_base;
  uint16_t power_jupiter_base;
  uint16_t resistance_jupiter_base;
  
  uint16_t health_max;
  uint16_t pp_max;

  uint16_t health_current;
  uint16_t pp_current;

  uint16_t attack_max;
  uint16_t defense_max;
  uint16_t agility_max;
  uint8_t luck_max;

  uint16_t power_venus_max; 
  uint16_t resistance_venus_max;
  uint16_t power_mercury_max;
  uint16_t resistance_mercury_max;
  uint16_t power_mars_max;
  uint16_t resistance_mars_max;
  uint16_t power_jupiter_max;
  uint16_t resistance_jupiter_max;

  Psyenergy psy[32];

  uint8_t class;
  
  // all the following are battle status stuff
  uint8_t battle_status;     // 00 if dead, 01 if alive enemy, 02 if alive ally
  uint8_t defending;         // defending or granite djinn
  uint8_t boost_power_venus;  // qty is how many djinn
  uint8_t boost_power_mercury; // qty is how many djinn
  uint8_t boost_power_mars;   // qty is how many djinn
  uint8_t boost_power_jupiter; // qty is how many djinn
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


// copy from game-wram Unit structure to the more concise game state structure ally
void export_copy_ally(Unit* unit, ExportAlly* send){
  // level thru pp_base
  memcpy(send, &unit->level, offsetof(struct Unit, _unknown1) - offsetof(struct Unit, level) );

  // attack_base thru luck_base
  memcpy(&send->attack_base, &unit->attack_base, offsetof(struct Unit, _unknown2) - offsetof(struct Unit, attack_base));

  // power_venus_base thru luck_max
  memcpy(&send->power_venus_base, &unit->power_venus_base, offsetof(struct Unit, _unknown3) - offsetof(struct Unit, power_venus_base));

  // power_venus_max thru psyenergy
  memcpy(&send->power_venus_max, &unit->power_venus_max, offsetof(struct Unit, item) - offsetof(struct Unit, power_venus_max));

  // class through end
  memcpy(&send->class, &unit->class, offsetof(struct Unit, _unknown10) - offsetof(struct Unit, class));
}

// chararacter 1
//      djinn 1
//            element
//            status
//            id
//     djinn 2
//

typedef struct Export_Djinn_Item {
  uint8_t element;
  uint8_t status; // 0=set
  uint32_t id; // binary encoding of the djinn id TODO check size
} Export_Djinn_Item;

// djinn element order is venus, mercury, mars, jupiter
typedef struct Export_Djinn_List{
  uint16_t quantity;
  Export_Djinn_Item djinn[9];
} Export_Djinn_List;

typedef Export_Djinn_List Export_Djinn[4];

uint32_t djinn_to_x86(uint32_t x){
  // reverse order of bytes
  // same order of bits within the byte
  uint8_t* byte = (uint8_t*) &x;
  uint32_t result = (byte[2]) + (byte[1] << 1) + (byte[0] << 2);
  return result;
}

// deconfuse djinn from wram, write it in a memory blob to send
void export_djinn(pid_t pid, uint8_t* wram_ptr, Unit* allies, Export_Djinn export_djinn){

  // look at characters to figure out how many djinn they have
  for (size_t i=0; i<ALLIES; ++i){
    Unit* ally = allies + i;
    printf("Processing Djinn for %s\n", ally->name);
    printf(" (venus: %u  ", ally->djinn_venus_have);
    printf("mercury: %u  ", ally->djinn_mercury_have);
    printf("mars: %u  ", ally->djinn_mars_have);
    printf("jupiter: %u)\n", ally->djinn_jupiter_have);

    export_djinn[i].quantity = 0;

    // for each element - this drives order!
    for (uint8_t e=0; e<4; ++e){
      uint8_t qty = *( &ally->djinn_venus_qty_total + e);
      uint32_t this_element = *( &ally->djinn_venus_have + e);
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
          printf("    found element %u, status %u, id %u\n", e, 0, this_djinn);
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
  printf("copied memory with djinn queue data. there are %u in queue\n", queue_length);

  // process the recovery queue - figure out which character and element and djinn, and set the recovery status in my structure 
  for (uint32_t q=0; q<queue_length; ++q){
    printf("Queue!  element:%u  djinn:%u  owner:%u  status:%u ", queue[q].element, queue[q].djinn, queue[q].owner, queue[q].status);

    uint32_t djinn_id = 0x1;
    djinn_id = djinn_id << queue[q].djinn;

    printf(" djinn_id = %u\n", djinn_id);
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


  for(size_t i=0; i<ALLIES; ++i){
    Unit* ally = allies+i;
    Export_Djinn_List* list = export_djinn+i;

    printf("Djinn Status for %s, they have %u djinn\n", ally->name, list->quantity);

    for(size_t d=0; d<list->quantity; ++d){
      printf("  djinn:%u   element:%u   status:%u\n", list->djinn[d].id, list->djinn[d].element, list->djinn[d].status);
    }
   
  }

}

#pragma pack(pop)

#endif // header guards
