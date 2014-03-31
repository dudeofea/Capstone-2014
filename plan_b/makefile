all:
	gcc -Wall -std=c99 -c socket.c
	gcc -Wall -std=c99 -c dsp.c
	gcc -Wall -std=c99 -c main.c
	gcc -o dsp main.o socket.o dsp.o -lXrandr -lpthread -lXi -lm
	rm -f *.o
clean:
	rm -f *.o dsp