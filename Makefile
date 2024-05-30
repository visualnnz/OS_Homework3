all: pmergesort.c
	gcc -o pmergesort pmergesort.c

clean:
	rm -rf pmergesort