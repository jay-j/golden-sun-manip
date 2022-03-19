# Game Memory
- Commonly uses `uint8_t`, `uint16_t`, `uint32_t` representations. 
- Does use ASCII for strings. E.g. for "Isaac"
- The GBA processor is 32-bit. So looking for 32-bit pointers. 
- Allied character struct (332 byte) is loaded directly from the savefile into wram; so all the representations and offsets here seem to be correct: https://gamefaqs.gamespot.com/gba/468548-golden-sun/faqs/43776
- Items appear in inventory slot order. 


# Memory Tools
PINCE uses the first pid for the mednafen process. if mednafen pid is 1595...
get pid with `$pidof mednafen`
then `$cat /proc/1595/maps | grep heap`
shows the interesting stuff will start at 0x07f3b000 ! perfect.
the start of character data is *not* some constant offset from mednafen start between reboots of mednafen.
`https://www.systutorials.com/docs/linux/man/2-process_vm_readv/`

# Finding WRAM
- Important addresses seem to be constant in `wram` but not within the emulator heap; have to find the `wram` start upon every emulator startup. Soft reset (A+B+START+SELECT) reset of the system does seem to leave the values in ram. and does reset to remove the extra psyenergies that I added, removes the extra stuff. 

what if there is a big/little endian discrepancy? So the number in raw memory would appear backwards?
yes! there is *something* here searching like this. found three values. but those don't seem to lead anywhere (could be offset from the true pointer..)

## issue with memory
finding memory approach doesn't work in the virtual environment. it seems that I need the *last* copy of Isaac-Garet 332 bytes? Vs. the first? And there are many copies of that unit structure throughout the game's memory - how to find the correct one?
If I use the last one, then everything else does seem to fall into place...


# TODO
- determine memory address auto finding. Is the variability within golden sun, or within mednafen? 
- demo utility: put up on a GUI live updating character stats, allows you to choose character at will. Nuklear gui? 
- find where the djinn recovery info is stored
- reverse engineer the monster structure
- find out where monster quantities are listed
- find out where active psyenergy option selection list is?


## Enemy Stuff
Ascii characters. Width is 332 bytes! First entry is the one on the left.
mednafen start..0x79ef000
enemy names are at 0x80497f8, and 0x8049944, and 0x8049a90
Can't seem to find an enemy totals counter.

Once the enemy is dead.... its memory is not cleared if the battle is still active.
HP gets set to zero though. 


There must somewhere be a "fainted" modifier - I can't seem to revive isaac simply by raising his health.

# Djinn
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

# Battle State
byte `0x6aa8104` seems to be 1 when waiting for action input, 0 when not (battle progressing, smash A)
In this example the heap start is `0x0644d000`. That's 199508 bytes after Isaac (player 1) start.

Djinn are in the character struct are >1 byte values. thus the layout in memory is read in an inverted manner by C; 0x 00 00 03 00 becomes 186608. 

wram `0x6c59cb0`. Then `0x6c8f8b8` and `0x6c8f8bc` show horizontal big selection in the battle menu (run vs flee vs status.. and attack thru defend etc.). 

