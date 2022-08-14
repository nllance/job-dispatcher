#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "log.h"

// Define priorities
#define high    0  // 0 means that the job is high priority
#define medium  1  // 1 = medium priority
#define low     2  // 2 = low priority

/* Note: Jobs with a high priority take precedence over jobs with a medium priority. 
   Jobs with a medium priority take precedence over jobs with a low priority. 
   And by some discrete math law, jobs with a high priority take precedence over jobs with a low priority. 
   A job in the low priority can be there for an unlimited number of times. 
*/

// The following constants define how many times a job can be in a particular queue, before it is demoted. 
#define hpq_times   2 // 2 times in the high priority queue (hpq)
#define mpq_times   4 // 4 times in the medium priority queue (mpq)

/* Data structure to handle jobs. It will have 5 members:
  - process pid
  - is it done or not?
  - current priority
  - keep track of how many times it's gone through the hpq
  - keep track of how many times it's gone through the mpq
*/
typedef struct Job {
  pid_t pid;
  bool done;
  int priority;
  int hpq_no;
  int mpq_no;
} Job; 

/* Data structure to handle queues. It will have 3 members:
  - keep track of first element
  - keep track of last element
  - an array of length 6, consisting of job indexes (For example, queue[first] = the index of next job)
  Queues are FIFO (First In, First Out)
*/
typedef struct Queue {
  int queue[6];
  int first;
  int last;
} Queue;

// DECLARE PROTOTYPES: functions, structs, sigset, etc.
pid_t create_job(int i); // return type is `pid_t` and parameter is an int
void siga_handler();   // handler for signal SIGALRM
void sigc_handler();   // handler for signal SIGCHLD
sigset_t mymask1;      // normal signal process mask when all signals are free but SIGALRM and SIGCHLD are blocked
sigset_t mymask2;      // special signal process mask when all signals are free
sigset_t jobmask;      // job signal process mask blocking all signals, leaving only SIGUSR2 unblocked
struct sigaction sa_alarm;  // disposition for SIGALRM
struct sigaction sa_chld;   // disposition for SIGCHLD

// Auxiliary functions for dealing with queues
Queue* new_queue(); // initialize a new queue
void enqueue(Queue* queue, int i); // add job of index i to a queue
int dequeue(Queue* queue); // pop the first job of a queue

int logindex = 0;
int * logi = &logindex;

/* Global int which stores how many jobs are present
   The number of jobs is supplied by the user via CLI args
*/
int number_of_jobs;

/* Job table
   If there are fewer jobs than 6, the last items of the array will not be set. 
   The index of a job used elsewhere is its index in the job table.
*/
Job job_table[6];

// Queues declaration
Queue* hpq;
Queue* mpq;
Queue* lpq;

// Index of the current executing job
int jobi; 

