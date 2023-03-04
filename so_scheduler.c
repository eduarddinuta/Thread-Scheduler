#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define DECL_PREFIX
#define SO_MAX_NUM_EVENTS 256
#define SO_MAX_PRIO 5
#define INVALID_TID ((tid_t)0)

typedef pthread_t tid_t;
typedef void (so_handler)(unsigned int);

typedef struct thread_t {
	// priority of the thread
	unsigned int prio;

	// waiting state
	unsigned int isWaiting;

	// current time spent on the cpu
	unsigned int timeRunning;

	// function handler
	so_handler *func;

	// thread's semaphore used for synchronization
	sem_t sem;

	// thread's id  
	tid_t id;
} thread_t;

typedef struct scheduler_t {

	// queue array for every priority
	linked_list_t **queues;

	// time quantum for every thread on the cpu
	unsigned int time;

	// maximum number of io events
	unsigned int no_events;

	// reference to the current running thread
	thread_t *runningThread;

	// list of all finished threads used for deallocating
	linked_list_t *finishedThreads;

	// semaphore for the scheduler used to wait for all the threads to finish
	// before we end the program
	sem_t sched_sem;

	// total number of running threads
	int nthreads;

	// queue array for the waiting threads for every io event
	linked_list_t **waitingQueues;
} scheduler_t;

static scheduler_t *scheduler;

// checks whether the current thread needs to be replaced
DECL_PREFIX void replace_running_thread(void) {
	int ok = 0;
	thread_t *aux = scheduler->runningThread;

	// if the running thread's time expired and it is not in waiting state
	// we need to add it to the next possible threads
	int added = 0;
	if (aux != NULL && aux->timeRunning == scheduler->time && !aux->isWaiting) {
		enqueue(scheduler->queues[aux->prio], aux);
		added = 1;
	}

	// check if any condition that result in a context switch is true
	for (int i = 5; i >= 0; i--) {
		if (ll_get_size(scheduler->queues[i]) != 0 && (aux == NULL || i > aux->prio || aux->timeRunning == scheduler->time || aux->isWaiting)) {
			ok = 1;
			if (aux != NULL) 
				aux->timeRunning = 0;
			scheduler->runningThread = scheduler->queues[i]->head->data;
			scheduler->runningThread->timeRunning = 0;
			ll_node_t *node = dequeue(scheduler->queues[i]);
			free(node);
			break;
		}
	}

	// if the switch happened put the old thread back in queue, pause it and 
	// start the new thread
	if (ok) {
		if (aux && !added && !aux->isWaiting)
			enqueue(scheduler->queues[aux->prio], aux);

		sem_post(&(scheduler->runningThread->sem));
		if (aux != NULL)
			sem_wait(&(aux->sem));
	}
}

// intializes the scheduler structure
// returns 0 for success and -1 for error
DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io) {

	// checking if the parametres are correct
	if (scheduler != NULL)
		return -1; 

	if (io > SO_MAX_NUM_EVENTS)
		return -1;

	if (time_quantum == 0)
		return -1;

	// allocating all the needed structures
	scheduler = malloc(sizeof(scheduler_t));

	scheduler->no_events = io;
	scheduler->time = time_quantum;
	scheduler->runningThread = NULL;
	scheduler->nthreads = 0;
	scheduler->queues = malloc(sizeof(linked_list_t*) * (SO_MAX_PRIO + 1));

	for (int i = 0; i <= 5; i++)
		scheduler->queues[i] = ll_create();

	scheduler->finishedThreads = ll_create();

	sem_init(&(scheduler->sched_sem), 0, 0);

	scheduler->waitingQueues = malloc(sizeof(linked_list_t *) * (SO_MAX_NUM_EVENTS + 1));

	for (int i = 0; i <= 256; i++)
		scheduler->waitingQueues[i] = ll_create();

	return 0;
}   

