all:
	gcc -Wall -std=c99 -c socket.c
	gcc -Wall -std=c99 -c dsp.c
	gcc -Wall -std=c99 -c main.c
	gcc -o dsp main.o socket.o dsp.o `pkg-config glfw3 --cflags --libs` -lGL -lGLU -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lm
	rm -f *.o
clean:
	rm -f *.o dsp