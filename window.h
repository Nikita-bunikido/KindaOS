#ifndef _WINDOWH_
#define _WINDOWH_

#include <inttypes.h>
#include <stdbool.h>
#include "header.h"

#define WA_SELECTED      (uint16_t)(0x1 << 0)
#define WA_LOADING       (uint16_t)(0x1 << 1)
#define WA_NOTRESPONDING (uint16_t)(0x1 << 2)

#define taskbar_button_positionX(win)  ((win)->sx+(win)->px-6)
#define fullmode_button_positionX(win) ((win)->sx+(win)->px-4)
#define close_button_positionX(win)    ((win)->sx+(win)->px-1)
#define taskbar_button_positionY(win)  ((win)->py+1)
#define fullmode_button_positionY(win) taskbar_button_positionY(win)
#define close_button_positionY(win)    taskbar_button_positionY(win)

#define selfarg     struct Swindow*

typedef struct Swindow {
    bool moving;
    bool resizing;
    bool vis;
    char title[255];
    unsigned char data[RES_H][RES_W];
    unsigned char overlay[RES_H][RES_W];
    uint32_t px, py;
    uint32_t sx, sy;
    uint32_t ox, oy;
    uint16_t id;
    uint16_t animation : 16;
    struct Swindow* next;
    uint8_t* animation_data;

    struct Swindow *(*create)(const char*, uint32_t, uint32_t, void(*)(struct Swindow*));
    void (*animate)          (selfarg);
    void (*destroy)          (selfarg);
    void (*destroy_all)      (selfarg);
    void (*put)              (selfarg, Tarr);
    void (*put_all)          (selfarg, Tarr);
    void (*set_active)       (selfarg, struct Swindow**);
    void (*set_active_hard)  (selfarg);
    void (*program)          (selfarg);
    struct Swindow* (*add_by_head)(selfarg, struct Swindow*);
} Twindow;
#undef selfarg

struct Swindow* window_create        (const char *title, uint32_t sx, uint32_t sy, void (*program)(struct Swindow*));
void            move_window          (struct Swindow** tail, struct Swindow* p);
struct Swindow* get_active_window    ();
void            string_print_arr     (Tarr arr, const char* string, uint32_t px, uint32_t py);
bool            working_with_windows (struct Swindow* head);
#endif