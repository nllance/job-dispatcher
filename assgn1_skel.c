// Include header files; check standard folder
// `gcc` already knows where to find these header files
// If you wanna find them, use `locate` (or `find`)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
// Note: If you see something like 'abc/xyz.h', then 'xyz.h' is inside
// a folder called 'abc'

// Include header files; check local folder
// The local folder is the space you are working out of; it is where
// the source files are
#include "log.h"

// Note: The `#include` keyword tells the compiler to copy all the
// contents of the header file and paste it right here.
// (i.e. cpp assgn1.c)

// The keyword `define` is a preprocessor directive. It is a constant
// that is used as a substitution for number, strings, etc. For
// example, `#define age 22`, tells the `cpp` to replace every
// occurence of `age` with 22

// Define priorities
#define high    0  // 0 means that the job is high priority
#define medium  1  // 1 = medium priority
#define low     2  // 2 = low priority

// Note: Jobs with a high priority take precedence over jobs with a
// medium priority. Jobs with a medium priority take precedence over
// jobs with a low priority. And by some discrete math law, jobs with
// a high priority take precedence over jobs with a low priority

// The following constants define how many times a job can be in a
// particular queue, before it is demoted
#define hpq_times   2 // 2 times in the high priority queue (hpq)
#define mpq_times   4 // 4 times in the medium priority queue (mpq)

//#define TRUE  1
//#define FALSE 0
 
// Note: A job in the low priority can be there for an unlimited
// number of times

// You may or may not use this; depends on your implementation
int logindex = 0;
int * logi = &logindex;
// `logi` contains the address of `logindex` - this is known as pass
// by reference, not value. 

// DECLARE PROTOTYPES: functions, structs, sigset, etc.
// i.e. Prototypes for functions define the return type, name, and
// parameters

pid_t create_job(int i); // Return type is `pid_t` and parameter is an int
void siga_handler();   // handler for signal SIGALRM
void sigc_handler();   // handler for signal SIGCHLD
sigset_t mymask1;      // normal signal process mask when all signals
                       // are free but SIGALRM and SIGCHLD are blocked
sigset_t mymask2;      // special signal process mask when all signals are free
sigset_t jobmask;      // job signal process mask blocking all signals, leaving
                       // only SIGUSR2 unblocked
struct sigaction sa_alarm;  // disposition for SIGALRM
struct sigaction sa_chld;   // disposition for SIGCHLD

// Global int which stores how many jobs are present
// The number of jobs is supplied by the user via CLI args
int number_of_jobs;

// void enqueue(??, ??);
// int dequeue(??, ??);

// TO DO
// your own data structure(s) to handle jobs and the 3 queues
//      1. Both will need to be implemented via structs. For example:
//              typedef struct NAME_STRUCT {
//                  int member1;
//                  char member2;
//              } NAME;
//      2. The struct for jobs will have 5 members:
//          - process pid
//          - is it done or not?
//          - current priority
//          - keep track of how many times its gone through the hpq
//          - keep track of how many times its gone through the mpq
//      3. There is a maximum of 6 jobs in total, and each struct
//         contains information for 1 job. So, how do you effectively
//         store and manage 6 jobs at once?
//           Hint1: If you had to declare 1000 variables of type int,
//           how would you do it? (i.e. int x1, x2, x3, ... x1000????)
//              Hint2: This will be your "job table"
//      4. The struct for queue will have 3 members:
//          - keep track of first element
//          - keep track of last element
//          - recall that there are at most 6 jobs (in total)
//      5. Since we are dealing with queues, we need to be able to
//         manipulate them; so additional functions are needed. These
//         need to be defined in this file! Do not create another C
//         source file; put everything in this file.
//           Potential functions:
//              - Enqueue (Add stuff to the queue)
//              - Dequeue (Remove stuff from the queue)
//              - Create  (Create the queue for the priority class)
//           Queues are FIFO (First In, First Out)
//         More information about implementing the queue can be found
//         on the course website.
//           For example:
//              - Declaring queues
//              - Keeping track of indices
// your own auxialiary variables
// END TO DO

