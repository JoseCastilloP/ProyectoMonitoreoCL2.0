#include "adl200n-ct.h"

extern HardwareSerial Serial485;
extern ModbusRTU mb;

// Variables para almacenar resultados
float voltageA;
float frequency;
float powerA;
float currentA;
float PFA;
double activeEnergyA;

uint16_t auxV[2], auxF[2], auxP[2], auxC[2], auxPF[2], auxE[4];

float modbusToFloat(uint16_t regLow, uint16_t regHigh)
{
    uint32_t raw = ((uint32_t)regHigh << 16) | regLow;
    return *((float *)&raw);
}

void modbusProcess(void)
{
    mb.task();
    yield();
}

bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
    if (event != Modbus::EX_SUCCESS)
    {
        DEBUG_NL("Fallo lectura Modbus. Codigo: %d", event);
    }
    return true;
}

void preTransmission()
{
    digitalWrite(RE_DE_PIN, HIGH);
}
void postTransmission()
{
    digitalWrite(RE_DE_PIN, LOW);
}

void adl200nCtLoop(void)
{
    static uint8_t turn = 0;
    if (!mb.slave())
    {
        switch (turn)
        {
        case 0:
        {
            mb.readHreg(1, 0x2000, auxV, 2, cbRead);
            voltageA = modbusToFloat(auxV[1], auxV[0]);
            turn++;
        }
        break;
        case 1:
        {
            mb.readHreg(1, 0x200C, auxC, 2, cbRead);
            currentA = modbusToFloat(auxC[1], auxC[0]);
            turn++;
        }
        break;
        case 2:
        {
            mb.readHreg(1, 0x2014, auxP, 2, cbRead);
            powerA = modbusToFloat(auxP[1], auxP[0]);
            turn++;
        }
        break;
        case 3:
        {
            mb.readHreg(1, 0x2034, auxF, 2, cbRead);
            frequency = modbusToFloat(auxF[1], auxF[0]);
            turn++;
        }
        break;
        case 4:
        {
            mb.readHreg(1, 0x202C, auxPF, 2, cbRead);
            PFA = modbusToFloat(auxPF[1], auxPF[0]);
            turn++;
        }
        break;
        case 5:
        {
            mb.readHreg(1, 0x3000, auxE, 4, cbRead);
            uint64_t raw = ((uint64_t)auxE[0] << 48) |
                           ((uint64_t)auxE[1] << 32) |
                           ((uint64_t)auxE[2] << 16) |
                           ((uint64_t)auxE[3]);

            memcpy(&activeEnergyA, &raw, sizeof(activeEnergyA));
            turn++;
        }
        break;

        default:
        {
            turn = 0;
        }
        break;
        }
    }

    static uint32_t last = 0;
    if (millis() - last > 30000)
    {
        last = millis();
        DEBUG_NL("------ Lectura Modbus ------");
        DEBUG_NL("Voltaje AC: %.2f", voltageA);
        DEBUG_NL("Corriente AC: %.2f", currentA);
        DEBUG_NL("Frecuencia AC: %.2f", frequency);
        DEBUG_NL("Fact Potencia: %.2f", PFA);
        DEBUG_NL("Potencia AC: %.2f", powerA);
        DEBUG_NL("Energia AC: %.2f", activeEnergyA);
    }
}