all:
	gcc -Wall -std=c99 -c ../dsp.c
	gcc -Wall -std=c99 -c unit_tests.c
	gcc -o tests unit_tests.o dsp.o -lcunit -lm
	rm -f *.o
clean:
	rm -f *.o tests