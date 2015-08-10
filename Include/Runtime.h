#pragma once


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

int Main(int argc, char **argv);
int Call(void (*coroutine)(uintptr_t), uintptr_t argument);
void Yield(void);
void Exit(void);
int Sleep(int duration);

#ifdef __cplusplus
} // extern "C"
#endif
