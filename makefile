dispatcher: dispatcher.o log.o job
	gcc -o dispatcher dispatcher.o log.o 
dispatcher.o: dispatcher.c log.h
	gcc -c dispatcher.c 
log.o: log.c log.h
	gcc -c log.c 
job.o: job.c log.h
	gcc -c job.c
job: job.o log.o
	gcc -o job job.o log.o
make clean:
	rm *.o
