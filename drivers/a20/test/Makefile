CC := arm-linux-androideabi-gcc
CFLAGS := -std=gnu99

progs=gpio readtty disp install testtty testdns ping

all : $(progs)

.PHONY : clean

disp : disp.o
gpio : gpio.o
readtty : readtty.o
testtty : testtty.o
testdns : testdns.o
ping : ping.o

install :
	cp -vf gpio readtty disp /home/jinglun/A20/dbg/

clean :
	rm -rf *.o $(progs)

