##
Isaac Health. 
- Address: 8772c78, type: int16, offset: 629c78 seems to work? except if I edit, it changes to the desired value for a bit, then it changes back to max health..
- int32 does not work, int64 does not work, would be huge
- int8 would be too small

Try Isaac Psyenergy
- Address: 8772c7a, type: int16, offset: 629c7a
Garet psyenergy
- Address: 8772dc6, type: int16, offset: 629dc6
so the character object is 332 bytes.

This continues to be health outside of battle. When outside of battle CAN manually modify the game values. And remains good outside of battles.

The GBA has a 32-bit processor; pointers likely there! 4 bytes


these addresses are no longer valid after game reboot.. addresses are not relative in any kind of useful way.. BUT the "offset" searched value still works. So for the new game... 0x81c9c78 - 0x629c78 = 0x7ba0000

it does seem to use ASCII internally e.g. for Isaac's name. 
So Flint becomes 0x 46 6c 69 6e 74
Can't seem to find this... ? Not sure how the string searches are working. Not sure how to make it go into hex search mode. 

Isaac name is 0x7036740-0x6a0d000 = 
no... the name *is* there but better to use the name that's co-located with a bunch of other character info. 0x7036c40

Isaac earth resistance AFTER djinn 0x7036c8a - 0x6a0d000 = 


in this guide https://gamefaqs.gamespot.com/gba/468548-golden-sun/faqs/43776
the characters seem offset by 332 bytes also!

isaac class: 0x7036d69 - 0x6a0d000. when the guide says stored at 0x639.. 

assume that the game runs by just loading directly character data from disc to ram? so the representation is 1:1? 
so got a "base savesate memory" by 0x7036d69 (isaac class) subtract 0x639 (isaac class location in guide)
Then go forwards by 0x5e8 (to isaac's weapons). Success! 

items show up in inventory slot order. 

psyenergy does work. 

soft (A+B+START+SELECT) reset of the system does seem to leave the values in ram. and does reset to remove the extra psyenergies that I added, removes the extra stuff. 

can I find info on the creatures being fought? 
Mauler = 4D 61 75 6C
yes! Can find that string in memory. here.. 0x7067250. 
offset from the other enemy 0x7067100.. so the enemy structure has size 0x150 (dec 336)
is this just the same structure as for players but slightly larget to have room for a bigger name? meaning can enemies have items to use? equipment? 
TODO: look up known stats (from wiki) within the enemy structure to figure out the arrangement of that structure? 

ignore items - consumable and thus can't continue indefinitely. 

is there an enemy counter? enemy alive status? 

PINCE uses the first pid for the mednafen process. if mednafen pid is 1595...
get pid with `$pidof mednafen`
then `$cat /proc/1595/maps | grep heap`
shows the interesting stuff will start at 0x07f3b000 ! perfect.
the start of character data is *not* some constant offset from mednafen start between reboots of mednafen.
`https://www.systutorials.com/docs/linux/man/2-process_vm_readv/`



## TODO
- determine memory address auto finding. Is the variability within golden sun, or within mednafen? 
- demo utility: put up on a GUI live updating character stats, allows you to choose character at will. Nuklear gui? 
- find where the djinn recovery info is stored
- reverse engineer the monster structure
- find out where monster quantities are listed
- find out where active psyenergy option selection list is?


## State space
- Character stats & status.
- (NOT) items
- active psyenergy list for characters. some kind of pre-cached class lookup?
- active / possible summons. lookup based on djinn info. allow every possible one to allow for djinn standby on the same turn as summon cast!
- enemy status & status. 

## Action Space
- each character, any possible action (except items), include defense (want waiting to heal on lesser monsters to be a good strategy)
- how to train on action space since the options will change as a result of the state (class)
- the psyenergy list only shows what you HAVE NOW, not what you could have if you changed your class with the djinn you have. so this makes a good list of the currently available options. seems to be ordered as it is ordered in the battle menu (danger - some are not battle psyenergies). 


## Enemy Stuff
Ascii characters. Width is 332 bytes! First entry is the one on the left.
mednafen start..0x79ef000
enemy names are at 0x80497f8, and 0x8049944, and 0x8049a90
Can't seem to find an enemy totals counter.

Once the enemy is dead.... its memory is not cleared if the battle is still active.
HP gets set to zero though. 


There must somewhere be a "fainted" modifier - I can't seem to revive isaac simply by raising his health.

##
the djinn recovery counters get messed up by setting new djinn in battle. 
they can count down multiple djinn simultaneously.. so likely binary encoding of which djinn are in which status, it just gets processed? 
would mean recovery is a two byte value though 
and during battle I can have 3 djinn on standby and it said that in the 
there is some memory/order to how the djinn were put on standby in the first place - where is that stored?

hypothesis:
- FALSE: there is a "binary encoding of all djinn with 1 turn left" variable. tested. also doesn't work since djinn of multiple elements recover in sequence on the same character.
- FALSE: there is a "flint recovery status" variable that counts down matching the recovery timer count

experiment: set a bunch of djinn into standby order venus then jupiter. then do a jupiter summon, then do a venus summon. the jupiter djinn on isaac goes first, then the venus in order. on ivan, the jupiter djinn also go first, then the venus. so... hypothesis that cross-element recovery prioritization is not tracked/used until the recovery state is entered?

experiment: if transfer two djinn is their set order preserved? No! only preserved within a single character.

### Knowns
- in the character struct, djinn *within each element* represented as bits in a binary number (2 byte for GS1, 4 byte for TLA)
- djinn remember when they are placed in `standby` state, because they `recover` in sequence
- djinn in `recovery` state return to `set` at one djinn per *character* per turn, even if the djinn are of different elements. 
- djinn in `recovery` state cannot be traded to other characters
- djinn in `standby` can be traded to other characters.. but this does mess up their `recovery` queue ordering (reverts to natural order?)
- djinn on character X may be summoned (trigger `standby` -> `recovery`) by character Y

From Salanewt: 
> When a djinni is changed from Set to Standby/Recovery, it gets added to the recovery queue. In the GBA games, the recovery queue is stored at #02000254 and each entry takes up 4 bytes. An entry has the owner's ID, the djinni's ID (ranging from 0-14 in hex), element (0-3), and state (FF for standby, or else a duration for recovery). Immediately after this list is a number that keeps track of the djinn currently in the recovery queue.

That memory address is from the start of WRAM. WRAM is emulated RAM from the original device. But where is this emulated RAM within my emulator? 

According to [this forum post](https://gamefaqs.gamespot.com/boards/468548-golden-sun/36988652?page=8) `wram 0x2000250` is coins, and WRAM starts at `0x2000000`. Use this to find the start of wram? Currently my coins are showing up in PINCE at `0x6e7b480`. The current Isaac base location is `0x6e7b730`.. 688 bytes later! So Isaac is then 688 + 0x250 = 1280 bytes after the start of `WRAM 0x2000000`. At this address... (currently `0x6E7B230`) is the string Isaac! Save file name? 

Confirmed - those 4th byte is the status. Not cleared if not in use. `ff` if standby, `fe` during summon casting, some positive value (`02`, `03`, etc.) for turn countdown. 

## Battle State
byte `0x6aa8104` seems to be 1 when waiting for action input, 0 when not (battle progressing, smash A)
In this example the heap start is `0x0644d000`. That's 199508 bytes after Isaac (player 1) start.

Djinn are in the character struct are >1 byte values. thus the layout in memory is read in an inverted manner by C; 0x 00 00 03 00 becomes 186608. 