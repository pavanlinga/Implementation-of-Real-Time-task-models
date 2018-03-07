//----------------------------------------------------------------------------//
//   FILE        : main.c
//   Description : Reads the input text file, get the tasks from it.
//                 Create a linked list for each of the task and assign the linked list
//                 to each of the periodic or aperiodic thread that is created. Run the threads
//                 till the given time period. Once the absolute time exceeds the given period
//                 terminate all the threads that are created.
//   Author      : Pavan Kumar & Ankit Wagle
//   Date        : 02/05/2018
//----------------------------------------------------------------------------//

#define _GNU_SOURCE
#include "stdio.h"
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/input.h>

#define PI_Enabled_Mutex

#define MOUSEFILE "/dev/input/event20"                 //check the event number before executing the program
#define LEFT_CLICK 0
#define RIGHT_CLICK 1
#define LOCK 99
#define UNLOCK 77


pthread_t tid[10];
pthread_attr_t attr[10];
pthread_barrier_t mybarrier;
pthread_mutex_t lock[11];
pthread_mutex_t general_lock,left_lock, right_lock;
pthread_mutexattr_t  prioinherit[11],left_prioinherit,right_prioinherit,general_prioinherit;
pthread_cond_t left_click,right_click;

int char_2_num,thread_num=1, ret;
char ch;
int array[100]={0};
int temp_array[100]={0};
int Num_of_tasks=0, Total_period=0,timeout=0;

struct node{                                           //structure for each linked list node
  int data;                                            //to store the task data
  struct node *next;                                   //to store the next node address
};
struct node *start=NULL, *current=NULL;
struct input_event ie;

//----------------------------------------------------------------------------//
//    Function name: time_diff
//    Discription  : It takes the start time pointer and end time pointer of the
//                   type struct timespec, calculate the difference betweem them
//                   and store them in result pointer of type struct timespec
//                   convert the resultant time into milliseconds and return the
//                   milliseconds value to the calling function
//----------------------------------------------------------------------------//
long time_diff(struct timespec *read1, struct timespec *read2, struct timespec *result){
  long time_in_milli=0;
  if((read2->tv_nsec - read1->tv_nsec) < 0){                              //check if the time diff is negative
    result->tv_sec  = read2->tv_sec - read1->tv_sec -1;                   //if yes decrement the seconds by 1
	  result->tv_nsec = read2->tv_nsec - read1->tv_nsec + 1000000000;       // and increment the nsec and store the
  }                                                                       //result in result structure pointer
  else{
    result->tv_sec  = read2->tv_sec - read1->tv_sec;                      //else store the diff in result pointer
  	result->tv_nsec = read2->tv_nsec - read1->tv_nsec;
  }
  time_in_milli = ((result->tv_sec*1000)) + (result->tv_nsec)/1000000;    //convert the time into milliseconds
  return time_in_milli;                                                   //return the time in milliseconds
}

