all:
	gcc -Wall -std=c99 dsp2.c -o dsp2 `pkg-config glfw3 --cflags --libs` -lGL -lGLU -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lm