// function main ------------------------------------------------- 
int main(int argc, char** argv) {
  int i;
  pid_t pid;
  
  // check the number of command line arguments, if not 2, terminate
  if (argc != 2) {
    printf("Error: Wrong number of arguments\n");
    exit(0);
  }

  // convert argv[1] (string) to an int
  int arg = atoi(argv[1]);
  // check if the single command line argument (argv[1]) has value 3 to 6, if not, terminate
  if ((arg < 3) || (arg > 6)) {
    printf("Error: Not a number between 3 and 6\n");
    exit(0);
  }
  printf("The argument is: %s\n", argv[1]);

  // set appropriately number_of_jobs
  number_of_jobs = arg;
  
  create_log("assgn1.log");
  
  // prepare mymask1 -- SIGCHLD and SIGALRM blocked, all other signals free
  sigemptyset(&mymask1);
  sigaddset(&mymask1, SIGALRM);
  sigaddset(&mymask1, SIGCHLD);

  // install mymask1 as the process signal mask using sigprocmask() 
  sigprocmask(SIG_SETMASK, &mymask1, NULL);

  // prepare mymask2 -- all signals free 
  sigemptyset(&mymask2);

  // prepare jobmask -- all signals blocked except SIGUSR2 
  sigfillset(&jobmask);
  sigdelset(&jobmask, SIGUSR2);

  // prepare SIGALRM disposition sa_alarm
  sa_alarm.sa_handler = siga_handler; // its handler (sa_handler) is siga_handler()
  sigfillset(&sa_alarm.sa_mask); // its signal mask (sa_mask) must block all signals
  sa_alarm.sa_flags = SA_RESTART; // its flags (sa_flags) must be set to SA_RESTART 
  sigaction(SIGALRM, &sa_alarm, NULL); // install SIGALRM disposition using sigaction() 
  
  // prepare SIGCHLD disposition sa_chld
  sa_chld.sa_handler = sigc_handler; // its handler (sa_handler) is sigc_handler()
  sigfillset(&sa_chld.sa_mask); // its signal mask (sa_mask) must block all signals
  sa_chld.sa_flags = SA_RESTART; // its flags (sa_flags) must be set to SA_RESTART 
  sigaction(SIGCHLD, &sa_chld, NULL); // install SIGCHLD disposition using sigaction() 

  // create empty queues
  hpq = new_queue();
  mpq = new_queue();
  lpq = new_queue();
  
  // create and save information about jobs
  for(i = 0; i < number_of_jobs; i++) {
    pid = create_job(i);
    job_table[i].pid = pid; // save pid for job i
    job_table[i].priority = high; // set priority to high
    job_table[i].done = false; // job not finished
    job_table[i].hpq_no = 0; // job hasn't gone through the high-priority queue
    job_table[i].mpq_no = 0; // job hasn't gone through the medium-priority queue
    enqueue(hpq, i); // put job in the high-priority queue
  }

  // loop for dispatching jobs; ends when all queues are empty
  while(1) {

    // the high-priority queue is not empty
    if (hpq->first != -1){
      jobi = dequeue(hpq); // Pop from the queue
      kill(job_table[jobi].pid, SIGUSR1); // "switch it on" by sending it SIGUSR1
      job_table[jobi].hpq_no += 1; // it's gone through the high-priority queue one more time
      Msg("Switched on high-priority job %d", jobi); // record it in the log
      msg("Switched on high-priority job %d", jobi); // display it on the screen
    }
    // the high-priority queue is empty but the medium-priority queue is not
    else if (mpq->first != -1){
      jobi = dequeue(mpq);
      kill(job_table[jobi].pid, SIGUSR1); 
      job_table[jobi].mpq_no += 1; // it's gone through the medium-priority queue one more time
      Msg("Switched on medium-priority job %d", jobi);
      msg("Switched on medium-priority job %d", jobi);
    }
    // the high- and medium-priority queues are empty, but the low-priority queue is not
    else if (lpq->first != -1){
      jobi = dequeue(lpq);
      kill(job_table[jobi].pid, SIGUSR1); 
      Msg("Switched on low-priority job %d", jobi);
      msg("Switched on low-priority job %d", jobi);
    }
    // all three queues are empty
    else {
      // free the memory space for queues
      free(hpq);
      free(mpq);
      free(lpq);
      Msg("All jobs done");
      msg("All jobs done");
      break; // terminate the loop
    }

    alarm(1); // set alarm for 1 second
    /* switch the current signal process mask mymask1 to mymask2 while going to suspension using sigsuspend()
       thus only SIGCHLD or SIGALRM will wake it up from suspension
       SIGCHLD indicates that the job that is currently executing just terminated
       SIGALRM indicates that the time for the job currently executing is up and it must be "switched off"
    */
    sigsuspend(&mymask2);
  }
    
  return 0;
} // end function main

// function new_queue --------------------------------------------
Queue* new_queue(){
  // Must assign a value to a pointer, otherwise it points to an unpredictable location
  // See also: https://stackoverflow.com/questions/12958931/warning-x-may-be-used-uninitialized-in-this-function
  Queue *q = malloc(sizeof(Queue)); // allocate memory for a new queue
  q->first = -1; // first == -1 represents an empty queue
  return q;
} // end function new_queue

