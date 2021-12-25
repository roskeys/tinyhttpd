all: server client

LIBS = -pthread -fstack-protector -fpie -pie -Wl,-z,relro,-z,now

server: server.c
	gcc -g -Wall $(LIBS) -o $@ $<
client: client.c
	gcc -g -Wall -o $@ $<

clean:
	-rm server client