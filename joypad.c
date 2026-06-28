#include "joypad.h"
#include <stdint.h>

//Writes a value to the joypad strobe
void joypad_write(joypad *j, uint8_t val) {
    j->strobe = (val & 1) == 1;
    if(j->strobe) j->button_index = 0;
}
//Reads the currently pressed button
uint8_t joypad_read(joypad *j) {
    if(j->button_index > 7) return 1;

    uint8_t response = (j->button_status >> j->button_index) & 1;
    if(!j->strobe && j->button_index <= 7)
        j->button_index++;

    return response;
}
//Sets the currently pressed button
void joypad_set_button_pressed(joypad *j, joypad_button jb, uint8_t cond) {
    if(cond) j->button_status |= (uint8_t)(1 << jb);
    else j->button_status &= (uint8_t)~(1 << jb);
}
