#define UNICODE

#define __STDC_WANT_LIB_EXT1__ 1

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

#include "headers\threadpool.h"

typedef struct tagWORKERPARAM {
	char **firstBlockElem;
	int blockSizeInElems;
} WORKERPARAM, *PWORKERPARAM;

// TODO: deal with volatile and alignas

char **readAllLines(HANDLE hFile, int MAXLINELEN, int *linesCount)
{
	*linesCount = 0;
	BOOL endOfFile = FALSE;
	char **result = HeapAlloc(GetProcessHeap(), 0, sizeof(char *));
	if (!result) return NULL;

	for (int i = 0; !endOfFile; i++) {
		DWORD curFilePointer = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
		result = HeapReAlloc(GetProcessHeap(), 0, result, sizeof(char *) * (i + 1));
		if (!result) {
			// error; must free the mem or return what was read successfully
			return NULL;
		}
		result[i] = HeapAlloc(GetProcessHeap(), 0, MAXLINELEN);
		if (!result[i]) {
			// error; as above
			return NULL;
		}

		BOOL lineFound = FALSE;
		DWORD bytesRead;
		BOOL res = ReadFile(hFile, result[i], MAXLINELEN, &bytesRead, NULL);
		if (res && !bytesRead) {
			endOfFile = TRUE;
		} else if (res && bytesRead) {
			int j;
			for (j = 0; (j < MAXLINELEN - 1) && !lineFound; j++) {
				if ((result[i][j] == '\r') && (result[i][j + 1] == '\n')) {
					lineFound = TRUE;
					SetFilePointer(hFile, curFilePointer + j + 2, NULL, FILE_BEGIN);
				}
			}

			if (lineFound) {
				HeapReAlloc(GetProcessHeap(), 0, result[i], j + 2);
				result[i][j + 1] = '\0';
				(*linesCount)++;
			} else {
				// no line or it was larger than MAXLINELEN. Error
				return NULL;
			}
		} else {
			// reading error
			return NULL;
		}
	}
	return result;
}

HANDLE createTargetFile(const wchar_t *const fileName)
{
	HANDLE file = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
	if (file == INVALID_HANDLE_VALUE) return NULL;
	return file;
}

HANDLE openSourceFile(const wchar_t *const fileName)
{
	HANDLE file = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (file == INVALID_HANDLE_VALUE) return NULL;
	return file;
}

// data isn't used
int ordinalStringCompare(const char **ptr1, const char **ptr2, void *data)
{
	int i = 0;
	const char *str1 = *ptr1;
	const char *str2 = *ptr2;
	BOOL str1Ended = (str1[0] == '\0'), str2Ended = (str2[0] == '\0');
	while (!str1Ended && !str2Ended) {
		if (str1[i] < str2[i]) {
			return -1;
		} else if (str1[i] > str2[i]) {
			return 1;
		}
		i++;
	}
	if (str1Ended && !str2Ended) return -1;
	else if (!str1Ended && str2Ended) return 1;
	return 0;
}

// need to get ptr to its array block and block's size (in array elements)
int workerSortTask(PWORKERPARAM param)
{
	qsort_s(param->firstBlockElem, param->blockSizeInElems, sizeof(char *), ordinalStringCompare, NULL);
	return 0;
}

void releaseFunc(PWORKITEM work)
{
	HeapFree(GetProcessHeap(), 0, work);
}

int wmain(int argc, wchar_t **argv)
{
	if (argc != 3) {
		printf("Invalid arguments count\n");
		return -1;
	}

	int POOL_SIZE = 3;
	PTHREADPOOL pool = ThreadPool_Create(POOL_SIZE);
	if (!pool) return -1;

	HANDLE srcFile = openSourceFile(argv[1]);
	HANDLE targetFile = createTargetFile(argv[2]);
	if (!srcFile || !targetFile) return -1;

	int linesCount;
	char **allLines = readAllLines(srcFile, 2048, &linesCount);
	CloseHandle(srcFile);

	int pieceSize = linesCount / POOL_SIZE;

	for (int i = 0; i < linesCount; i += pieceSize) {
		PWORKITEM item = HeapAlloc(GetProcessHeap(), 0, sizeof(WORKITEM));
		item->callable = workerSortTask;
		item->param = HeapAlloc(GetProcessHeap(), 0, sizeof(WORKERPARAM));
		((PWORKERPARAM)item->param)->firstBlockElem = allLines + i;
		((PWORKERPARAM)item->param)->blockSizeInElems = min(pieceSize, linesCount - i);
		ThreadPool_AddTask(pool, item);
	}

	ThreadPool_Stop(pool, releaseFunc);
	DWORD written;
	
	qsort_s(allLines, linesCount, sizeof(char *), ordinalStringCompare, NULL);

	for (int i = 0; i < linesCount; i++) {
		WriteFile(targetFile, allLines[i], strlen(allLines[i]), &written, NULL);
	}
	return 0;
}