//----------------------------------------------------------------------------//
//    Function name: periodic_task
//    Discription  : It takes the pointer to the periodic thread. Barrier is created
//                   to make the threads to wait at the barrier. Once all the threads
//                   are initialised and created, release them.
//                   Periodic threads gets the linked list pointer and get its own task period
//                   Get the current time and keep track of the time (absolute) and check whether
//                   it exceeds the task period and total period or not. If the task is done within the task period
//                   sleep for remaining time, else start the new iteration. If it exceeds the total periodic
//                   come out of the loop and terminate the threads.
//----------------------------------------------------------------------------//
void* periodic_task(void *ptr){    			            //thread func -> sholud be replaced with periodic and aperiodic
  pthread_barrier_wait(&mybarrier);                 //wait till all the threads are created
	int myperiod =0,priority;//loop=0;                                  //to store the thread period
  long milli_sec=0;
  struct timespec read1, read2, result;
  struct timespec start,end;                        //to calculate the start and end time of the task body
	struct node *temp_ptr;
	temp_ptr = (struct node *)ptr;
  priority = temp_ptr->data;                       //store the priority of the thread
  temp_ptr = temp_ptr->next;                       //increment the pointer
  myperiod = temp_ptr->data;                       //store the period in myperiod
  temp_ptr = temp_ptr->next;                       //increment the pointer
	ptr = temp_ptr;
  printf("Periodic Thread id: %lu, priority: %d, task period:%d\n",syscall(SYS_gettid),priority,myperiod);     //printf the thread id of the current thread
	clock_gettime(CLOCK_MONOTONIC,&read1);                         //get the present time
	while(milli_sec < Total_period)                                //check if the absolute time is greater than the total period
	{
    clock_gettime(CLOCK_MONOTONIC,&start);                       //get the start time of the task body
    pthread_mutex_lock(&general_lock);                           //lock the crictail section
    end.tv_sec = (myperiod/1000) + start.tv_sec;                 //find the end time of the task body
    end.tv_nsec = ((myperiod%1000)*1000000) + start.tv_nsec;
    pthread_mutex_unlock(&general_lock);
    int lock_or_data=0,num=0,i=0,j=0;
     while(temp_ptr !=NULL){                                     //check if there is data in linked list
       j=0;
       lock_or_data =temp_ptr->data/10;                          //check if it is Lock or the iteration value
       if(lock_or_data == LOCK){
         num = temp_ptr->data%10;                                //get the lock number
         pthread_mutex_lock(&lock[num]);                         //lock the mutex with the given lock number
       }
       else if(lock_or_data == UNLOCK){                          //check if the data is related to unlock
         num = temp_ptr->data%10;                                //get the unlock number
         pthread_mutex_unlock(&lock[num]);                       //lunock the mutex with the given lock number
       }
       else{
         for(i=0;i<temp_ptr->data;i++){                          //get the number of iterations for the for loop
           j=j+i;
         }
       }
       temp_ptr = temp_ptr->next;                                //increment the pointer
       clock_gettime(CLOCK_MONOTONIC,&read2);                    //get the present time and store it in read2
       milli_sec =time_diff(&read1, &read2, &result);            //calculate the time diff and store in milli_sec
       if((milli_sec < Total_period) && (temp_ptr ==NULL)){            //find if milli_sec is less than total period and the find if node is NULL
         clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,&end,NULL);    //sleep if there is remainf time, else come out of it
       }
     }
	  clock_gettime(CLOCK_MONOTONIC,&read2);                       //get the present time and store it in read2
    milli_sec =time_diff(&read1, &read2, &result);               //calculate the time diff and store in milli_sec
    //printf("milli sec time: %ld\n",milli_sec);
    if(temp_ptr == NULL){                                        //for repeated execution of task
		     temp_ptr = ptr;
         //printf("Thread id %lu,no of task body completed %d\n",syscall(SYS_gettid),loop);     //printf the thread id of the current thread
         //loop++;
    }
  }
	printf("out of the periodic task\n");
	return 0;
}


//----------------------------------------------------------------------------//
//    Function name: aperiodic_task
//    Discription  : It takes the pointer to the aperiodic thread. Barrier is created
//                   to make the threads to wait at the barrier. Once all the threads
//                   are initialised and created, release them.
//                   Aperiodic threads gets the linked list pointer. FInd if it is left click event or right click event
//                   Create a global variable to check whether timeout is done or not. If yes, comes out of the while loop
//                   Create a conditional varialbe and make the thread wait on that condition.
//                   If there is an event, wake up the thread and execute the for loop and wait for the event
//----------------------------------------------------------------------------//

