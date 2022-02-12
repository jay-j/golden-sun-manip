COMMON=-O2 -std=c99 -Wall -pedantic -pthread -g
LIB=golden_sun.h golden_sun_export.h

all: oneshot.bin

%.bin: %.o
	gcc $< $(COMMON) -o $@

.PRECIOUS: %.o
%.o: %.c $(LIB)
	gcc -c $< $(COMMON) -o $@

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
