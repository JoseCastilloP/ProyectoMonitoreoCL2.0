#include <stdint.h>
#include "temp-hum.h"

extern float temperature;
extern float humidity;
extern float lastTemperature;
extern float lastHumidity;
extern uint32_t stateVariables;

void DHT_init(DHT *dht)
{
  dht->begin();
}

void DHTSensor(DHT *dht)
{
  temperature = dht->readTemperature();
  humidity = dht->readHumidity();
  if (isnan(humidity) || isnan(temperature))
  {
    stateVariables &= ~DHT22_ONE_DETECTED;
    DEBUG_NL("[SensorDHT] Error de sensor DHT11");
    return;
  }
  stateVariables |= DHT22_ONE_DETECTED;
  if((abs(temperature - lastTemperature) > 3) || (abs(humidity - lastHumidity) > 3)) updateFlags |= UPDATE_DTH;

  lastHumidity = humidity;
  lastTemperature =  temperature;
}