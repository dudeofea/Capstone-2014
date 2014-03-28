all:
	gcc -Wall -std=c99 -c paint.c
	gcc -o paint paint.o `pkg-config glfw3 --cflags --libs` -lGL -lGLU -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lm
	rm -f *.o
clean:
	rm -f *.o dsp