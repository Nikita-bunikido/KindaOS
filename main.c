#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "header.h"
#include "window.h"
#include "taskbar.h"

#define NDEBUG
#include <assert.h>

bool keys[255] = {0};

struct cursor {
    uint32_t x, y;
} cursor = {0,0};

#define cursor_in(_x,_y)\
    ((cursor.x == (_x)) && (cursor.y == (_y)))

struct {
    enum {
        ACTIVE,
        DEACTIVE,
        HOVER
    } state;
    enum {
        UP,
        DOWN,
        LEFT,
        RIGHT
    } position;
    int height;
    unsigned char cdraw[3];
    struct Swindow* stack[255];
    uint32_t stack_pos;
} task_bar = {ACTIVE, DOWN, 3, (unsigned char[]){219, 219, 219}};

Tarr output;

void cpy_arr (Tarr to, Tarr source, unsigned char mask){
    for (unsigned py = 0; py < RES_H; py++)
        for (unsigned px = 0; px < RES_W; px++)
            if (source[py][px] != mask)
                to[py][px] = source[py][px];
}

void show_arr (Tarr arr, FILE *stream){
    for (unsigned py = 0; py < RES_H; ++py)
        fprintf(stream, "%s\n", arr[py]);
}

bool key_down (int k){
    return GetKeyState(k) < 0;
}

void draw_output (Tarr arr){
    arr[cursor.y][cursor.x] = C_CURSOR;

    /* setting strings end characters */
    for (unsigned py = 0; py < RES_H; py++){
        arr[py][RES_W] = (unsigned char)'\0';
    }
    /* move cursor to begin */
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), (COORD){0,0});
    show_arr(arr, stdout);
}

void arr_from_file (Tarr arr, FILE* stream){
    if (!stream) memset(arr, 'E', sizeof(Tarr));

    int w = -0xFFFFF, h = 0;
    int cnt = 0;
    char c;

    while (!feof(stream)){
        cnt++;
        c = fgetc(stream);
        if (c == '\n'){
            if (cnt > w)
                w = cnt;
            cnt = 0;
            h++;
        }
    }

    w--;
    fseek(stream, 0, 0);
    memset(arr, C_EMPTY, sizeof(Tarr));

    for (int py = 0; py < h; py++)
        for (int px = 0; px < w; px++){
            unsigned char c = fgetc(stream);
            if (c =='\n')
                c = fgetc(stream);
            arr[py][px] = c;
        }
    
}

void user_input(){
    if (key_down(VK_LEFT)) cursor.x  -= (cursor.x != 0);
    if (key_down(VK_RIGHT)) cursor.x += (cursor.x != RES_W-1);
    if (key_down(VK_UP)) cursor.y    -= (cursor.y != 0);
    if (key_down(VK_DOWN)) cursor.y  += (cursor.y != RES_H-1);
}

void update(struct Swindow* head, struct Swindow** tail){
    /* updating taskbar state */
    switch (task_bar.position){
    case DOWN:
        if (cursor.y > RES_H-task_bar.height)
            task_bar.state = HOVER;
        else
            task_bar.state = ACTIVE;
        break;
    default:
        break;
    }

    /* activate window */
    struct Swindow* tmp = head;
    while (tmp->next != NULL){
        if (!working_with_windows(head) && key_down(' ') && (cursor.x >= tmp->next->px)
            && (cursor.x < tmp->next->px+tmp->next->sx)
            && (cursor.y >= tmp->next->py)
            && (cursor.y < tmp->next->py+3)){
                tmp->set_active(tmp, tail);
                break;
            }
        tmp = tmp->next;
    }

    tmp = get_active_window();

    /* to taskbar '_' button*/
    if (key_down(' ') && cursor_in(
        taskbar_button_positionX(tmp),
        taskbar_button_positionY(tmp)) &&
        !taskbar_in( tmp ))
    {
        taskbar_push( tmp );
        tmp->vis = false;
        tmp->set_active_hard (*tail);
    }

    /* open full screen selected window '#' button */
    if (key_down(' ') && cursor_in(
        fullmode_button_positionX(tmp),
        fullmode_button_positionY(tmp)))
    {
            if (tmp->px == 0 && tmp->py == 0 && tmp->sx == RES_W && tmp->sy == RES_H-task_bar.height){
                tmp->sx = 20;
                tmp->sy = 10;
                tmp->px = 5;
                tmp->py = 5;
            } else {
                tmp->px = 0;
                tmp->py = 0;
                tmp->sx = RES_W;
                tmp->sy = RES_H-task_bar.height;
            }
            return;
    }

    /* move selected window */

    if (!tmp->moving && key_down(' ') && (cursor.x >= tmp->px)
        && (cursor.x < tmp->px+tmp->sx)
        && (cursor.y >= tmp->py)
        && (cursor.y < tmp->py+3)){
            tmp->ox = cursor.x - tmp->px;
            tmp->oy = cursor.y - tmp->py;
            tmp->moving = true;
        }
    if (tmp->moving){
        if ((cursor.x - tmp->ox) >= 0)
            tmp->px = cursor.x - tmp->ox;
        if ((cursor.y - tmp->oy) >= 0)
            tmp->py = cursor.y - tmp->oy;
        if (!key_down(' ')){
            tmp->moving = false;
            memset(tmp->overlay, C_NAC, sizeof(tmp->overlay));
        }
    }

    /* resize selected window */
    if (!tmp->resizing && key_down(' ') && (cursor.x == tmp->px+tmp->sx-1 && cursor.y == tmp->py+tmp->sy-1)){
        tmp->resizing = true;
        tmp->animation |= WA_LOADING;
    }
    if (tmp->resizing){
        tmp->sx = cursor.x - tmp->px + 1;
        tmp->sy = cursor.y - tmp->py + 1;
        if (!key_down(' ')){
            tmp->resizing = false;
            tmp->animation &= ~WA_LOADING;
            memset(tmp->overlay, C_NAC, sizeof(tmp->overlay));
        }
    }   
    
    /* program execute */
    if (tmp->vis && tmp->program && !working_with_windows(head))
        tmp->program(tmp);
    else
        keys[(int)' '] = false;

    /* taskbar unwrap */
    taskbar_unwrap();
}

