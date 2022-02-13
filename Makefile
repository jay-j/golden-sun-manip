COMMON=-O2 -std=c99 -Wall -pedantic -pthread -g
LIB=golden_sun.h golden_sun_utils.h memory_utils.h
LIBOBJ=memory_utils.o golden_sun_utils.o

all: oneshot.bin

%.bin: %.o $(LIBOBJ)
	gcc $< $(COMMON) $(LIBOBJ) -o $@

.PRECIOUS: %.o
%.o: %.c $(LIB)
	gcc -c $< $(COMMON) -o $@

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
