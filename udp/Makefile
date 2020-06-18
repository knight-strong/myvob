
LDFLAGS = -lpthread

.PHONY : clean

all : udp tcps tcpc

udp : udp.o
	$(CC) -o $@ $^ $(LDFLAGS)

tcps : tcps.o
	$(CC) -o $@ $^ $(LDFLAGS)

tcpc : tcpc.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean :
	rm -f *.o udp tcpc tcps
