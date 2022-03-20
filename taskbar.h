#ifndef _TASKBARH_
#define _TASKBARH_

#include "window.h"
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

void taskbar_push (struct Swindow * const);
struct Swindow *taskbar_pop(void);
bool taskbar_in (const struct Swindow * const);
void taskbar_put (Tarr * const);
struct Swindow *taskbar_cursorwindow(uint32_t cx, uint32_t cy);
void taskbar_unwrap(void);

#endif