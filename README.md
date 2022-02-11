# Golden Sun Hacking
Having some fun with the GBA Game [Golden Sun](https://en.wikipedia.org/wiki/Golden_Sun). Testing on the first game for now.

The unit struct is 332 bytes in both GS1 and TLA. 

## Locating in Memory
Many (all?) significant values are stored at static memory addresses within `WRAM`, the GBA's original memory. So the challenge is to locate where the start of `WRAM` is within the emulator's heap. I use character names and the size of the unit struct: find where in memory the string "Isaac" starts 332 bytes before the string "Garet". The start of "Isaac" is 1280 bytes after the start of `WRAM 0x2000000`, which seems to be the commonly used memory coordinate system. From here, several areas of memory are accessed in order to extract a full battle state:
- Djinn recovery/standby queue. Queue length at `+0x354`, queue of 4-byte elements starting at `+0x254`.
- Allied units. An array of four 332-byte `Unit` structures starting at `+0x500`
- Enemy units. An array of five (?) 332-byte `Unit` structures starting at `+0x30878`.
- Battle UI requesting human input. A single byte at `+0x31054`.

# TODO
- Constant background monitoring
  - Automatically detect process pid. `$pidof mednafen`
  - service: provide game state on request
  - service: advance battle on request
- Process djinn stats to represent true action space?
- Process psyenergy list to remove utiltiy-only psyenergies
- Condense battle stats structs to remove empty data
- Check data structures against TLA. 