static void draw_id(Twindow* head){
    while (head != NULL){
        printf("%"PRId16" - ", head->id);
        head = head->next;
    }
    printf("\n");
}

static void load_frames (FILE *f, Tarr *frames){
    assert(f);
    assert(frames);

    char buf[RES_W+1];
    int line = 0, frame = 0;

    while (!feof(f) && frame < 100){
        fread(buf, 1, RES_W+1, f); 
        buf[RES_W] = '\0'; 
        memcpy(frames[frame][line], buf, RES_W+1);
        line++; 
        if (line > RES_H-1){
            line = 0;
            frame++;
        }
    }
}

static void play_intro (const char *fname){
    FILE *intro = fopen(fname, "r");
    if (intro == NULL){
        fprintf(stderr, "error: can\'t load intro.\n");
        assert(0 && "ECNT_INRO");
    }

    /* setting output with empty characters */
    memset(output, C_EMPTY, sizeof(output));

    Tarr frames[100];
    load_frames(intro, frames);
    fclose(intro);
    uint64_t i = 0;

    for (long iter = 0; iter < 1; iter++){
        i = 0;
        do {
            draw_output(frames[i++]);
            Sleep(30);
        } while (i < 100);
    }
    Sleep(300);
}

struct key_comb {
    int key;
    bool shift;
};
struct key_comb get_key_comb (const int c){

    static struct key_comb spec_combs[255] = {
        [' '] = {32, 0},
        ['!'] = {49, 1},
        ['\\']= {220,0},
        ['\"']= {222,1},
        ['#'] = {51, 1},
        ['$'] = {52, 1},
        ['%'] = {53, 1},
        ['&'] = {55, 1},
        ['\'']= {222,0},
        ['('] = {57, 1},
        [')'] = {48, 1},
        ['*'] = {56, 1},
        ['+'] = {187,1},
        [','] = {188,0},
        ['-'] = {189,0},
        ['.'] = {190,0},
        ['/'] = {191,0},
        [':'] = {186,1},
        [';'] = {186,0},
        ['<'] = {188,1}, //not
        ['='] = {187,0},
        ['>'] = {190,1}, //not
        ['?'] = {191,1}, //not
        ['['] = {219,0},
        [']'] = {221,0},
        ['^'] = {54, 1}, //not
        ['_'] = {189,1}, //not
        ['`'] = {192,0},
        ['@'] = {50, 1}, //not
        ['{'] = {219,1}, //not
        ['|'] = {220,1}, //not
        ['}'] = {221,1}, //not
        ['~'] = {192,1}  //not
    };

    if (c >= 'A' && c <= 'Z')
        return (struct key_comb){
            .key = c,
            .shift = true
        };
    else if (c >= 'a' && c <= 'z')
        return (struct key_comb){
            .key = toupper(c),
            .shift = false
        };
    else if (isdigit(c))
        return (struct key_comb){
            .key = c,
            .shift = false
        };
    else return spec_combs[c];
}

int main(int argc, char** argv){
    system("cls");
    memset(task_bar.stack, 0, sizeof(task_bar.stack));

    FILE* freestr = fopen("sys\\.reg", "a");
    FILE* fconfig = fopen("sys\\.config", "r");

    FILE* fwall = fopen("sys\\wp\\wall.wp", "r");
    Tarr wall;
    arr_from_file(wall, fwall);

    Twindow* head = NULL;
    Twindow* tmp = NULL;
    Twindow* tail = NULL;

    head = tmp = (Twindow*)window_create("__ROOT__", 20, 10, NULL);
    head->vis = false;

    for (int u = 1; u < 3; u++){
        static char resultb[255], itoab[255];
        memset(itoab, 0, 255);
        sprintf(resultb, "window ");
        itoa(u, itoab, 10);
        strcat(resultb, itoab);
        tmp->next = (Twindow*)window_create(resultb, 20, 10, NULL);
        tmp = tmp->next;
    }

    void kinda_text (struct Swindow * );
    tmp->next = (Twindow*)window_create("kindaText", 20, 10, &kinda_text);
    tmp = tmp->next;
    tail = tmp;

    #ifndef NDEBUG
        play_intro("sys\\intro.txt");
        Sleep(100);
    #endif

    /* ================================= */
    while (!key_down('Q')){
        /* setting output with empty characters */
        memset(output, C_EMPTY, sizeof(output));
        /* coping wallpaper to output */
        cpy_arr(output, wall, C_NAC); 

        taskbar_put(&output);
        head->put_all((struct Swindow*)head, output);

        draw_output(output);
        draw_id(head);

        user_input();
        if (key_down('W')){
            tail->next = window_create("test", 20, 10, NULL);
            tail = tail->next;
        }

        update(head, &tail);
        tail->animate(tail);
        Sleep(30);
    }

    head->destroy_all((struct Swindow*)head);
    fclose(freestr);
    fclose(fconfig);
    fclose(fwall);
    return 0;
}