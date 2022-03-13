#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>

#include "header.h"
#include "window.h"

struct Swindow* active_window = NULL;
const char *window_gradient = " .:-=+*#%";

struct Swindow* get_active_window(void){
    return active_window;
}

bool working_with_windows (struct Swindow* head){
    assert(head);
    struct Swindow* current = head;

    while (current){
        if (current->moving || current->resizing)
            return true;
        current = current->next; 
    }
    return false;
}

void string_print_arr (Tarr arr, const char* string, uint32_t px, uint32_t py){
    assert(string);

    int pos = 0;
    while (*string != '\0'){
        arr[py][px+pos++] = *string++;
    }
}

void move_window (struct Swindow** tail, struct Swindow* p){
    assert(tail);
    assert(p);
    assert(p->next);
    if (p->next->next == NULL)
        return;
    struct Swindow* w = p->next;
    p->next = p->next->next;
    (*tail)->next = w;
    *tail = (w);
    w->next = NULL;
}

void set_active(struct Swindow* window, struct Swindow** tail){
    if (active_window == window->next)
        return;
    move_window(tail, window);
    struct Swindow* w = window->next;
    active_window = w;
    w->animation |= WA_SELECTED;
    *w->animation_data = w->sx/2 - 15/2;
}

static void select_animation (struct Swindow* win){
    const uint8_t fallof[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    size_t line_width = sizeof(fallof) / sizeof(fallof[0]);
    uint8_t linex = win->animation_data[0];

    memset(win->overlay, C_NAC, sizeof(win->overlay));
    for (uint32_t py = 3; py < win->sy; py++)
        for (uint32_t px = 0; px < line_width; px++){
            if (in_area(px+linex, 0, win->sx-1))
                win->overlay[py][px+linex] = window_gradient[fallof[px]];
            if (in_area(win->sx-(px+linex), 0, win->sx-1))
                win->overlay[py][win->sx-(px+linex)] = window_gradient[fallof[px]];
        }
    win->animation_data[0]+=2;
    if (win->animation_data[0] > win->sx){
        win->animation &= ~WA_SELECTED;
        memset(win->overlay, C_NAC, sizeof(win->overlay));
    }
}

static void loading_animation (struct Swindow* win){
    static double offset = 0.0;
    #define distance(x1,y1,x2,y2) (double)(sqrt( (((x1)-(x2)) * (((x1)-(x2)))) + (4 * (((y1)-(y2)) * (((y1)-(y2)))))));
    for (uint32_t py = 3; py < win->sy; py++)
        for (uint32_t px = 0; px < win->sx; px++){
            double dist = distance(px, py, win->sx/2, win->sy/2);
            uint8_t idx = fabs(cos(dist*0.2+offset))*strlen(window_gradient);
            win->overlay[py][px] = window_gradient[strlen(window_gradient)-1-idx];
        }
    offset -= 0.1;
    #undef distance
}

void animate (struct Swindow* win){
    if (win->animation & WA_LOADING)  loading_animation(win);
    if (win->animation & WA_SELECTED) select_animation(win);
}

void destroy (struct Swindow* window){
    assert(window);
    free(window->animation_data);
    free(window);
}

void put (struct Swindow* window, Tarr arr){
    assert(window);
    
    /* printing window data and left and right edges */
    for (int py = window->py; py < window->py+window->sy-1; py++){
        for (int px = window->px; px < window->px+window->sx; px++){
            arr[py][px] = window->data[py-window->py][px-window->px];
            if (window->overlay[py-window->py][px-window->px] != C_NAC)
                arr[py][px] = window->overlay[py-window->py][px-window->px];
            if (px == window->px || px == window->px+window->sx-1)
                arr[py][px] = UI_VERTICAL;
        }
    }
    /* printing up and down edge */
    for (int px = window->px; px < window->px+window->sx; px++){
        arr[window->py+window->sy-1][px] = UI_HORZIONTAL;
        arr[window->py][px] = UI_HORZIONTAL;
    }

    /* printing corners */
    arr[window->py][window->px] = UI_UPLEFT;
    arr[window->py+window->sy-1][window->px] = UI_DOWNLEFT;
    arr[window->py][window->px+window->sx-1] = UI_UPRIGHT;
    arr[window->py+window->sy-1][window->px+window->sx-1] = UI_DOWNRIGHT;

    /* printing window title and buttons */
    string_print_arr(arr, window->title, window->px+1, window->py+1);
    const char* buttons = "_ # X";
    string_print_arr(arr, buttons, window->sx+window->px-strlen(buttons)-1, window->py+1);

    /* printing deltas and title line */
    arr[window->py+2][window->px] = UI_DELTALEFT;
    arr[window->py+2][window->px+window->sx-1] = UI_DELTARIGHT;
    for (int i = window->px+1; i < window->px+window->sx-1; i++)
        arr[window->py+2][i] = UI_HORZIONTAL;
}

void put_all (struct Swindow* head, Tarr arr){
    assert(head);
    struct Swindow* tmp = head;
    while (tmp != NULL){
        head->put((struct Swindow*)tmp, arr);
        tmp = tmp->next;
    }
}

void destroy_all (struct Swindow* head){
    assert(head);
    struct Swindow* cur = (struct Swindow*)head;

    while (cur != NULL){
        struct Swindow* tmp = cur->next;
        cur->destroy((struct Swindow*)cur);
        cur = tmp;
    }
}

struct Swindow* add_by_head (struct Swindow* head, struct Swindow* win){
    assert(head);
    assert(win);
    win->next = head;
    return win;
}

struct Swindow* window_create (const char *title, uint32_t sx, uint32_t sy, void (*program)(struct Swindow*)){
    assert(title);
    assert(sx);
    assert(sy);

    static uint32_t px = 0, py = 0;
    static uint16_t id = 1;
    struct Swindow* ret = malloc(sizeof(struct Swindow));
    strcpy(ret->title, title);
    memset(ret->data, C_EMPTY, sizeof(ret->data));
    memset(ret->overlay, C_NAC, sizeof(ret->overlay));
    ret->animation_data = malloc(10*sizeof(uint8_t));
    memset(ret->animation_data, 0, 10*sizeof(uint8_t));
    ret->animation = 0x0;
    ret->sx = sx;
    ret->sy = sy;
    ret->px = px;
    ret->py = py;
    ret->ox = 0;
    ret->oy = 0;
    ret->moving = false;
    ret->resizing = false;
    ret->next = NULL;
    ret->id = id;
        
    ret->animate = animate;
    ret->create = window_create;
    ret->destroy = destroy;
    ret->destroy_all = destroy_all;
    ret->put = put;
    ret->put_all = put_all;
    ret->add_by_head = add_by_head;
    ret->set_active = set_active;
    ret->program = program;

    active_window = ret;

    px += 2;
    py += 2;
    ++id;
    return ret;
}