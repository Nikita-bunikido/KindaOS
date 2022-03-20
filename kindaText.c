#include "window.h"

struct key_comb {
    int key;
    bool shift;
};
extern struct key_comb get_key_comb (const int);
extern bool key_down (int);

static int cursor_posx = 1;
static int cursor_posy = 2;

void kinda_text (struct Swindow * self){
    self->data[cursor_posy][cursor_posx] = (char)219;

    for (int i = 32; i <= 126; i++){
        struct key_comb c = get_key_comb(i);
        if (key_down(c.key) && !keys[c.key]){
            if(c.shift && !key_down(KEY_SHIFT))
                continue;
            self->data[cursor_posy][cursor_posx++] = i;
            keys[c.key] = true;
        } else if (!key_down(c.key))
            keys[c.key] = false;
    }

    if (key_down(KEY_BACKSPACE) && !keys[0]){ //backspace
        self->data[cursor_posy][cursor_posx--] = ' ';
        /* finding upper text */
        if (!cursor_posx && cursor_posy>3){
            cursor_posx = 1;
            cursor_posy--;
            while (self->data[cursor_posy][cursor_posx++] != C_EMPTY)
                ;
            cursor_posx--;
        }

        keys[0] = true;
    } else if (!key_down(KEY_BACKSPACE))
        keys[0] = false;

    if (key_down(KEY_ENTER) && !keys[1]){ //enter
        self->data[cursor_posy][cursor_posx] = ' ';
        self->data[++cursor_posy][cursor_posx=1] = (char)219;
        keys[1] = true;
    } else if (!key_down(KEY_ENTER))
        keys[1] = false;

    if (cursor_posx < 1)
        cursor_posx = 1;
}