// wrapper function to start the handler associated to a thread
// the function will only be called once the thread is running
// and will return only when the thread finished
DECL_PREFIX void* start_thread(void *params) {
	thread_t *p = (thread_t *)params;
	sem_wait(&(p->sem));

	p->func(p->prio);

	ll_add_nth_node(scheduler->finishedThreads, scheduler->finishedThreads->size, p);


	scheduler->runningThread = NULL;

	// the thread ended so we need to start a new one
	replace_running_thread();

	scheduler->nthreads--;
	if (scheduler->nthreads == 0)
		sem_post(&(scheduler->sched_sem));

	return NULL;
}

// fork function creates a new thread from the current running thread 
// with a given priority that calls the function func.
// returns the id of the new thread
DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority) {
	// check if the parametres are valid
	if (!func)
		return INVALID_TID;

	if (priority > SO_MAX_PRIO)
		return INVALID_TID;

	if (scheduler->runningThread)
		scheduler->runningThread->timeRunning++;

	// intialize a new thread structure
	thread_t *newThread = malloc(sizeof(thread_t));
	newThread->prio = priority;
	newThread->timeRunning = 0;
	newThread->func = func;
	newThread->isWaiting = 0;
	sem_init(&(newThread->sem), 0, 0);

	// create the new thread
	pthread_create(&(newThread->id), NULL, &start_thread, newThread);

	scheduler->nthreads++;

	enqueue(scheduler->queues[priority], newThread);

	// check if we need to switch the current running thread
	replace_running_thread();

	return newThread->id;
}

// stops the current running thread until io is signaled
// return 0 for success and -1 for error
DECL_PREFIX int so_wait(unsigned int io) {

	if (io >= scheduler->no_events)
		return -1;

	scheduler->runningThread->timeRunning++;

	// add the current thread to the waiting queue of signal io
	enqueue(scheduler->waitingQueues[io], scheduler->runningThread);


	scheduler->runningThread->isWaiting = 1;

	// check for a new thread to replace the waiting one
	replace_running_thread();

	return 0;
}

// signals the io device and wakes up all the waiting threads waiting for the io
// returns the number of signaled threads
DECL_PREFIX int so_signal(unsigned int io) {

	if (io >= scheduler->no_events)
		return -1;

	scheduler->runningThread->timeRunning++;

	int nrUnlocked = 0;

	// all the threads in the waiting queue are removed from waiting state
	while (scheduler->waitingQueues[io]->size != 0) {
		ll_node_t *node = dequeue(scheduler->waitingQueues[io]);
		node->data->isWaiting = 0;
		enqueue(scheduler->queues[node->data->prio], node->data);
		free(node);
		nrUnlocked++;
	}

	// check if we need to replace the current running thread
	replace_running_thread();

	return nrUnlocked;
}

// generic instruction, only consumes one time unit
DECL_PREFIX void so_exec(void) {
	scheduler->runningThread->timeRunning++;

	// check if we need to replace the current running thread
	replace_running_thread();
}

// deallocating all the memory used for the scheduler structures
DECL_PREFIX void so_end(void) {

    if (scheduler) {
		// waiting for all the threads to end before deallocating
		if (scheduler->nthreads != 0)
			sem_wait(&(scheduler->sched_sem));
    
		if (scheduler->finishedThreads) {

			// deallocating all the thread structures
			while (scheduler->finishedThreads->size != 0) {
				pthread_join(scheduler->finishedThreads->head->data->id, NULL);
				ll_node_t *node = dequeue(scheduler->finishedThreads);
				sem_destroy(&(node->data->sem));
				free(node->data);
				free(node);
			} 

			free(scheduler->finishedThreads);
		}

		if (scheduler->waitingQueues) {
			for (int i = 0; i <= 256; i++)
				if (scheduler->waitingQueues[i])
					ll_free(&(scheduler->waitingQueues[i]));
			
			free(scheduler->waitingQueues);
		}

		if (scheduler->queues) {
			for (int i = 0; i <= 5; i++) {
				if (scheduler->queues[i])
					ll_free(&(scheduler->queues[i]));
			}
            
			free(scheduler->queues);
			scheduler->queues = NULL;
        }

		sem_destroy(&(scheduler->sched_sem));
		free(scheduler);
		scheduler = NULL;
	}
}