And they are crazy values during other pa
if ((XQueryPointer (display, root_window, &root_ret,
&child_ret, &root_x, &rorts of the battle? Yes, but not good enough - still just zero when enemies initially show up. 

When enemies show up, they do appear in memory first! So the enemy memory structure shows how many times A has to be pushed to advance to start the battle. 

Push A twice in between battles... 

End of battle.. watch the enemy status. After sum(enemy status) goes to zero, push A once to get past 'party goes down' text, that's it. Then are out of battle, push A twice to restart. 

##  Actions
Looking for start points
With mia first in the party, looking at her psyenergy command gives 0x79294c0
there appears to be a 16-byte structure (increment +0x10).. that is populated in order of party members, but then after the final party member gets re-ordered according to who is going to act first

the structure is all entities in the battle - both enemies AND players

look two bytes before that. 0x79294be
00 = attack
01 = psyenergy
02 = item
03 = defend
04 = 
05 = djinn

the character id part is the start of the queue structure?
all these are set ff
as you set each character they move from ff to the character_id (e.g. mia=03 regardless of party)

in battle downed characters have their array set to ff

have to hit 'a' once to make the first character's action be set to ff; so grab the battle status before this happens

if a character is asleep, then the queue isn't zero'd; instead that character still gets an entry. if a character is downed, then they don't get any entry in the action queue

# Sending Key Press Commands
https://tronche.com/gui/x/xlib/event-handling/XSendEvent.html
https://stackoverflow.com/questions/1262310/simulate-keypress-in-a-linux-c-console-application
http://t-sato.in.coocan.jp/xvkbd/events.html
- seems like from this I could be able to select a specific window to send the keypress event to

https://stackoverflow.com/questions/68852436/how-xtestfakekeyevent-combines-events
- has some code for using the XSendEvent() interface
https://stackoverflow.com/questions/34704200/xsendevent-not-working


seems the problem will be identifying the true window
https://ubuntuforums.org/archive/index.php/t-570702.html

pass through will also be an issue - how to send keypresses to my application for logging purposes, but end up with keypresses in the game also?

use xquerypointer() to find the window id under the pointer
- maybe setup a state machine? run program, user hits keyboard button to lock onto the game window under the pointer and start things. Then user hits an end button to stop playing battle mode? 
- also record own focus, in case need to restore focus back to the terminal application window?

https://groups.google.com/g/comp.windows.x/c/Ac2IKoCh1R8
- Window is not the clear descriptor that I think it is. And programs may simply choose to ignore events sent by other programs. 

Looked at a bunch of memory addresses. Seems there's nothing within GS (at least on initial checks) that I can activate to send keyboard pushes just by changing memory states.

Set *all* the identified values at the same time (during game pause?) also not successful. 


## pygame keys
https://www.pygame.org/docs/ref/key.html


https://github.com/NotFx/Bizhawk-GS-TBS-Utility-Script




# Challenge Setup

## Items
Concept: Randomize items (from a set of ~endgame options) and djinn (all djinn, randomize distribution)!
Use controlled memory overrides to do this. 

*Need* to have some PP-regenerating items to keep the action space rich.
In GS1, these are all artifacts (only one available within the game). Guarantee every one is given to the party, randomize who gets what to the best extent allowable. 
| Item Name | Equip Slot | Regeneration | Isaac | Garet | Ivan | Mia |
|-----------|------------|--------------|-------|-------|------|-----|
| Magical Cassock | body | 2 PP/turn |  |   |  x |  x |
| Lucky Cap | head | 2 PP/turn | x | x | x | x |
| Mythril Circlet | head | 3 PP/turn| | | x | x |
| Thunder Crown (cursed) | head | 4 PP/turn | x | x | x | x |

Cursed items applicable for Isaac & Garret:
- Demon Axe (considered inferior...)
- Wicked Mace
- Muramasa
- Demon Mail (meh)

Track a pool of available equipment. Randomly select the PP-regen item. Then randomly select the rest of the equipment, using filters to downselect (build a list that meet a filter, then pick an  integer) repeat.

## Djinn Randomizer
Bi element selections at maximum. Each character has a "4 slot" and "3 slot" to randomly choose an element. Then within those slots they randomly choose djinn from the appropriate element. This does give access to all classes in GS1.

Implement.. keep track of which djinn are which element and have been allocated. Go through characters one by one. Randomly select an element, then get 4 djinn. Then randomly select and element and get 3 djinn. But need to be sure that the element selection is allowed.

# Machine Learning Environment / Approach
Using the Tensorflow and Keras-RL library and "OpenAI Gym". For reinforcement learning applications, it seems like TensorFlow is the recommended option (vs. PyTorch).
[Video Tutorial Link](https://www.youtube.com/watch?v=bD6V3rcr_54)

Packages from pip
 - tensorflow
 - gym
 - keras
 - keras-rl2

From the `gym` module, need `Env` as the clas to extend, and `gym.spaces.Discrete`, `gym.spaces.Box` to describe the action or response spaces. 

https://gym.openai.com/docs/

```python
class BattleEnv(Env):
  def __init__(self):
    # define the action space
    # example uses three possible values.. decrease, increase, stay the same
    self.action_space = gym.spaces.Discrete(3)

    # define the observation space
    self.observation_space = Box(low=np.array([0]), high=np.array([100]))

    # length - maximum allowed turns per battle? 
    self.battle_length = 20

    # reset
    reset()

  def step(self, action):
    # return: observation (object), reward (float), done (boolean; time to reset?), info (dict; diagnostic for debugging)
    pass

  def render(self):
    pass
  def reset(self):
    self.state = number
    pass
```

so need to setup an interface where step() can either interact with a saved game, or with a live one?
wrapper training function also needs to interact.. to either choose an action, or pull one from the save file


need to quanity the option space - how many choices are there? discrete or continuous? Choose discrete since there should never be options between psyenergies, etc. So then need to somehow compute the set of discrete actions. 

`Discrete(n)` space type has *one variable* that can take on `n` possible values; `[0... n-1]`. 

## State space
- Character stats, status, active psyenergies, not items. (112 numbers per character: 448 total)
- Djinn statuses (3 per djinn.. 7x4 max djinn in GS1.. 84 numbers)
- enemy status & status (48 per enemy: 240 total)

Atari playing systems use an `observation space` which is Box(210, 160, 3) ; 210 pixels by 160 pixels by 3 color channels. Each of the ~100k items can have a value (presumably 0-255; simple graphics). And then potentially stack a few layers if velocity tracking is important. 

Stitch my observation space into just a long list of numbers. So Box(772,). Doesn't seem so bad!
TODO do this!

TODO scrub the Unit struct to see if any can be removed. Maybe some enemy base stats? (leaving the maximum) since their maximum possible is not ever going to be modified during a battle? 

## Action Space
- each character, any possible action (except items), include defense (want waiting to heal on lesser monsters to be a good strategy)
- how to train on action space since the options will change as a result of the state (class)
- the psyenergy list only shows what you HAVE NOW, not what you could have if you changed your class with the djinn you have. so this makes a good list of the currently available options. seems to be ordered as it is ordered in the battle menu (danger - some are not battle psyenergies). 
- the actions all have targets as well. 
- Ignore items - consumable and thus can't continue indefinitely. 
- Allow any summon to be requested, to enable pre-casting of summons

Action space is a big `Discrete` space. Just concatenate everything. What is a sensible way of encoding/decoding this?
- (4) For each character. use true character_ids, auto skip/ignore characters that are unavailable to act
- (197) expand the action & subaction list! ~~255~~ 151 possible psyenergies + 28 djinn + 16 summons + 1 attack + 1 defend
- (5) target increment

3940 possible results (though many are invalid..)

Actually a lot less - there is a lot of the psyenergy byte that is used up by weapon unleashes and djinn and summons, and enemy moves.. and items .. definitely want to pare down to actually valid options..

TODO write an encoder/decoder for this action space. A pre-written array.. given integer convert to action type, command, target, etc. Also store text Also store text. 
Separate array of string conversions? A way to start an auto builder? 

NN representation (0-n integer) --> lookup table --> 
Table is 3D array? Or have some way to cut it down ahead of time.. back out target and character first, then just have a medium complexity table dealing with the action_type/subaction values.

```C
action_state_index = actor_id*QTY_TARGETS*QTY_ACTIONS + target_id*QTY_ACTIONS + action_id;
```
given an integer, need to figure out what the component parts are
```C
actor_id = floor(action_state_index / (QTY_TARGETS * QTY_ACTIONS));
actor_id_rem = action_state_index - actor_id * QTY_TARGETS * QTY_ACTIONS;
target_id = floor(actor_id_rem / (QTY_ACTIONS));
action_id = actor_id_rem - target_id * QTY_ACTIONS;
```
TODO is there a higher efficiency algorithm for doing this? Probably I should not care..

NO - for each player's possible actions (197 action x 5 targets = 985) every other player has that many options as well. So 985^4 = **941 billion**. That is the real possible action space. 

What if you limit the action space? Only 12 psyenergies + 7 djinn + 16 summons + 2 = 37.. Five targets means each character's action space is 185. 185^4 = **1.2 billion**. Still far too many.

Can't I just have four discrete outputs? Quick searches look like complicated / maybe not. Maybe with a custom branching network architecture.

TensorFlow seems to have a MultiDiscrete space type? Also Discrete
Choice of algorithm is really about the *action* space (not so much the *observation* space).

## Reward Function
What is the reward function? 
Idea: reward after every turn: `sum(ally_health) - sum(enemy_health) - 2000[qty invalid action]`
- encourages ally health to be high
- encourages enemy health to be low
- put in tuning factors to adjust the relative weight
- a strong dis-incentive to selecting an invalid input

## Diagnostic Info
- store factors that go into reward. so the weighting can be adjusted later
- 
