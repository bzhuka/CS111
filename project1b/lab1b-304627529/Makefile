main: client.c server.c
	gcc -o client -lpthread -lmcrypt -Wall -g client.c
	gcc -o server -lpthread -lmcrypt -Wall -g server.c

client: client.c
	gcc -o client -lpthread -lmcrypt -Wall -g client.c

server: server.c
	gcc -o server -lpthread -lmcrypt -Wall -g server.c

clean:
	rm server
	rm client

dist:
	tar -czvf lab1b-304627529.tar.gz server.c client.c Makefile README my.key
