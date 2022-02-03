assgn1: assgn1.o log.o job
	gcc -o assgn1 assgn1.o log.o 
assgn1.o: assgn1.c log.h
	gcc -c assgn1.c 
log.o: log.c log.h
	gcc -c log.c 
job.o: job.c log.h
	gcc -c job.c
job: job.o log.o
	gcc -o job job.o log.o
make clean:
	rm *.o job assgn1.log assgn1
