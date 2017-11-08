make:
	mpicc main.c -o run -lm
	mpirun -np 20 run