// function main ------------------------------------------------- 
int main(int argc, char** argv) {
  int i, j;
  pid_t pid;
  
  // TO DO 
  // check the number of command line arguments, if not 2, terminate
  // the program with a proper error message on the screen.
  // check if the single command line argument (argv[1]) has value 3 to 6,
  // if not, treminate the program with a proper error message on the
  // screen.
  // set appropriately number_of_jobs
  // END TO DO
  if (argc != 2) {
    printf("ERROR: IncorrectNumberOfArgs\n");
    exit(0);
  }
  printf("The argument is: %s\n", argv[1]);
  // convert argv[1] (string) to an int
  // the function you need is called: (_ _ _ _)
  // store result (int) into number_of_jobs
  
  create_log("assgn1.log");
  
  // TO DO
  // prepare mymask1 -- SIGCHLD and SIGALRM blocked, all other signals free 
  // using sigemptyset(), sigfillset(), sigaddset(), sigdelset() 
  // END TO DO

  /*
   if ((sigemptyset(&mymask1) == -1) ||
       (sigaddset(&mymask1) SIGALRM) == -1) {
    // dont forget to block: SIGCHLD 
        perror("");
    }
  */



 
  // TO DO
  // instal mymask1 as the process signal mask using sigprocmask() 
  // END TO DO 
    if (sigprocmask(SIG_SETMASK, &mymask1, NULL) == -1) {
        // it did fail
    }
  
  // TO DO
  // prepare mymask2 -- all signals free 
  // using sigemptyset(), sigfillset(), sigaddset(), sigdelset() 
  // END TO DO
  
  // TO DO
  // prepare jobmask -- all signals blocked except SIGUSR2 
  // using sigemptyset(), sigfillset(), sigaddset(), sigdelset() 
  // END TO DO

    // WRONG: sigemptyset
    // WRONG: one by one, use sigaddset to block signals

    sigfillset(&jobmask);
    sigdelset(&jobmask, SIGUSR2);


  // TO DO
  // prepare SIGALRM disposition sa_alarm
  // its handler (sa_handler) is siga_handler()
  // its signal mask (sa_mask) must block all signals
  // its flags (sa_flags) must be set to SA_RESTART 
  // END TO DO
  
  // TO DO
  // instal SIGALRM disposition using sigaction() 
  // END TO DO
  
  // TO DO
  // prepare SIGCHLD disposition sa_chld
  // its handler (sa_handler) is sigc_handler()
  // its signal mask (sa_mask) must block all signals
  // its flags (sa_flags) must be set to SA_RESTART 
  // END TO DO

  // TO DO
  // instal SIGCHLD disposition using sigaction() 
  // END TO DO

  // TO DO
  // create empty high-priority queue
  // create empty medium-priority queue
  // create empty low-priority queue
  // END TO DO
  
  // TO DO
  // create a data structure to keep information about jobs - PID, number of runs
  // for(i = 0; i < number_of_jobs; i++) {
  //   pid = create_job(i);
  //    job_table[i].pid = pid // save pid for job i in your data structure
  //    job_table[i].priority = high
  //    job_table[i].othermember = ???
  //   put all jobs in the high-priority queue 
  //    
  // }
  // END TO DO

  // TO DO
  // in a loop
  //    if all queues are empty
  //       record it in the log by Msg("All jobs done");
  //       and display it on the screen by msg("All jobs done");
  //       and terminate the loop
  //    "switch on" the first job from the highest-priority non-empty queue
  //    by sending it the SIGUSR1 signal (using sigsend())
  //    Record it in the log using 
  //        Msg("Switched on high-priority job %d",job number);  or
  //        Msg("Switched on medium-priority job %d",job number); or
  //        Msg("Switched on low-priority job %d",job number); 
  //    announce it on the screen using corresponding msg();
  //    set alarm for 1 second using alarm()
  //    switch the current signal process mask mymask1 to mymask2 while
  //    going to suspension using sigsuspend()
  //    (thus only SIGCHLD or SIGALRM will wake it up from suspension
  //    SIGCHLD indicates that the job that is currently executing just
  //    terminated, SIGALRM indicates that the time for the job currently
  //    executing is up and it must be "switched off")
  // end loop
  // END TO DO
    
  return 0;
}// end function main
  
  
// function create_job -------------------------------------------- 
pid_t create_job(int i) {
  pid_t pid;
  char argv0[10];
  char argv1[10];
  
  // TO DO
  // switch process signal mask from mymask1 to jobmask 
  // using sigprocmask()
  // END TO DO

  
  if ((pid = fork()) < 0) Sys_exit("fork error\n");
  if (pid == 0) { // child process
    strcpy(argv0,"job");
    sprintf(argv1,"%d",i);
    execl("job",argv0,argv1,NULL);
  }
  
  // parent process
  // TO DO
  // switch process signal mask from jobmask back to mymask1 
  // using sigprocmask()
  // END TO DO 
 
  return pid;
}// end function create_job
  
  
  
  
// function siga_handler ------------------------------------------ 
void siga_handler() {
  // TO DO
  // "switch of" the currently executing job by sending it SIGUSR2 signal
  // (using kill())
  // either put the job back to the queue it came from (you must count
  // how many times it has been through the queue) or "demote it" to the
  // lower-prority queue.
  // record this in the log using 
  //     Msg("Switched off high-priority job %d",job number); or
  //     Msg("Switched off medium-priority job %d",job number); or
  //     Msg("Switched off low-priority job %d",job number);
  // announce it on the screen suing corresponding msg();
  // END TO DO 
  return;
}// end function siga_handler
  
  
// function sigc_handler ------------------------------------------ 
void sigc_handler() {
  // TO DO
  // disarm the alarm
  // record in the log that the currently executing job is done by
  // Msg("job %d done",job number);
  // END TO DO 
    printf("%d\n",hpq_times);
}// end function sigc_handler

// TO DO
// functions for handling your data structures
//   For example:
//      - Enqueue (Add to queue)
//      - Dequeue (Remove from queue)
//      - Create  (Create the queue; we need 1 for each priority class)
//   More information on course website under Assignment #1
// END TO DO

//void enqueue() {
//    // add something to queue
//}
