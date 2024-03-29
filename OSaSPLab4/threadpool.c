/*
* "Free worker threads queue" could be used here (queue of some THREADINFO items),
* it could've made easier handing the task to a thread (through copying work item ptr into its THREADINFO object),
* and maybe thread management (maybe easier to wait for all or something).
* Hovewer, it would require a sync primitive for each thread in the pool, which is horrible
* (for taking a thread from the queue and unlocking it).
* Here I managed to get away with three events and one semaphore
* Pool stopping is NOT thread safe! Making it thread safe would require another critical section or something like this
* Adding tasks is thread safe (multiple threads can do this)
*/

#include <windows.h>
#include "headers\threadpool.h"
#include "headers\threadpoolinner.h"
#include "headers\blockingqueue.h"

#define QUEUE_DEFAULT_CAPACITY 20

DWORD WINAPI ThreadPool_MainThread(PTHREADPOOL pSelf)
{
	int queueResult = QUEUE_SUCCESS;
	while (queueResult == QUEUE_SUCCESS) {
		pSelf->currentWork = BlockingQueue_TakeElem(pSelf->tasks, pSelf->poolStoppedEvent, &queueResult);
		if (queueResult == QUEUE_FINISHED || queueResult == QUEUE_OPCANCELLED) {
			break;
		}

		WaitForSingleObject(pSelf->workerThreadsSemaphore, INFINITE); // will block if there're no free threads in the pool
		// check poolstopped event for termination (not graceful stopping!)
		// some thread(-s) is (are) free, we can perform the task
		SetEvent(pSelf->workerBlockingEvent);
		WaitForSingleObject(pSelf->workerReadValueEvent, INFINITE);
		pSelf->currentWork = NULL;
	}
	return 0;
}

DWORD WINAPI ThreadPool_WorkerLoop(PTHREADPOOL pSelf)
{
	while (1) {
		WaitForSingleObject(pSelf->workerBlockingEvent, INFINITE);
		//InterlockedDecrement(&pSelf->freeThreadsCount);
		// currentWork == NULL means it's time to close
		if (!pSelf->currentWork) {
			SetEvent(pSelf->workerReadValueEvent);
			break;
		}
		WORKITEM localWork = *(pSelf->currentWork);
		SetEvent(pSelf->workerReadValueEvent);
		localWork.callable(localWork.param);
		ReleaseSemaphore(pSelf->workerThreadsSemaphore, 1, NULL);
		//InterlockedIncrement(&pSelf->freeThreadsCount);
	}
	// no release semaphore because when work is NULL manager doesn't acquire it
	return 0;
}

BOOL ThreadPool_AddTask(PTHREADPOOL pSelf, PWORKITEM task)
{
	if (BlockingQueue_AddElem(pSelf->tasks, task, pSelf->dummyEvent) != QUEUE_SUCCESS) return FALSE;
	else return TRUE;
}

// stops the pool and lets it run remaining tasks; will block until all the remaining tasks are done
void ThreadPool_Stop(PTHREADPOOL pSelf, nodeReleaser releaseFunc)
{
	BlockingQueue_MarkFinished(pSelf->tasks);
	WaitForSingleObject(pSelf->mainThread, INFINITE);
	CloseHandle(pSelf->mainThread);
	for (int i = 0; i < pSelf->totalThreadsCount; i++) {
		WaitForSingleObject(pSelf->workerThreadsSemaphore, INFINITE); // waiting for all worker threads to complete their tasks
		SetEvent(pSelf->workerBlockingEvent); // signalling to the threads all the time to exit
	}
	for (int i = 0; i < pSelf->totalThreadsCount; i++) {
		WaitForSingleObject(pSelf->workerThreads[i], INFINITE); // waiting for all worker threads to complete
		CloseHandle(pSelf->workerThreads[i]);
	}
	CloseHandle(pSelf->workerBlockingEvent);
	CloseHandle(pSelf->workerThreadsSemaphore);
	CloseHandle(pSelf->workerReadValueEvent);
	BlockingQueue_Dispose(pSelf->tasks, releaseFunc);
}

PTHREADPOOL ThreadPool_Create(int threadsCount)
{
	if (threadsCount <= 0) return NULL;

	PTHREADPOOL pSelf = HeapAlloc(GetProcessHeap(), 0, sizeof(THREADPOOL));
	pSelf->tasks = BlockingQueue_Create(QUEUE_DEFAULT_CAPACITY);
	pSelf->poolStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	pSelf->workerBlockingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pSelf->workerReadValueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pSelf->currentWork = NULL;

	pSelf->dummyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if (!(pSelf->mainThread = CreateThread(NULL, 0, ThreadPool_MainThread, pSelf, CREATE_SUSPENDED, NULL))) return NULL;
	pSelf->workerThreads = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE) * threadsCount);
	int i;
	for (i = 0; i < threadsCount; i++) {
		if (pSelf->workerThreads[i] = CreateThread(NULL, 0, ThreadPool_WorkerLoop, pSelf, 0, NULL)) {
			//(pSelf->freeThreadsCount)++;
		} else {
			break;
		}
	}
	if (!(pSelf->workerThreadsSemaphore = CreateSemaphore(NULL, i, i, NULL))) {
		DWORD err = GetLastError();
		return FALSE; // terminate thread

	}
	pSelf->totalThreadsCount = i;

	ResumeThread(pSelf->mainThread);
	return pSelf;//->mainThread;
}
