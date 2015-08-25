/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdbool.h>
#include <stdint.h>

#include "Noreturn.h"


#if defined __cplusplus
extern "C" {
#endif

int FiberMain(int argc, char **argv);
bool AddFiber(void (*function)(uintptr_t), uintptr_t argument);
bool AddAndRunFiber(void (*function)(uintptr_t), uintptr_t argument);
void YieldCurrentFiber(void);
NORETURN void ExitCurrentFiber(void);
bool SleepCurrentFiber(int duration);

#if defined __cplusplus
} // extern "C"
#endif
