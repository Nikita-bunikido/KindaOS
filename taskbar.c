#include <stdio.h>
#include <math.h>
#include "window.h"
#include "taskbar.h"

#define TASKBAR_STACKCAP    256

struct cursor { uint32_t x, y; };
extern struct cursor cursor;
extern bool key_down (int);
static double t = 0x0ULL;
static bool was_unwrap = false;

static struct taskbar {
    struct Swindow * stack[TASKBAR_STACKCAP];
    size_t sp;
    size_t ss;
    size_t height;
} taskbar = {.stack = {0},
             .sp = 0,
             .ss = 0,
             .height = 2,
            };

static const size_t taskbar_gradient_sz = 17ULL;

static inline unsigned char get_taskbar_gradient (double sres){
    sres = fabs(sres) * taskbar_gradient_sz;
    if (sres < 0) sres = 0;
    if (sres > taskbar_gradient_sz-1) sres = taskbar_gradient_sz-1;
    return  " .:-=+*#%#*+=-:. "[(size_t)sres];
}

static void delim_draw (Tarr * const src, ssize_t x, ssize_t y, size_t len){
    for (size_t h = 0; h < len; h++)
        (*src)[h+y][x] = '|';
}

void taskbar_push (struct Swindow * const win){ taskbar.stack[taskbar.sp++] = win; }
struct Swindow *taskbar_pop(void){ return taskbar.stack[--taskbar.sp]; }

bool taskbar_in (const struct Swindow * const win){
    for (size_t w = 0; w < taskbar.sp; w++)
        if (taskbar.stack[w] == win) return true;
    return false;
}

void taskbar_put (Tarr * const out){
    for (ssize_t dx = 0; dx < RES_W-1; ++dx)
    for (ssize_t dy = RES_H-1; dy >= RES_H-taskbar.height; --dy){
        (*out)[dy][dx] = (unsigned char)219;
    }

    const ssize_t delim_step = taskbar.sp != 0 ? (RES_W) / taskbar.sp : 0;

    if (taskbar.sp == 0) return;

    if (cursor.y >= RES_H-taskbar.height){
        taskbar.ss = cursor.x / delim_step;
        for (ssize_t dx = taskbar.ss*delim_step; dx < (taskbar.ss+1)*delim_step; ++dx)
        for (ssize_t dy = RES_H-1; dy >= RES_H-taskbar.height; --dy){
            (*out)[dy][dx] = get_taskbar_gradient(sin(dx*0.1+t));
        }

        /* printing window title */
        extern void string_print_arr (Tarr, const char*, uint32_t, uint32_t);
        string_print_arr(*out, taskbar.stack[taskbar.ss]->title, cursor.x-strlen(taskbar.stack[taskbar.ss]->title)/2, cursor.y-1);
    }

    for (ssize_t i = 0; i < RES_W; i += delim_step)
        delim_draw(out, i, RES_H-taskbar.height, taskbar.height);
    delim_draw(out, RES_W-1, RES_H-taskbar.height, taskbar.height);

    t += 0.1;
}

void taskbar_unwrap(void){
    if(cursor.y < RES_H-taskbar.height || !taskbar.sp) return;
    
    if (was_unwrap == false && key_down(' ')){
        taskbar.stack[taskbar.ss]->vis = true;
        for (size_t i = taskbar.ss; i < taskbar.sp-1; i++)
            taskbar.stack[i] = taskbar.stack[i+1];
        --taskbar.sp;
        was_unwrap = true;
    } else if (!key_down(' '))
        was_unwrap = false;
}