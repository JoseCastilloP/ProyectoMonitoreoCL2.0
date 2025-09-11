#include "adl200n-ct.h"

extern HardwareSerial Serial485;
extern ModbusRTU mb;

// Variables para almacenar resultados
uint16_t voltageA;
uint16_t frequency;
uint16_t powerA;
uint16_t currentA;
uint16_t PFA;
uint32_t activeEnergyA;

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
    if (!mb.slave())
    {
        mb.readHreg(1, 0x2000, &voltageA, 1, cbRead);
        mb.readHreg(1, 0x200C, &currentA, 1, cbRead);
        mb.readHreg(1, 0x2014, &powerA, 1, cbRead);
        mb.readHreg(1, 0x2034, &frequency, 1, cbRead);
        mb.readHreg(1, 0x202C, &PFA, 1, cbRead);

        // Rated primary voltage → uint32 (dos registros)
        uint16_t aPowerARegs[2];
        mb.readHreg(1, 0x1015, aPowerARegs, 2, cbRead);
        activeEnergyA = ((uint32_t)aPowerARegs[0] << 16) | aPowerARegs[1];

    }

    mb.task(); // Procesa la máquina de estados Modbus
    yield();

    // Mostrar cada 3s
    static uint32_t last = 0;
    if (millis() - last > 3000)
    {
        last = millis();
        DEBUG_NL("------ Lectura Modbus ------");
        DEBUG_NL("Voltaje: %ld", voltageA);
        DEBUG_NL("Corriente: %ld", currentA);
        DEBUG_NL("Frecuencia: %ld", frequency);
        DEBUG_NL("Potencia: %ld", powerA);
        DEBUG_NL("Energia: %ld", activeEnergyA);

    }
}