# Z-algorithm
Implementing Z algorithm for pattern matching in sequential and Parallel

<hr/>


### Makefile
CC=mpicxx

all: parallel

parallel: parallel.o
	$(CC) -o parallel parallel.o

parallel.o: parallelMPI.cpp
	$(CC) -o parallel.o -c parallelMPI.cpp

clean:
	rm -f parallel.o parallel

<hr/>

### Run file
mpirun -np 4 ./parallel ../inputs/input64.txt ../outputs/parallel64.txt