void* aperiodic_task(void *ptr){
  pthread_barrier_wait(&mybarrier);                     //wait till all the threads are created
	int i=0,event,j=0,priority;
	struct node *temp_ptr;
	temp_ptr = (struct node *)ptr;
  priority = temp_ptr->data;                       //store the priority of the thread
  temp_ptr = temp_ptr->next;                       //increment the pointer
  event = temp_ptr->data;                              //read the event 0----left clcik, 1--------right click
	temp_ptr = temp_ptr->next;                           //increment the pointer
  printf("Aperiodic Thread id:%lu, priority:%d\n",syscall(SYS_gettid),priority);     //printf the thread id of the current thread
	//printf("------Aperiodic thread fun-------\n");
	if(event == LEFT_CLICK){
	  while(timeout ==0){                                      //check out the timeout variable
      j=0;
		  pthread_mutex_lock(&left_lock);                         //required for pthread_cond_wait
		  pthread_cond_wait(&left_click, &left_lock);             //sleeps till the signal arrives
      if(timeout == 0){
		    for(i=0;i<temp_ptr->data;i++){                          //loop for given iterations
		     j=j+i;
		    }
        printf("in LEFT CLICK LOOP ----j value is: %d\n",j);
      }
		  pthread_mutex_unlock(&left_lock);
	  }
	}
	else if(event == RIGHT_CLICK){                               //for right click
	  while(timeout ==0){                                      //check out the timeout variable
      j=0;
		  pthread_mutex_lock(&right_lock);                         //required for pthread_cond_wait
		  pthread_cond_wait(&right_click,&right_lock);             //sleeps till the signal arrives
      if(timeout == 0){                                        //loop for given iterations
		    for(i=0;i<temp_ptr->data;i++){
		      j=j+i;
		    }
        printf("in RIGHT CLICK LOOP----j value is: %d\n",j);
      }
		  pthread_mutex_unlock(&right_lock);
	  }
	}
	return 0;
}

//----------------------------------------------------------------------------//
//    Function name: mouse_reader
//    Discription  : Barrier is created to make the threads to wait at the barrier. Once all the threads
//                   are initialised and created, release them.
//                   Read the values from the mouse. Make the read as nonblocking
//                   Create a global variable to check whether timeout is done or not. If yes, comes out of the while loop
//                   continously read the mouse dataa and find if it is right click or left click and the mouse is clicked or released
//                   If there is an event, signal the aperiodic thread waiting for that signal
//----------------------------------------------------------------------------//



void* mouse_reader(void *ptr){                                //this thread reads the mouse events
  pthread_barrier_wait(&mybarrier);                           //wait till all the threads are created
  struct timespec read1, read2, result;
	int mouse_fd,milli_sec=0;
	if((mouse_fd = open(MOUSEFILE, O_RDONLY)) == -1) {          //open the file
		perror("opening device");
		exit(EXIT_FAILURE);
	}
  int flags = fcntl(mouse_fd, F_GETFL, 0);                    //make the read as Non blocking
  fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);
  clock_gettime(CLOCK_MONOTONIC,&read1);                      //get the initial time
  while(milli_sec < Total_period){                            //check for timeout
    if(read(mouse_fd, &ie, sizeof(struct input_event))){      //read the mouse events
      if((ie.code == BTN_LEFT) && (ie.value == 1))            //check whether it is right click or left click
  		  pthread_cond_signal(&left_click);
  	  else if((ie.code == BTN_RIGHT) && (ie.value == 1))
  	 	  pthread_cond_signal(&right_click);
    }
    clock_gettime(CLOCK_MONOTONIC,&read2);                    //read the present time
    milli_sec =time_diff(&read1, &read2, &result);            //calculate the time diff
  }
  timeout=1;                                                  //Rise the timeout flag if the Total period is reached
  pthread_cond_signal(&left_click);
  pthread_cond_signal(&right_click);
	return 0;
}

int SingleChar_2_int(char charc, int arr_index){        //takes in the single char, clubs into an interger
  int var=0;
  var = array[arr_index];
  array[arr_index] = (var*10) + charc;
  return 0;
}

//----------------------------------------------------------------------------//
//    Function name: charcheck
//    Discription  : it coverts the ascii to char and replace the letters with the number as follows
//----------------------------------------------------------------------------//

int charcheck(char ch){                     //it replaces the alphabet value with a num
  switch (ch) {
    case 'L':				    //Replace 'L' with number 99
      char_2_num = 99;
      return char_2_num;
    case 'U':				    //Replace 'U' with number 77
      char_2_num = 77;
      return char_2_num;
    case 'P':				    //Replace 'P' with number 33
      char_2_num = 33;
      return char_2_num;
    case 'A':				    //Replace 'A' with number 11
      char_2_num = 11;
      return char_2_num;
    default:				    //Convert the Ascii value to respective integer
      char_2_num = ch - '0';
      return char_2_num;
  }
}

