#include "main.h"
#include <wificonnect.h>
#include <temp-hum.h>
#include <pzem017.h>
#include <adl200n-ct.h>
#include <gps-neo6m.h>

const uint16_t THINGSBOARD_PORT = 80U;
const uint16_t MAX_MESSAGE_SIZE = 128U;
char payload_str[255];

WiFiClient espClient;
ThingsBoard tb(espClient);

HardwareSerial Serial485(2);
ModbusRTU mb;

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float humidity = 0;
float lastTemperature = 0;
float lastHumidity = 0;
float mms;
uint32_t stateVariables;

bool suscrito = false;
bool r1Status = false;
bool sendExpress = false;
bool initPzem = false;

WiFiState wifiState = DISCONNECTED;
unsigned long lastAttemptTime = 0;
const unsigned long attemptInterval = 1000; // 500ms
int connect_count = 0;

uint32_t cicle_t = 0;
uint32_t to_var = 0, to_log = 0, to_tb = 0, to_second, to_modbus = 0, to_adl200n = 0, to_gps = 0;
const uint32_t TIME_LOOPS = 100;
uint32_t updateFlags = 0;
uint64_t conn = 0, dcon = 0;

extern uint16_t voltageDc;
extern uint16_t currentDc;
extern uint32_t powerDc;
extern uint32_t energyDc;
extern uint16_t voltageDc2;
extern uint16_t currentDc2;
extern uint32_t powerDc2;
extern uint32_t energyDc2;

extern float voltageA;
extern float frequency;
extern float powerA;
extern float currentA;
extern float PFA;
extern double activeEnergyA;

extern float latitud;
extern float longitud;

void printData(void);
void analogILoop(void);
float mapValue(float input, float in_min, float in_max, float out_min, float out_max);
String generateString(int length);
bool reconnect(void);
void sendData(void);
void handleWiFiConnection(void);
void systemTick(void);
void mms_update(void);
void update_all_data(void);

RPC_Response setRelay1(const RPC_Data &data)
{
  bool state = data;  // true o false
  digitalWrite(RELAY_01, state ? LOW : HIGH); // activo en LOW
  Serial.print("Relay1 -> ");
  Serial.println(state ? "ON" : "OFF");
  tb.sendAttributeBool("setRelay1", state);
  return RPC_Response("relay1", state);
}

RPC_Response setRelay2(const RPC_Data &data)
{
  bool state = data;
  digitalWrite(RELAY_02, state ? LOW : HIGH);
  Serial.print("Relay2 -> ");
  Serial.println(state ? "ON" : "OFF");
  tb.sendAttributeBool("setRelay2", state);
  return RPC_Response("relay2", state);
}

// Lista de métodos RPC que acepta el ESP32
RPC_Callback callbacks[] = {
  { "setRelay1", setRelay1 },
  { "setRelay2", setRelay2 }
};


