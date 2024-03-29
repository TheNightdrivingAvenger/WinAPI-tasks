#pragma once

#include "headers\queue.h"
#include "headers\blockingqueue.h"

typedef struct tagTHREADPOOL {
	HANDLE mainThread;
	HANDLE *workerThreads;
	PQUEUEHEAD tasks;
	//PQUEUEHEAD freeThreads;
	int totalThreadsCount;
	HANDLE poolStoppedEvent; // manual-reset; used for hard pool termination (do not wait for all the tasks in the queue)
	HANDLE workerBlockingEvent; // auto-reset
	HANDLE workerThreadsSemaphore;
	HANDLE workerReadValueEvent; // auto-reset
	HANDLE dummyEvent; // for passing as cancellation token when I don't need one
	volatile PWORKITEM currentWork;
} THREADPOOL, *PTHREADPOOL;