//----------------------------------------------------------------------------//
//    Function name: thread_init_create
//    Discription  : It creates the required periodic or aperiodic threads
//----------------------------------------------------------------------------//

int thread_init_create(void *ptr){        //thread initialization and thread create
  int task_finder,priority;
  struct node *temp_ptr;
  temp_ptr =(struct node *)ptr;

  task_finder = temp_ptr->data;			// Find whether the thread is periodic or aperiodic
  temp_ptr = temp_ptr->next;			// Point to the next node
  priority = temp_ptr->data;			// Get the thread priority
  //temp_ptr = temp_ptr->next;			// Point to the next node

  struct sched_param param[thread_num];
  pthread_attr_init(&attr[thread_num]);
  pthread_attr_getschedparam (&attr[thread_num], &param[thread_num]);
  param[thread_num].sched_priority = priority;
  pthread_attr_setschedparam (&attr[thread_num], &param[thread_num]);
  ret =pthread_attr_setschedpolicy(&attr[thread_num], SCHED_FIFO);
  if(ret != 0){
    printf("SCHED_FIFO not set\n");
    return 1;
  }

  if(task_finder == 33){
    if(pthread_create(&tid[thread_num], &attr[thread_num], periodic_task, temp_ptr) != 0){
      printf("thread not created\n");
    }
    printf("threads creates\n");
  }
  else if(task_finder == 11){
    if(pthread_create(&tid[thread_num], &attr[thread_num], aperiodic_task, temp_ptr) != 0){
      printf("thread not created\n");
    }printf("threads creates\n");
  }
  else printf("------Periodic or Aperiodic not specified-----Thread not created\n");
 thread_num++;
 return 0;
}

