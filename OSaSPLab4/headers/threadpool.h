#pragma once

// should make workItem a struct (to store in the queue both this ptr and param (struct) ptr)
typedef int (*workProc)(void *);

typedef struct tagTHREADPOOL THREADPOOL, *PTHREADPOOL;

typedef struct tagWORKITEM {
	workProc callable;
	void *param;
} WORKITEM, *PWORKITEM;

PTHREADPOOL ThreadPool_Create(int threadsCount);
void ThreadPool_Stop(PTHREADPOOL pSelf, void (*releaseFunc)(PWORKITEM));
BOOL ThreadPool_AddTask(struct tagTHREADPOOL *pSelf, PWORKITEM task);
BOOL ThreadPool_Dispose(struct tagTHREADPOOL *pSelf);
