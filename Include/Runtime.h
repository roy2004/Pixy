/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdint.h>

#include "Noreturn.h"


#ifdef __cplusplus
extern "C" {
#endif

int FiberMain(int argc, char **argv);
int AddFiber(void (*function)(uintptr_t), uintptr_t argument);
int AddAndRunFiber(void (*function)(uintptr_t), uintptr_t argument);
void YieldCurrentFiber(void);
void ExitCurrentFiber(void) NORETURN;
int SleepCurrentFiber(int duration);

#ifdef __cplusplus
} // extern "C"
#endif