//----------------------------main program--------------------------------//
int main(){
  cpu_set_t  mask;                                       //create a mask of cpu_set_t type
  CPU_ZERO(&mask);                                       //make the mask to zero
  CPU_SET(0, &mask);
  if((sched_setaffinity(0, sizeof(mask), &mask)) !=0){    //set the cpu affinity ----CPU0
    printf("CPU affinity not set\n");
    return 1;
  }
  FILE *fp = fopen("process_def.txt","r");               //get the file pointer
  FILE *fp1 = fopen("process_def.txt","r");
  int arr_index=0,temp=0,lines=0,i=1;
  pthread_cond_init(&right_click,NULL);
  pthread_cond_init(&left_click,NULL);
  #ifdef PI_Enabled_Mutex
    int mutex_protocol,ret;
    printf("inside PI enabled\n");
    if((ret=pthread_mutexattr_init(&left_prioinherit)) !=0){          ///set the mutex attr
      printf("Mutex attribute initialisation failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_init(&right_prioinherit)) !=0){
      printf("Mutex attribute initialisation failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_init(&general_prioinherit)) !=0){
      printf("Mutex attribute initialisation failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_getprotocol(&left_prioinherit, &mutex_protocol)) !=0){
      printf("Mutex attr get protocol failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_getprotocol(&right_prioinherit, &mutex_protocol)) !=0){
      printf("Mutex attr get protocol failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_getprotocol(&general_prioinherit, &mutex_protocol))!=0){
      printf("Mutex attr get protocol failed\n");
      return 1;
    }

    if((ret=pthread_mutexattr_setprotocol(&left_prioinherit, PTHREAD_PRIO_INHERIT))!=0){      //set the mutex protocol
      printf("Mutex attr set protocol failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_setprotocol(&right_prioinherit, PTHREAD_PRIO_INHERIT))!=0){
      printf("Mutex attr set protocol failed\n");
      return 1;
    }
    if((ret=pthread_mutexattr_setprotocol(&general_prioinherit, PTHREAD_PRIO_INHERIT))!=0){
      printf("Mutex attr set protocol failed\n");
      return 1;
    }
    if((ret=pthread_mutex_init(&left_lock,&left_prioinherit)) !=0){                //initialise the mutex with prio inheritance
       printf("Mutex initialisation failed\n");
       return 1;
    }
    if((ret=pthread_mutex_init(&right_lock,&right_prioinherit)) !=0){
       printf("Mutex initialisation failed\n");
       return 1;
    }
    if((ret=pthread_mutex_init(&general_lock,&general_prioinherit)) !=0){
       printf("Mutex initialisation failed\n");
       return 1;
    }
  #endif
  #ifndef PI_Enabled_Mutex
    pthread_mutex_init(&left_lock,NULL);
    pthread_mutex_init(&right_lock,NULL);
    pthread_mutex_init(&general_lock,NULL);
  #endif

  while(fread(&ch,1,1,fp1) ==1){                                //read the file and find num of lines
    if (ch == '\n')
        lines++;
  }
  printf("total number of line: %d\n",lines);

//////create a thread to read the mouse events
  struct sched_param param[lines];
  pthread_attr_init(&attr[lines]);
  pthread_attr_getschedparam (&attr[lines], &param[lines]);
  param[lines].sched_priority = 99;
  pthread_attr_setschedparam (&attr[lines], &param[lines]);
  ret =pthread_attr_setschedpolicy(&attr[lines], SCHED_FIFO);
  if(ret != 0){
    printf("SCHED_FIFO not set\n");
    return 1;
  }
  if(pthread_create(&tid[lines], &attr[lines], mouse_reader, NULL) != 0){
      printf("thread not created\n");
  }

  for(i=0;i<11;i++){
      #ifdef PI_Enabled_Mutex                                                      //create mtex attr
        if((ret=pthread_mutexattr_init(&prioinherit[i])) !=0){
          printf("Mutex attribute initialisation failed\n");
          return 1;
        }
        pthread_mutexattr_getprotocol(&prioinherit[i], &mutex_protocol);
        pthread_mutexattr_setprotocol(&prioinherit[i], PTHREAD_PRIO_INHERIT);      //set the protocol for the threads
        if((ret=pthread_mutex_init(&lock[i],&prioinherit[i])) !=0){                //initiliase the threads
           printf("Mutex initialisation failed\n");
           return 1;
        }
      #endif
      #ifndef PI_Enabled_Mutex
        if(pthread_mutex_init(&lock[i],NULL) !=0){
           printf("Mutex initialisation failed\n");
           return 1;
         }
      #endif
  }

  while(fread(&ch,1,1,fp) ==1){       //read all the characters from the file
    if((ch != ' ') && (ch != '\n')){    //get all the single char and do the computation
      char_2_num =charcheck(ch);
      temp = char_2_num;
      SingleChar_2_int(temp,arr_index);
    }
    else if(arr_index < 2){
      if(arr_index == 1){
        Num_of_tasks = array[arr_index-1];
        Total_period = array[arr_index];
        pthread_barrier_init(&mybarrier, NULL, Num_of_tasks+1);         //one thread is created to read the mouse data
      }
      arr_index++;
    }
    else if((ch == ' ') || (ch == '\n')){                     //if encountered with a space or new line then
      struct node *newnode;                                   //get the interger value which is saved in an array,
      newnode = (struct node *)malloc(sizeof(struct node));   //and create a new node to store the array element
      newnode->data = array[arr_index];
      newnode->next = NULL;
      if(start == NULL){
        start = newnode;                          //to store the start addr of the linked list
        current = newnode;
      }
    else{
      current->next = newnode;
      current = newnode;
    }
    arr_index++;
    }

    if((ch == '\n') && (arr_index > 2)){                           //if there is a new line, means elemets belonging to thread have been read
      thread_init_create(start);              //create a thread and pass the linked list pointer
      start=NULL;                             //to start a new linked list for the other thread
      current=NULL;
    }
  }
  sleep(1);
  for(i=1; i<=Num_of_tasks+1; i++){                     //terminate the threads
	  pthread_join(tid[i], NULL);
	  printf("thread %d terminated\n", i);
  }
  pthread_barrier_destroy(&mybarrier);
  return 0;
}