void setup()
{
  GPS_SERIAL.begin(SERIAL_BAUD_GPS, SERIAL_8N1, GPS_TX_SERIAL_RX, GPS_RX_SERIAL_TX, false, 20000, 112);
  SERIAL_MON.begin(SERIAL_BAUD);
  DEBUG_NL("[setup] Initializing device");

  DHT_init(&dht);
  DHTSensor(&dht);

  Serial485.begin(RS485_BAUD_RATE, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  mb.begin(&Serial485);
  mb.master();

  pinMode(RELAY_01, OUTPUT);
  pinMode(RELAY_02, OUTPUT);
  digitalWrite(RELAY_01, LOW);
  digitalWrite(RELAY_02, LOW);

  handleWiFiConnection();
  // Uncomment in order to reset the internal energy counter

  Wire.begin(4, 15, 0);
  // Set pinMode to OUTPUT

  initPzem = true;
  delay(300);
}

void loop()
{
  if (to_modbus >= 2) // 200ms
  {
    modbusProcess();
    to_modbus = 0;
  }

  if (to_adl200n >= 10)
  {
    adl200nCtLoop();
    to_adl200n = 0;
  }

  if(to_gps >= 5)
  {
    gpsdata();
    to_gps = 0;
  }


  handleWiFiConnection();
  // fota_loop();
  if (to_var >= 50) // 5000ms
  {
    DHTSensor(&dht);
    to_var = 0;
  }

  if (to_log >= 300)
  {
    printData();
    to_log = 0;
  }

  if ((to_tb >= 3000) || sendExpress)
  {
    updateFlags |= UPDATE_DTH;

    sendData();
    sendExpress = false;
    to_tb = 0;
  }

  if (to_second >= 15)
  {
    to_second = 0;
  }

  systemTick();
  tb.loop();
}

void analogILoop(void)
{
}

void mms_update(void)
{
  static float _mms = 0;
  analogILoop();
  if (abs(_mms - mms) > 0.7)
  {
    updateFlags |= UPDATE_MMS;
    sendExpress = true;
  }
}

float mapValue(float input, float in_min, float in_max, float out_min, float out_max)
{
  return (input - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void printData(void)
{
  char aux[255];
  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[GHT-22] temp: %.2f hum: %.2f", temperature, humidity);
  DEBUG_NL(aux);
  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[ADL200N] vac: %.2f cac: %.2f pow: %.2f ene: %.2f fre: %.2f pf: %.2f",voltageA, currentA, powerA, activeEnergyA, frequency, PFA);
  DEBUG_NL(aux);

  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[GPS-NEO6M] Latitud: %f Longitud: %f",longitud, latitud);
  DEBUG_NL(aux);

  DHTSensor(&dht);
}

String generateString(int length)
{
  const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const int charsetSize = strlen(charset);

  String randomString = "";

  for (int i = 0; i < length; i++)
  {
    char randomChar = charset[random(charsetSize)];
    randomString += randomChar;
  }

  return randomString;
}

void sendData(void)
{
  DEBUG_NN("[sendData] ");
  if (wifiState != CONNECTED)
  {
    DEBUG_NL("exiting ... no wifi");
    return;
  }

  if (!tb.connected())
  {
    // Connect to the ThingsBoard
    DEBUG_NN("Connecting to: ");
    DEBUG_NN(THINGSBOARD_SERVER);
    DEBUG_NN(" with token ");
    DEBUG_NL(TOKEN);
    String randomId = generateString(6);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, 1883, randomId.c_str()))
    {
      DEBUG_NL("Failed to connect");
      return;
    }
    else
    {
      DEBUG_NL("connect!!!");
      tb.RPC_Subscribe(callbacks, sizeof(callbacks) / sizeof(callbacks[0]));
    }
  }

  memset(payload_str, 0, sizeof(payload_str));
  sprintf(payload_str, "{\"VBat1\":%d,\"VPanel1\":%d,\"IBat1\":%d,\"IPanel1\":90,\"PBat1\":%d,\"PPanel1\":%d,\"EBat1\":%d,\"EPanel1\":%d}",
          voltageDc,
          voltageDc2,
          currentDc,
          currentDc2,
          powerDc,
          powerDc2,
          energyDc,
          energyDc2);
  tb.sendTelemetryJson(payload_str);

  memset(payload_str, 0, sizeof(payload_str));
  sprintf(payload_str, "{\"Energy\":%d,\"Fp\":%d,\"Frec\":%d,\"Humidity\":90,\"POWER\":%d,\"VAC\":%d}",
          activeEnergyA,
          PFA,
          frequency,
          powerA,
          voltageA);
  tb.sendTelemetryJson(payload_str);

  // if (current > 0)
  // {
  //   tb.sendTelemetryData("conn", conn);
  //   conn = 0;
  // }else
  // {
  //   tb.sendTelemetryData("dcon", dcon);
  //   dcon = 0;
  // }
}

void handleWiFiConnection(void)
{
  static uint8_t first_connect = 0;
  static unsigned long lastDisconnectTime = 0;

  switch (wifiState)
  {
  case DISCONNECTED:
    DEBUG_NL("[handleWiFiConnection] \n   Connecting to:  %s with: %s", WIFI_SSID, WIFI_PASS);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    wifiState = CONNECTING;
    lastAttemptTime = millis();
    break;

  case CONNECTING:
    if (WiFi.status() == WL_CONNECTED)
    {
      wifiState = CONNECTED;
      DEBUG_NL("   WiFi connected");
      DEBUG_NL("   IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      update_all_data();
      updateFlags = UPDATE_DTH + UPDATE_MMS + UPDATE_PZEM;
      sendData();
    }
    else if (millis() - lastAttemptTime >= attemptInterval)
    {
      lastAttemptTime = millis();
      connect_count++;
      if (connect_count > 180)
      {
        wifiState = DISCONNECTED; // Vuelve al estado DISCONNECTED o maneja el fallo como prefieras
        DEBUG_NN("");
        connect_count = 0;
      }
    }
    break;

  case CONNECTED:
    // Si necesitas hacer algo periódicamente cuando estás conectado, puedes hacerlo aquí.
    if (WiFi.status() != WL_CONNECTED && millis() - lastDisconnectTime > 30000)
    {
      DEBUG_NN("WiFi disconnected for more than 30 seconds. Reconnecting...");
      wifiState = DISCONNECTED;
      lastDisconnectTime = millis(); // Actualizar el tiempo de desconexión
    }

    if (!first_connect)
    {
      // dht.begin();
      first_connect = 1;
    }
    break;
  }
}

void update_all_data(void)
{
  DHTSensor(&dht);
}

void systemTick(void)
{
  if (millis() - cicle_t > TIME_LOOPS)
  {
    to_tb++;
    to_var++;
    to_log++;
    to_second++;
    to_modbus++;
    to_adl200n++;
    cicle_t = millis();
  }
}