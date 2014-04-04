all:
	gcc -Wall -std=c99 -c dsp.c
	gcc -Wall -std=c99 -c main.c
	gcc -Wall -std=c99 -c calibrate.c
	gcc -Wall -std=c99 -c finger.c
	gcc -o dsp main.o dsp.o calibrate.o finger.o `pkg-config glfw3 --cflags --libs` -lGL -lGLU -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lm
	rm -f *.o
clean:
	rm -f *.o dsp