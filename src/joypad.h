#ifndef __JOYPAD_H__
#define __JOYPAD_H__
#include <stdint.h>

//TODO: Implement a proper driver to connect to an external joypad
typedef enum {
    JOYPAD_BUTTON_A,
    JOYPAD_BUTTON_B,
    JOYPAD_BUTTON_SELECT,
    JOYPAD_BUTTON_START,
    JOYPAD_BUTTON_UP,
    JOYPAD_BUTTON_DOWN,
    JOYPAD_BUTTON_LEFT,
    JOYPAD_BUTTON_RIGHT,
} joypad_button;

typedef struct {
    uint8_t strobe;
    uint8_t button_index;
    uint8_t button_status;
} joypad;

//Writes a value to the joypad strobe
void joypad_write(joypad *j, uint8_t val);
//Reads the currently pressed button
uint8_t joypad_read(joypad *j);
//Sets the currently pressed button
void joypad_set_button_pressed(joypad *j, joypad_button jb, uint8_t cond);

#endif //__JOYPAD_H__
