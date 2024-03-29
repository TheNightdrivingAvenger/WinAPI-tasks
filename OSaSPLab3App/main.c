#include <windows.h>
#include <stdio.h>

//#define IMPLICITLINK

#ifdef IMPLICITLINK
#include "OSaSPLab.h"

int main(int argc, char **args)
{
	printf("Implicit dynamic linking\n");
	char *target = HeapAlloc(GetProcessHeap(), 0, 10);
	memcpy(target, "I am they", 10);
	//char *source = HeapAlloc(GetProcessHeap(), 0, 10);
	//memcpy(source, "I am notm", 10);
	char source[10] = { "I am notm" };
	int res = FindAndReplaceString(target, source);
	if (res == 0) printf("`%s`\n", target);
	else if (res == 1) printf("target string was not found\n");
	else if (res == -1) printf("strings are different in size, valid exchange is not possible\n");
	getchar();
	return 0;
}

#else

int main(int argc, char **args)
{
	printf("Explicit dynamic linking\n");
	char *target = HeapAlloc(GetProcessHeap(), 0, 10);
	memcpy(target, "I am they", 10);
	char *source = HeapAlloc(GetProcessHeap(), 0, 10);
	memcpy(source, "I am notm", 10);

	HMODULE lib = LoadLibraryW(L"OSaSPLab3.dll");
	int res = -2;
	if (lib != NULL) {
		int(*WINAPI proc)(const char *const, const char *const) = GetProcAddress(lib, "_FindAndReplaceString@8");
		if (proc != NULL) {
			res = proc(target, source);
		}
	}
	FreeLibrary(lib);
	if (res == 0) printf("`%s`\n", target);
	else if (res == 1) printf("target string was not found\n");
	else if (res == -1) printf("strings are different in size, valid exchange is not possible\n");
	getchar();
	return 0;
}

#endif
