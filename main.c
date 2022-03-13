#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "header.h"
#include "window.h"

struct cursor {
    uint32_t x, y;
} cursor = {0,0};

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

void task_bar_stack_push(struct Swindow* win){
    task_bar.stack[task_bar.stack_pos++] = win;
}

bool task_bar_not_in_stack (struct Swindow* win){
    for (uint32_t px = 0; px < task_bar.stack_pos; px++)
        if (task_bar.stack[px] == win)
            return false;
    return true;
}

struct Swindow* task_bar_stack_pop(void){
    return task_bar.stack[--task_bar.stack_pos];
}

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
    if (task_bar_not_in_stack(tmp) && key_down(' ') && cursor.x == tmp->sx+tmp->px-6
        && cursor.y == tmp->py+1){
            task_bar_stack_push(tmp);
        }

    /* open full screen selected window '#' button */
    if (key_down(' ') && cursor.x == tmp->sx+tmp->px-4
        && cursor.y == tmp->py+1){
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
        tmp->px = cursor.x - tmp->ox;
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
    if (tmp->program)
        tmp->program(tmp);
}

void draw_task_bar (Tarr arr){
    /* line */
    memset(arr[RES_H-task_bar.height], UI_HORZIONTAL, sizeof(arr[0]));
    for (uint32_t py = RES_H-task_bar.height+1; py < RES_H; py++)
        memset(arr[py], task_bar.cdraw[task_bar.state], sizeof(arr[py]));

    string_print_arr(arr, " | ", 0, RES_H-task_bar.height+1);
    uint32_t dpx = 3;
    for (uint32_t sp = 0; sp < task_bar.stack_pos; ++sp){
        string_print_arr(arr, task_bar.stack[sp]->title, dpx, RES_H-task_bar.height+1);
        dpx += strlen(task_bar.stack[sp]->title);
        string_print_arr(arr, " | ", dpx, RES_H-task_bar.height+1);
        dpx += 3;
    }
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


bool keys[255] = {0};

static struct key_comb {
    int key;
    bool shift;
} get_key_comb (const int c){

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

void text_editor (struct Swindow *self){
    static int cursor_posx = 1,
               cursor_posy = 3;

    self->data[cursor_posy][cursor_posx] = (char)219;

    for (int i = 32; i <= 126; i++){
        struct key_comb c = get_key_comb(i);
        if (key_down(c.key) && !keys[c.key]){
            if(c.shift && !key_down(16)) continue;
            self->data[cursor_posy][cursor_posx++] = i;
            keys[c.key] = true;
        } else if (!key_down(c.key))
            keys[c.key] = false;
    }

    if (key_down(8) && !keys[0]){ //backspace
        self->data[cursor_posy][cursor_posx--] = ' ';
        keys[0] = true;
    } else
        keys[0] = false;

    if (key_down(13) && !keys[1]){ //enter
        self->data[cursor_posy][cursor_posx] = ' ';
        self->data[++cursor_posy][cursor_posx=1] = (char)219;
        keys[1] = true;
    } else
        keys[1] = false;

    if (cursor_posx < 1)
        cursor_posx = 1;
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
    
    for (int u = 1; u < 3; u++){
        static char resultb[255], itoab[255];
        memset(itoab, 0, 255);
        sprintf(resultb, "window ");
        itoa(u, itoab, 10);
        strcat(resultb, itoab);
        tmp->next = (Twindow*)window_create(resultb, 20, 10, NULL);
        tmp = tmp->next;
    }


    tmp->next = (Twindow*)window_create("kinda text", 20, 10, text_editor);
    tmp = tmp->next;
    tail = tmp;

    play_intro("sys\\intro.txt");
    Sleep(100);

    /* ================================= */
    while (!key_down('Q')){
        /* setting output with empty characters */
        memset(output, C_EMPTY, sizeof(output));
        /* coping wallpaper to output */
        cpy_arr(output, wall, C_NAC); 

        draw_task_bar(output);
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