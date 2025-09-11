#include "pzem017.h"
#include "adl200n-ct.h"

extern HardwareSerial Serial485;
extern ModbusRTU mb;

uint16_t voltageDc;
uint16_t currentDc;
uint32_t powerDc;
uint32_t energyDc;

void pzem017Loop(void)
{
    if (!mb.slave())
    {
        mb.readHreg(2, 0x0000, &voltageDc, 1, cbRead);
        mb.readHreg(2, 0x0001, &currentDc, 1, cbRead);


        // Rated primary voltage → uint32 (dos registros)
        uint16_t powerDcRegs[2];
        mb.readHreg(1, 0x0002, powerDcRegs, 2, cbRead);
        powerDc = ((uint32_t)powerDcRegs[0] << 16) | powerDcRegs[1];

        uint16_t energyDcRegs[2];
        mb.readHreg(1, 0x0002, energyDcRegs, 2, cbRead);
        energyDc = ((uint32_t)energyDcRegs[0] << 16) | energyDcRegs[1];
    }

    // Mostrar cada 3s
    static uint32_t last = 0;
    if (millis() - last > 3000)
    {
        last = millis();
        DEBUG_NL("------ Lectura Modbus ------");
        DEBUG_NL("VoltajeDc: %ld", voltageDc);
        DEBUG_NL("CorrienteDc: %ld", currentDc);
        DEBUG_NL("PotenciaDc: %ld", powerDc);
        DEBUG_NL("EnergiaDc: %ld", energyDc);

    }

}