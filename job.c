#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "log.h"

int logindex = 0;
int* logi = &logindex;
struct sigaction sa_usr1;
struct sigaction sa_usr2;
sigset_t sigmask1;
sigset_t sigmask2;

void sig1_handler(int signo);
void sig2_handler(int signo);


// signals  SIGUSR1 --- execute
//          SIGUSR2 --- sleep


int jobi;

void DONE() { Msg("Job number %d done",jobi);
              msg("Job number %d done",jobi);}


// function main ------------------------------------------
int main(int argc,char** argv) {
  int i;
  unsigned int t;

  atexit(DONE);
  
  // we inherited signal process mask in which all signals 
  // are blocked except SIGUSR2 
  
  
  // install singal handler for SIGUSR1 
  sa_usr1.sa_handler = sig1_handler;
  sigemptyset(&sa_usr1.sa_mask);
  sa_usr1.sa_flags = SA_RESTART;
  sigaction(SIGUSR1,&sa_usr1,NULL);
  
  // install singal handler for SIGUSR2 
  sa_usr2.sa_handler = sig2_handler;
  sigemptyset(&sa_usr2.sa_mask);
  sa_usr2.sa_flags = SA_RESTART;
  sigaction(SIGUSR2,&sa_usr2,NULL);
  
  // prepare signal mask with SIGUSR1 unblocked 
  sigfillset(&sigmask1);
  sigdelset(&sigmask1,SIGUSR1);
  
  // we unblock SIGUSR1 and wait
  sigsuspend(&sigmask1);
  // when we wake up, we are back to the old signal mask,
  // i.e. all signals are blocked except SIGSUR2 
  
  if (argc != 2) 
    Msg_exit("wrong number of arguments\n");
  if((jobi = atoi(argv[1])) < 0)
    Msg_exit("incorrect argument\n");
  Msg("I am job number %d",jobi);
 
  // determine how many times the job shoud loop 
  do {
    srand(time(NULL)*getpid());
    t = rand();
    while(t > 20) t = (t %20);
  } while(t == 0);
  
  for(i = 0; i < t; i++) {
    Msg("I am job %d and making my %d-th turn out "
        "of %d",jobi,i+1,t);
    sleep(1);
  }
  
  }// end function main 
  
  
  // function sig1_handler --------------------------------
  void sig1_handler(int signo) {
   return;
  }// end function sig1_handler
  
  
  // function sig2_handler --------------------------------
  void sig2_handler(int signo) {
  
   // we go to sleep until SIGUSR1 
   sigsuspend(&sigmask1);
   return;
  
  }// end function sig2_handler 
