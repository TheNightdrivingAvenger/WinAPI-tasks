# DllMemoryWalk

A simple DLL that exports only one function, which searches all the Virtual Address Space of the process for a string and replaces it with another one. ifdef's in main application are used to quickly switch between two types of linking: implicit and excplicit.
