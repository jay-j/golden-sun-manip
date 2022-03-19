COMMON=-O2 -Wall -pthread -g
LIB=golden_sun.h golden_sun_utils.h  loop_timer.h
LIBOBJ= golden_sun_utils.o loop_timer.o

all: oneshot.bin keyboard_test.bin server_teleop.bin djinn_randomizer.bin

keyboard_test.bin: keyboard_test.o
	gcc $< $(COMMON) $(LIBOBJ) -lX11 -lXtst -o $@

%.bin: %.o $(LIBOBJ)
	gcc $< $(COMMON) $(LIBOBJ) -lX11 -lXtst -lzmq -o $@

.PRECIOUS: %.o
%.o: %.c $(LIB)
	gcc -c $< $(COMMON) -o $@

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
