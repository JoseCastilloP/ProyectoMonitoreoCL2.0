#include "main.h"
#include <wificonnect.h>
#include <temp-hum.h>
#include <pzem017.h>
#include <adl200n-ct.h>
#include <gps-neo6m.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_MQTT_Client.h>
#include <OTA_Firmware_Update.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <Espressif_Updater.h>

constexpr char CURRENT_FIRMWARE_TITLE[] = "NODE_CL";
constexpr char CURRENT_FIRMWARE_VERSION[] = "00.00.01";

// constexpr char TOKEN[] = "y8axwLXM0BPN7saIuLHH";
// constexpr char THINGSBOARD_SERVER[] = "iot.cleanlight.cl";
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 512U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 512U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
char payload_str[255];

constexpr const char RPC_SWITCH_KEY[] = "switch";
constexpr const char RPC_RELAY01_METHOD[] = "setRelay1";
constexpr const char RPC_RELAY02_METHOD[] = "setRelay2";
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 2U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
// Initialize used apis
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
OTA_Firmware_Update<> ota;

void procesSetRelay1(const JsonVariantConst &data, JsonDocument &response);
void procesSetRelay2(const JsonVariantConst &data, JsonDocument &response);

const std::array<IAPI_Implementation *, 2U> apis = {
    &rpc, &ota};

const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
      RPC_Callback{ "setRelay1", procesSetRelay1 },
      RPC_Callback{ "setRelay2", procesSetRelay2 }
    };
// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);
Espressif_Updater<> updater;

bool currentFWSent = false;
bool updateRequestSent = false;
bool subscribed = false;

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
uint32_t to_var = 0, to_log = 0, to_tb = 0, to_second, to_modbus = 0, to_adl200n = 0, to_gps = 0, to_pzem017 = 0;
const uint32_t TIME_LOOPS = 100;
uint32_t updateFlags = 0;
uint64_t conn = 0, dcon = 0;

extern uint16_t voltageDc;
extern uint16_t currentDc;
extern uint32_t powerDc;
extern uint32_t energyDc;
extern float Vbat1;
extern float Cbat1;
extern double Pbat1;
extern double Ebat1;

extern uint16_t voltageDc2;
extern uint16_t currentDc2;
extern uint32_t powerDc2;
extern uint32_t energyDc2;
extern float Vpanel1;
extern float Cpanel1;
extern double Ppanel1;
extern double Epanel1;

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
bool relay1 = false, relay2 = false;

void update_starting_callback()
{
  Serial.println("update_starting_callback()");
}

void finished_callback(const bool &success)
{
  if (success)
  {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t &current, const size_t &total)
{
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}

void procesSetRelay1(const JsonVariantConst &data, JsonDocument &response)
{

  relay1 = data;
  digitalWrite(RELAY_01, relay1? LOW : HIGH);
  DEBUG_NN("Relay1 -> ");
  DEBUG_NL(relay1 ? "ON" : "OFF");
  memset(payload_str, 0, sizeof(payload_str));
  sprintf(payload_str,  "{\"relay1\":%d,\"relay2\":%d}", relay1, relay2);
  tb.sendTelemetryString(payload_str);
  response.set(relay1);
}

void procesSetRelay2(const JsonVariantConst &data, JsonDocument &response)
{
  relay2 = data["setRelay2"];
  digitalWrite(RELAY_02, relay2? LOW : HIGH);
  DEBUG_NN("Relay2 -> ");
  DEBUG_NL(relay2 ? "ON" : "OFF");
  memset(payload_str, 0, sizeof(payload_str));
  sprintf(payload_str,  "{\"relay1\":%d,\"relay2\":%d}", relay1, relay2);
  tb.sendTelemetryString(payload_str);
  response.set(relay2);
}

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

  if (to_pzem017 >= 10)
  {
    pzem017Loop();
    to_pzem017 = 0;
  }

  if (to_gps >= 5)
  {
    gpsdata();
    to_gps = 0;
  }

  handleWiFiConnection();
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
  sprintf(aux, "[ADL200N] vac: %.2f cac: %.2f pow: %.2f ene: %.2f fre: %.2f pf: %.2f", voltageA, currentA, powerA, activeEnergyA, frequency, PFA);
  DEBUG_NL(aux);

  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[GPS-NEO-6M] Vbat1: %d Cbat1: %d", longitud, latitud);

  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[PZEM-017 - 1] VBat1: %.2f CBat1: %f PBat1: %.2f EBat1: %.2f", Vbat1, Cbat1, Pbat1, Ebat1);
  DEBUG_NL(aux);

  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[PZEM-017 - 2] VPanel1: %.2f CPanel1: %.2f PPanel1: %.2f EPanel1: %.2f", Vpanel1, Cpanel1, Ppanel1, Epanel1);
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
      if (!currentFWSent)
      {
        Serial.println("ota.Firmware_Send_Info");
        currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
      }

      if (!updateRequestSent)
      {
        Serial.println("Firwmare Update Subscription...");
        const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = ota.Subscribe_Firmware_Update(callback);
      }

      if (!subscribed) {
    Serial.println("Subscribing for RPC...");
    if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }

    Serial.println("Subscribe done");
    subscribed = true;
  }

    }
  }

  memset(payload_str, 0, sizeof(payload_str));
  sprintf(payload_str, "{\"VBat1\":%.2,\"VPanel1\":%.2f,\"IBat1\":%.2f,\"IPanel1\":%.2f,\"PBat1\":%.2f,\"PPanel1\":%.2f,\"EBat1\":%.2f,\"EPanel1\":%.2f}",
          Vbat1,
          Vpanel1,
          Cbat1,
          Cpanel1,
          Pbat1,
          Ppanel1,
          Ebat1,
          Epanel1);
  tb.sendTelemetryString(payload_str);

  memset(payload_str, 0, sizeof(payload_str));
  sprintf(payload_str, "{\"Energy\":%.2,\"Pf\":%.3f,\"Frec\":%.2f,\"POWER\":%d,\"VAC\":%d,\"latitude\":%f,\"longitude\":%f,\"temp\":%f,\"humi\":%f}",
          activeEnergyA,
          PFA,
          frequency,
          powerA,
          voltageA,
          latitud,
          longitud,
          temperature,
          humidity);
  tb.sendTelemetryString(payload_str);

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
    to_gps++;
    to_pzem017++;
    cicle_t = millis();
  }
}