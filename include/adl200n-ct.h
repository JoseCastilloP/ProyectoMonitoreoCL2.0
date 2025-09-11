#ifndef _ADL200N_CT_H
#define _ADL200N_CT_H
    #include "main.h"

    void adl200nCtLoop(void);
    bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void *data);
    void preTransmission();
    void postTransmission();

    void modbusProcess(void);

#endif