// function enqueue -------------------------------------------- 
void enqueue(Queue* queue, int i) {
  // if first < 0, set last = first = 0
  if (queue->first < 0) {
    queue->first = 0; 
    queue->last = 0;
  }
  // if first >= 0, and last == 5, reset last = 0
  else if (queue->last == 5)
    queue->last = 0;
  // else, first >= 0 and last < 5
  else
    queue->last += 1;
  queue->queue[queue->last] = i; // set queue[last] = i
} // end function enqueue

// function dequeue -------------------------------------------- 
int dequeue(Queue* queue) {
  int index; // save the first job here
  // if first < 0, the queue is empty, nothing to dequeue
  if (queue->first < 0) {
    printf("Error: Cannot dequeue from empty queue\n");
    exit(0);
  }
  // if first == last, there's only 1 job in the queue
  else if (queue->first == queue->last) {
    index = queue->queue[queue->first];
    queue->first = -1;
  }
  // if first == 5, reset first = 0
  else if (queue->first == 5) {
    index = queue->queue[queue->first];
    queue->first = 0;
  }
  // else, 0 <= first < 5
  else {
    index = queue->queue[queue->first];
    queue->first += 1;
  }
  return index;
} // end function dequeue

// function create_job -------------------------------------------- 
pid_t create_job(int i) {
  pid_t pid;
  char argv0[10];
  char argv1[10];

  // switch process signal mask from mymask1 to jobmask 
  sigprocmask(SIG_SETMASK, &jobmask, NULL);
  
  if ((pid = fork()) < 0) Sys_exit("fork error\n");

  // child process
  if (pid == 0) { 
    strcpy(argv0, "job");
    sprintf(argv1, "%d", i);
    execl("job", argv0, argv1, NULL);
  }
  // parent process
  else
    // switch process signal mask from jobmask back to mymask1
    sigprocmask(SIG_SETMASK, &mymask1, NULL);
 
  return pid;
} // end function create_job
  
// function siga_handler ------------------------------------------ 
void siga_handler() {

  // if the current job is in the high-priority queue
  if (job_table[jobi].priority == high){
    kill(job_table[jobi].pid, SIGUSR2); // "switch it off" by sending it SIGUSR2
    // if it has already reached hpq_times
    if (job_table[jobi].hpq_no == hpq_times) {
      enqueue(mpq, jobi); // demote it to the medium-priority queue
      job_table[jobi].priority = medium; // set priority to medium
    }
    // if it hasn't reached hpq_times yet
    else
      enqueue(hpq, jobi); // put it back to the high-priority queue
    Msg("Switched off high-priority job %d", jobi);
    msg("Switched off high-priority job %d", jobi);
  }

  // if the current job is in the medium-priority queue
  else if (job_table[jobi].priority == medium){
    kill(job_table[jobi].pid, SIGUSR2);
    // if it has already reached mpq_times
    if (job_table[jobi].mpq_no == mpq_times) {
      enqueue(lpq, jobi); // demote it to the low-priority queue
      job_table[jobi].priority = low; // set priority to low
    }
    // if it hasn't reached mpq_times yet
    else
      enqueue(mpq, jobi); // put it back to the medium-priority queue
    Msg("Switched off medium-priority job %d", jobi);
    msg("Switched off medium-priority job %d", jobi);
  }

  // if the current job is in the low-priority queue
  else {
    kill(job_table[jobi].pid, SIGUSR2);
    enqueue(lpq, jobi); // put it back to the low-priority queue
    Msg("Switched off low-priority job %d", jobi);
    msg("Switched off low-priority job %d", jobi);
  }
} // end function siga_handler
  
// function sigc_handler ------------------------------------------ 
void sigc_handler() {
  alarm(0); // disarm the alarm  
  job_table[jobi].done = true; // notice it is done
  Msg("Job %d done", jobi);
  msg("Job %d done", jobi);
} // end function sigc_handler
