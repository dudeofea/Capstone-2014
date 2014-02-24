all:
	gcc -Wall -Wno-uninitialized -std=c99 macro.c -o macro `pkg-config --libs --cflags libusb-1.0` -lX11 -lXtst
clean:
	rm -f macro