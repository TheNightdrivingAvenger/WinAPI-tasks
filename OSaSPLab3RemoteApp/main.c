
#include <windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include "OSASPLAB.h"

static char *dllName = "F:\\Pelles C Projects\\OSaSPLab3Full\\OSaSPLab3Remote\\OSaSPLab3Remote.dll";

DWORD ejectDll(HANDLE targetProcess, HMODULE dll)
{
	DWORD result = 0;
	PTHREAD_START_ROUTINE freeLibRoutine = GetProcAddress(GetModuleHandle("Kernel32.dll"), "FreeLibrary");
	if (!freeLibRoutine) goto cleanUp;
	HANDLE remoteThread = CreateRemoteThread(targetProcess, NULL, 0, freeLibRoutine, dll, 0, NULL);
	if (!remoteThread) goto cleanUp;
	WaitForSingleObject(remoteThread, INFINITE);
	if (!GetExitCodeThread(remoteThread, &result)) goto cleanUp;

cleanUp:
	if (remoteThread) CloseHandle(remoteThread);
	return result;
}

DWORD callProcInRemoteProcess(HANDLE targetProcess, DWORD injectionAddr, DWORD procDelta, char *findWhat, char *replaceWith)
{
	DWORD result = -1;

	char *foreignFindWhatAddr = VirtualAllocEx(targetProcess, NULL, strlen(findWhat) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (!foreignFindWhatAddr) goto cleanUp;
	char *foreignReplaceWithAddr = VirtualAllocEx(targetProcess, NULL, strlen(replaceWith) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (!foreignReplaceWithAddr) goto cleanUp;
	if (!WriteProcessMemory(targetProcess, foreignFindWhatAddr, findWhat, strlen(findWhat) + 1, NULL)) goto cleanUp;
	if (!WriteProcessMemory(targetProcess, foreignReplaceWithAddr, replaceWith, strlen(replaceWith) + 1, NULL)) goto cleanUp;

	INJECTEDPROCPARAMS params = { foreignFindWhatAddr, foreignReplaceWithAddr };
	PINJECTEDPROCPARAMS foreignParamsAddr = VirtualAllocEx(targetProcess, NULL, sizeof(INJECTEDPROCPARAMS), MEM_COMMIT, PAGE_READWRITE);
	if (!WriteProcessMemory(targetProcess, foreignParamsAddr, &params, sizeof(INJECTEDPROCPARAMS), NULL)) goto cleanUp;

	HANDLE remoteThread = CreateRemoteThread(targetProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(injectionAddr + procDelta), foreignParamsAddr, 0, NULL);
	if (!remoteThread) goto cleanUp;
	WaitForSingleObject(remoteThread, INFINITE);
	if (!GetExitCodeThread(remoteThread, &result)) goto cleanUp;

cleanUp:
	if (foreignFindWhatAddr) VirtualFreeEx(targetProcess, foreignFindWhatAddr, 0, MEM_RELEASE);
	if (foreignReplaceWithAddr) VirtualFreeEx(targetProcess, foreignReplaceWithAddr, 0, MEM_RELEASE);
	if (foreignParamsAddr) VirtualFreeEx(targetProcess, foreignParamsAddr, 0, MEM_RELEASE);
	if (remoteThread) CloseHandle(remoteThread);
	return result;
}

// returns Dll's handle (virtual address) in foreign addr space
DWORD InjectDll(DWORD targetProcessID, HANDLE *outRemoteProcess)
{
	DWORD result = 0;
	HANDLE remoteProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, targetProcessID);
	if (!remoteProcess) {
		return result;
	}
	*outRemoteProcess = remoteProcess;

	int nameLen = strlen(dllName) + 1;
	void *foreignMemSpace = VirtualAllocEx(remoteProcess, NULL, nameLen, MEM_COMMIT, PAGE_READWRITE);
	if (!foreignMemSpace) goto cleanUp; // couldn't allocate memory
	SIZE_T written;
	if (!WriteProcessMemory(remoteProcess, foreignMemSpace, dllName, nameLen, &written)) goto cleanUp; // couldn't write process memory
	PTHREAD_START_ROUTINE loadLibroutine = GetProcAddress(GetModuleHandle("Kernel32.dll"), "LoadLibraryA");
	if (!loadLibroutine) goto cleanUp; // couldn't get LoadLibrary proc address
	HANDLE remoteThread = CreateRemoteThread(remoteProcess, NULL, 0, loadLibroutine, foreignMemSpace, 0, NULL);
	if (!remoteThread) goto cleanUp; // coudln't create remote thread
	WaitForSingleObject(remoteThread, INFINITE);
	if (!GetExitCodeThread(remoteThread, &result)) goto cleanUp;

cleanUp:
	if (foreignMemSpace) VirtualFreeEx(remoteProcess, foreignMemSpace, 0, MEM_RELEASE);
	if (remoteThread) {
		CloseHandle(remoteThread);
	}
	return result;
}

DWORD getProcAddrDelta(void)
{
	HMODULE dll = LoadLibraryA(dllName);
	DWORD plainAddr = (DWORD)GetProcAddress(dll, "_FindAndReplaceString@4");
	DWORD result = plainAddr - (DWORD)dll;
	FreeLibrary(dll);
	return result;
}

int main(int argc, char **argv)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		// skips the first process, who cares
		while (Process32Next(snapshot, &entry) == TRUE)
		{
            if (strcmp(entry.szExeFile, "OSaSPLab3TargetApp.exe") == 0)
            {
				HANDLE remoteProc = NULL;
				DWORD injectedDllAddr = InjectDll(entry.th32ProcessID, &remoteProc);
				printf("Injected DLL address in remote process: 0x%lX\n", injectedDllAddr);
				DWORD delta = getProcAddrDelta();
				printf("Offset of `FindAndReplaceString` in the DLL: 0x%lX\n", delta);
				int callResult = callProcInRemoteProcess(remoteProc, injectedDllAddr, delta, "I'm they", "I'm notm");
				printf("Called DLL function returned 0x%lX (F..F -- function was not called, something prior to its calling failed)\n", callResult);
				DWORD ejectResult = ejectDll(remoteProc, (HMODULE)injectedDllAddr);
				printf("Eject DLL returned 0x%lX\n", ejectResult);
				CloseHandle(remoteProc);
				break;
			}
		}
	}
    CloseHandle(snapshot);

    return 0;
}
