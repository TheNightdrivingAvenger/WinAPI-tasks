/****************************************************************************
 *                                                                          *
 * File    : dllmain.c                                                      *
 *                                                                          *
 * Purpose : Generic Win32 Dynamic Link Library (DLL).                      *
 *                                                                          *
 * History : Date      Reason                                               *
 *           00/00/00  Created                                              *
 *                                                                          *
 ****************************************************************************/

#define WIN32_LEAN_AND_MEAN  /* speed up */
#include <windows.h>

/*
 * Include our "interface" file and define a symbol
 * to indicate that we are *building* the DLL.
 */
#define _OSASPLAB_
#include "OSASPLAB.h"

/****************************************************************************
 *                                                                          *
 * Function: DllMain                                                        *
 *                                                                          *
 * Purpose : DLL entry and exit procedure.                                  *
 *                                                                          *
 ****************************************************************************/

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            /*
             * Microsoft says:
             *
             * The DLL is being loaded into the virtual address space of the current
             * process as a result of the process starting up or as a result of a call
             * to LoadLibrary. DLLs can use this opportunity to initialize any instance
             * data or to use the TlsAlloc function to allocate a thread local storage
             * (TLS) index.
             */
            break;

        case DLL_THREAD_ATTACH:
            /*
             * Microsoft says:
             *
             * The current process is creating a new thread. When this occurs, the system
             * calls the entry-point function of all DLLs currently attached to the process.
             * The call is made in the context of the new thread. DLLs can use this opportunity
             * to initialize a TLS slot for the thread. A thread calling the DLL entry-point
             * function with DLL_PROCESS_ATTACH does not call the DLL entry-point function
             * with DLL_THREAD_ATTACH.
             *
             * Note that a DLL's entry-point function is called with this value only by threads
             * created after the DLL is loaded by the process. When a DLL is loaded using
             * LoadLibrary, existing threads do not call the entry-point function of the newly
             * loaded DLL.
             */
            break;

        case DLL_THREAD_DETACH:
            /*
             * Microsoft says:
             *
             * A thread is exiting cleanly. If the DLL has stored a pointer to allocated memory
             * in a TLS slot, it should use this opportunity to free the memory. The system calls
             * the entry-point function of all currently loaded DLLs with this value. The call
             * is made in the context of the exiting thread.
             */
            break;

        case DLL_PROCESS_DETACH:
            /*
             * Microsoft says:
             *
             * The DLL is being unloaded from the virtual address space of the calling process as
             * a result of unsuccessfully loading the DLL, termination of the process, or a call
             * to FreeLibrary. The DLL can use this opportunity to call the TlsFree function to
             * free any TLS indices allocated by using TlsAlloc and to free any thread local data.
             *
             * Note that the thread that receives the DLL_PROCESS_DETACH notification is not
             * necessarily the same thread that received the DLL_PROCESS_ATTACH notification.
             */
            break;
    }

    /* Return success */
    return TRUE;
}

// length with \0 symbol
DWORD WINAPI GetStringLength(const char *const string)
{
	int result = 0;
	while (string[result++]);
	return result;
}

// 0 -- OK
// 1 -- no string found
// -1 -- strings are different in size
// it is obvious that strings must be of the same size
OSASPLABAPI int WINAPI FindAndReplaceString(const char *const findWhat, const char *const replaceWith)
{
	DWORD findWhatLength = GetStringLength(findWhat);
	DWORD replaceWithLength = GetStringLength(replaceWith);
	if (findWhatLength != replaceWithLength) return -1;

	SIZE_T queryResult = 1;
	char *currentAddress = 0;
	MEMORY_BASIC_INFORMATION mbi;
	while (queryResult) {
		queryResult = VirtualQuery(currentAddress, &mbi, sizeof(mbi));
		if (queryResult && (mbi.State == MEM_COMMIT) && (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE
				|| mbi.Protect == PAGE_EXECUTE_WRITECOPY || mbi.Protect == PAGE_WRITECOPY)) {
			// this memory has been allocated and is readable and writeable, so it may contain the string
			// this also means that I can replace it
			DWORD stringPos = 0;
			for (DWORD i = 0; i < mbi.RegionSize - findWhatLength; i++) {
				if ((findWhat[stringPos] == ((char *)mbi.BaseAddress)[i]) && (stringPos < findWhatLength)) stringPos++;
				else stringPos = 0;
				
				if (stringPos == findWhatLength) {
					memcpy((char *)mbi.BaseAddress + i - findWhatLength + 1, replaceWith, findWhatLength);
					return 0;
				}
			}
		}
		currentAddress += mbi.RegionSize;
	}
	return 1;
}
