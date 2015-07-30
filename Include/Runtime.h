#pragma once


#include "Base.h"


bool Call(void (*coroutine)(any_t), any_t argument);
void Yield(void);
bool Sleep(int duration);
