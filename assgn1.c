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

// You may or may not use this; depends on your implementation
int logindex = 0;
int * logi = &logindex;

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
  - a queue of length 6
  Queues are FIFO (First In, First Out)
*/
typedef struct Queue {
  int queue[6];
  int first;
  int last;
} Queue;

// DECLARE PROTOTYPES: functions, structs, sigset, etc.
pid_t create_job(int i); // Return type is `pid_t` and parameter is an int
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
int dequeue(Queue* queue); // pop the first element of a queue

/* Global int which stores how many jobs are present
   The number of jobs is supplied by the user via CLI args
*/
int number_of_jobs;

/* Job table
   If there are fewer jobs than 6, the last items of the array will not be set. 
   The index of a job used elsewhere is its index in the job table.
*/
Job job_table[6];

// function main ------------------------------------------------- 
int main(int argc, char** argv) {
  int i, j;
  pid_t pid;
  
  // check the number of command line arguments, if not 2, terminate
  // check if the single command line argument (argv[1]) has value 3 to 6, if not, terminate
  if ((argc != 2) || (strchr("3456", argv[1]) == NULL)) {
    printf("Error: Incorrect input\n");
    exit(0);
  }
  printf("The argument is: %s\n", argv[1]);

  // convert argv[1] (string) to an int and set appropriately number_of_jobs
  number_of_jobs = atoi(argv[1]);
  
  create_log("assgn1.log");
  
  // prepare mymask1 -- SIGCHLD and SIGALRM blocked, all other signals free
  if ((sigemptyset(&mymask1) == -1) ||
      (sigaddset(&mymask1, SIGALRM) == -1) ||
      (sigaddset(&mymask1, SIGCHLD) == -1))
    perror("Failed to set up mymask1");

  // install mymask1 as the process signal mask using sigprocmask() 
  if (sigprocmask(SIG_SETMASK, &mymask1, NULL) == -1) 
    perror("Failed to install mymask1 as the process mask");

  // prepare mymask2 -- all signals free 
  if (sigemptyset(&mymask2) == -1)
    perror("Failed to set up mymask2");

  // prepare jobmask -- all signals blocked except SIGUSR2 
  if ((sigfillset(&jobmask) == -1) ||
      (sigdelset(&jobmask, SIGUSR2) == -1))
    perror("Failed to set up jobmask");

  // prepare SIGALRM disposition sa_alarm
  sa_alarm.sa_handler = siga_handler; // its handler (sa_handler) is siga_handler()
  sa_alarm.sa_flags = SA_RESTART; // its flags (sa_flags) must be set to SA_RESTART 
  if ((sigfillset(&sa_alarm.sa_mask) == -1) || // its signal mask (sa_mask) must block all signals
      (sigaction(SIGALRM, &sa_alarm, NULL) == -1)) // install SIGALRM disposition using sigaction() 
    perror("Failed to install SIGALRM disposition");
  
  // prepare SIGCHLD disposition sa_chld
  sa_chld.sa_handler = sigc_handler; // its handler (sa_handler) is sigc_handler()
  sa_chld.sa_flags = SA_RESTART; // its flags (sa_flags) must be set to SA_RESTART 
  if ((sigfillset(&sa_chld.sa_mask) == -1) || // its signal mask (sa_mask) must block all signals
      (sigaction(SIGCHLD, &sa_chld, NULL) == -1)) // install SIGCHLD disposition using sigaction() 
    perror("Failed to install SIGCHLD disposition");

  Queue* hpq = new_queue(); // create empty high-priority queue
  Queue* mpq = new_queue(); // create empty medium-priority queue
  Queue* lpq = new_queue(); // create empty low-priority queue
  
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
      // "switch on" the first job from the highest-priority non-empty queue by sending it SIGUSR1
      kill(job_table[hpq->first].pid, SIGUSR1);
      Msg("Switched on high-priority job %d", hpq->first); // record it in the log
      msg("Switched on high-priority job %d", hpq->first); // display it on the screen
    }
    // the high-priority queue is empty but the medium-priority queue is not
    else if (mpq->first != -1){
      kill(job_table[mpq->first].pid, SIGUSR1);
      Msg("Switched on medium-priority job %d", mpq->first);
      msg("Switched on medium-priority job %d", mpq->first);
    }
    // the high- and medium-priority queues are empty, but the low-priority queue is not
    else if (lpq->first != -1){
      kill(job_table[lpq->first].pid, SIGUSR1);
      Msg("Switched on low-priority job %d", lpq->first);
      msg("Switched on low-priority job %d", lpq->first);
    }
    // all three queues are empty
    else {
      Msg("All jobs done");
      msg("All jobs done");
      break; // terminate the loop
    }
    alarm(1); // set alarm for 1 second
    /* switch the current signal process mask mymask1 to mymask2 while
       going to suspension using sigsuspend()
       (thus only SIGCHLD or SIGALRM will wake it up from suspension
       SIGCHLD indicates that the job that is currently executing just
       terminated, SIGALRM indicates that the time for the job currently
       executing is up and it must be "switched off")
    */
    sigsuspend(&mymask2);
  }
    
  return 0;
} // end function main

// function new_queue --------------------------------------------
Queue* new_queue(){
  Queue* q;
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
    queue->last++;
  queue->queue[queue->last] = i; // set queue[last] = i
} // end function enqueue

// function dequeue -------------------------------------------- 
int dequeue(Queue* queue) {
  int index; // save the first element here
  // if first < 0, the queue is empty, nothing to dequeue
  if (queue->first < 0) {
    printf("Error: Cannot dequeue from empty queue\n");
    exit(0);
  }
  // if first == last, there's only 1 element in the queue
  else if (queue->first == queue->last) {
    index = queue->first;
    queue->first = -1;
  }
  // if first == 5, reset first = 0
  else if (queue->first == 5) {
    index = queue->first;
    queue->first = 0;
  }
  // else, 0 <= first < 5, first++
  else {
    index = queue->first;
    queue->first++;
  }
  return index;
} // end function dequeue

// function create_job -------------------------------------------- 
pid_t create_job(int i) {
  pid_t pid;
  char argv0[10];
  char argv1[10];

  // switch process signal mask from mymask1 to jobmask 
  if (sigprocmask(SIG_SETMASK, &jobmask, NULL) == -1) 
    perror("Failed to switch process signal mask from mymask1 to jobmask");
  
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
  // TO DO
  // "switch off" the currently executing job by sending it SIGUSR2 signal
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
} // end function siga_handler
  
  
// function sigc_handler ------------------------------------------ 
void sigc_handler() {
  // TO DO
  // disarm the alarm
  // record in the log that the currently executing job is done by
  // Msg("job %d done",job number);
  // END TO DO 
    printf("%d\n",hpq_times);
} // end function sigc_handler