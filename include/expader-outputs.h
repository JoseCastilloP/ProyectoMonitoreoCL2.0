#ifndef _EXPANDER_OUTPUTS_H
#define _EXPANDER_OUTPUTS_H
    #include "main.h"
    void relays_init(uint8_t slaveAddr, int sda, int scl);
    void relay_set(uint8_t relay, uint8_t value);
#endif