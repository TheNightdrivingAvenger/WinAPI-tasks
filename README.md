# DLL memory walker

OSaSPLab3 contains a simple DLL that exports only one function, which searches all the Virtual Address Space of the process for a string and replaces it with another one. OSaSPLab3App uses this dll. ifdef's in main application are used to quickly switch between two types of linking: implicit and excplicit.

# Remote DLL memory walker
OSaSPLab3Remote contains a the same DLL as above, but function signature is changed to be the same as PTHREAD_START_ROUTINE (to start a remote thread from this function). OSaSPLab3RemoteApp uses this DLL (only explicit linking with LoadLibrary) for a technique called DLL injection via remote thread. OSaSPLab3TargetApp is the target app for the remote thread and string replacing.

Only basic error management is used, and this code with hardcoded constants is not pretty, but it gives a feeling of how it probably should be.

# Lab4: thread pool (and using it to sort text file)
Implementation of a threadpool using [this queue](https://github.com/TheNightdrivingAvenger/blockingqueue) for tasks (queue implementation is included in this project too). This pool doesn't use synchronization primitives for every thread (and that decreases resources overhead), and doesn't put "NULL"s in the queue to finish its work (so its a tiny bit faster and less memory consuming).