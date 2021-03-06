# Golden Sun Hacking
Having some fun with the GBA Game [Golden Sun](https://en.wikipedia.org/wiki/Golden_Sun). Testing on the first game for now.

The unit struct is 332 bytes in both GS1 and TLA. 

# 
## Memory
The GBA allocates memory mapped blocks to different regions:
- ROM starts at `0x08000000`. So `0x12` in the ROM is found in GBA memory at `0x08000012`.
- EWRAM starts at `0x02000000`.
- IRAM starts at `0x03000000`. 

## Locating in Memory
Many (all?) significant values are stored at static memory addresses within `WRAM`, the GBA's original memory. So the challenge is to locate where the start of `WRAM` is within the emulator's heap. I use character names and the size of the unit struct: find where in memory the string "Isaac" starts 332 bytes before the string "Garet". The start of "Isaac" is 1280 bytes after the start of `WRAM 0x2000000`, which seems to be the commonly used memory coordinate system. From here, several areas of memory are accessed in order to extract a full battle state:
- Djinn recovery/standby queue. Queue length at `+0x354`, queue of 4-byte elements starting at `+0x254`.
- Order of allied units. A four byte array starting at `+0x438` (array index is position, value is character; Isaac=0, Garet=1, Ivan=2, Mia=3).
- Allied units. An array of four 332-byte `Unit` structures starting at `+0x500`
- Enemy units. An array of five (?) 332-byte `Unit` structures starting at `+0x30878`.
- Battle UI requesting human input. A single byte at `+0x31054`.
- Battle menu character init single byte at `+0x2094`. 65 if ready, 0 if not (selecting djinn or psyenergy or something). 65 at attack/flee screen as well. Moves to 4 in the middle of battle in some places.. so this probably isn't it? 

Some battle menu navigation values are stored at static memory addresses within `IRAM`. 
- Djinn menu navigation. Add two bytes together.
- Target selection. Counts up from 0 regardless if selecting enemy or ally.

# TODO
- Constant background monitoring
  - service: provide game state on request
  - service: advance battle on request
- Process djinn stats to represent true action space?
- Process psyenergy list to remove utiltiy-only psyenergies
- Condense battle stats structs to remove empty data
- Check data structures against TLA. Apparently there are changes..

# State Space
## Allies
- Everything
- REMOVE: unknowns except battle status, items, djinn, experience, name
## Djinn
- per character structure; list of djinn, ordered as they will appear in the menu
- each djinn says element, id, status_counter (0 for set, 0xff..0xf0 for standby, 0x01...0x04 for recovery, etc.).
## Enemy
- Everything
- REMOVE: unknowns except battle status, items, djinn, experience, name
## Battle Menu Navigation
- A bunch of uint8's...


# Implementation
A virtual machine (wow.. antiX seems fast!) runs the emulator, which runs the game. A ZeroMQ bridge puts the user teleop interface (and later bot) outside the virtual machine.

## Dependencies - Virtual Machine
- mednafen 1.2.9 (emulator.. others likely possible with modification)
- build-essential
- libxtst-dev (for -lXtst). will bring in other x11-dev dependencies
- libzmq3-dev (for zmq compile libraries)

Setup network between host and virtual machine. I use VirtualBox's `NAT` interface (isolated single vm-to-host). Port forwarding setup is from host IP address (on the normal local network.. 192.168...) port 5555 to client internal reported IP address (10.0.2.15 for some reason) port 5555. 

## Dependencies - Host Machine
- `pip install pyzmq` for comms
- `pip install pygame` for teleop training keyboard input

