#include <windows.h>
#include <stdio.h>

int main(void)
{
	char *str = HeapAlloc(GetProcessHeap(), 0, 10);
	memcpy(str, "I'm they", 9);
	while (1) {
		printf("%s\n", str);
		Sleep(2000);
	}
	return 0;
}
