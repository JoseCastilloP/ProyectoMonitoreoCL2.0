#include "pzem017.h"
#include "adl200n-ct.h"

extern HardwareSerial Serial485;
extern ModbusRTU mb;

uint16_t voltageDc;
uint16_t currentDc;
uint32_t powerDc;
uint32_t energyDc;
float Vbat1;
float Cbat1;
double Pbat1;
double Ebat1;

uint16_t voltageDc2;
uint16_t currentDc2;
uint32_t powerDc2;
uint32_t energyDc2;
float Vpanel1;
float Cpanel1;
double Ppanel1;
double Epanel1;

void pzem017Loop(void)
{
    static uint8_t turn = 0;

    if (!mb.slave())
    {
        switch (turn)
        {
        case 0:
        {
            // mb.readIreg(PZEM017_ID1, 0x0000, &rawVoltage, 1);
            mb.readHreg(PZEM017_ID1, 0x0000, &voltageDc, 1, cbRead);
            Vbat1 = voltageDc * 0.01;
            turn++;
        }
        break;
        case 1:
        {
            // mb.readIreg(PZEM017_ID1, 0x0001, &rawCurrent, 1);
            mb.readHreg(PZEM017_ID1, 0x0001, &currentDc, 1, cbRead);
            Cbat1 = currentDc * 0.01;
            turn++;
        }
        break;
        case 2:
        {
            // Rated primary voltage → uint32 (dos registros)
            uint16_t powerDcRegs[2];
            // mb.readIreg(PZEM017_ID1, 0x0002, powerDcRegs, 2, cbRead);
            mb.readHreg(PZEM017_ID1, 0x0002, powerDcRegs, 2, cbRead);
            powerDc = ((uint32_t)powerDcRegs[0] << 16) | powerDcRegs[1];
            Pbat1 = powerDc * 0.1;
            turn++;
        }
        break;
        case 3:
        {
            uint16_t energyDcRegs[2];
            // mb.readIreg(PZEM017_ID1, 0x0002, energyDcRegs, 2, cbRead);
            mb.readHreg(PZEM017_ID1, 0x0002, energyDcRegs, 2, cbRead);
            energyDc = ((uint32_t)energyDcRegs[0] << 16) | energyDcRegs[1];
            Ebat1 = energyDc;
            turn++;
        }
        break;
        case 4:
        {
            // mb.readIreg(PZEM017_ID2, 0x0000, &rawVoltage, 1);
            mb.readHreg(PZEM017_ID2, 0x0000, &voltageDc2, 1, cbRead);
            Vpanel1 = voltageDc2 * 0.01;
            turn++;
        }
        break;
        case 5:
        {
            // mb.readIreg(PZEM017_ID2, 0x0001, &rawCurrent, 1);
            mb.readHreg(PZEM017_ID2, 0x0001, &currentDc2, 1, cbRead);
            Cpanel1 = currentDc2 * 0.01;
            turn++;
        }
        break;
        case 6:
        {
            uint16_t powerDcRegs2[2];
            // mb.readIreg(PZEM017_ID2, 0x0002, powerDcRegs, 2, cbRead);
            mb.readHreg(PZEM017_ID2, 0x0002, powerDcRegs2, 2, cbRead);
            powerDc2 = ((uint32_t)powerDcRegs2[0] << 16) | powerDcRegs2[1];
            Ppanel1 = powerDc2 * 0.1;
            turn++;
        }
        break;
        case 7:
        {
            uint16_t energyDcRegs2[2];
            // mb.readIreg(PZEM017_ID2, 0x0002, energyDcRegs, 2, cbRead);
            mb.readHreg(PZEM017_ID2, 0x0002, energyDcRegs2, 2, cbRead);
            energyDc2 = ((uint32_t)energyDcRegs2[0] << 16) | energyDcRegs2[1];
            Epanel1 = energyDc2;
            turn++;
        }
        break;
        
        default:
        turn = 0;
            break;
        }

    }

}