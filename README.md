# Golden Sun Hacking
Having some fun with the GBA Game [Golden Sun](https://en.wikipedia.org/wiki/Golden_Sun) #1. 

The unit struct is 332 bytes in both GS1 and TLA. 

# TODO
- Constant background monitoring
  - Automatically detect process pid. `$pidof mednafen`
  - service: provide game state on request
  - service: advance battle on request
- Identify where Djinn recovery status is
- Condense battle stats structs to remove empty data
