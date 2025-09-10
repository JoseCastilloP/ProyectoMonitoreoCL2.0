#include "main.h"
#include <wificonnect.h>
#include <temp-hum.h>

const uint16_t THINGSBOARD_PORT = 80U;
const uint16_t MAX_MESSAGE_SIZE = 128U;

WiFiClient espClient;
ThingsBoard tb(espClient);

PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

PCF8574 pcf8574(RELAY_ADDRESS, SDAPIN, SCLPIN);

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float humidity = 0;
float lastTemperature = 0;
float lastHumidity = 0;
float mms;
uint32_t stateVariables;

int analog_input1;
int analog_input2;
int analog_input3;
int analog_input4;

bool suscrito = false;
bool r1Status = false;
bool sendExpress = false;
bool initPzem = false;

WiFiState wifiState = DISCONNECTED;
unsigned long lastAttemptTime = 0;
const unsigned long attemptInterval = 1000; // 500ms
int connect_count = 0;

uint32_t cicle_t = 0;
uint32_t to_var = 0, to_log = 0, to_tb = 0, to_second;
const uint32_t TIME_LOOPS = 100;
uint32_t updateFlags = 0;
uint64_t conn = 0, dcon = 0;

extern float voltage ;
extern float current ;
extern float power ;
extern float energy ;
extern float frequency ;
extern float pf ;
extern float lastVoltage ;
extern float lastCurrent ;

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

void setup()
{
  SERIAL_MON.begin(SERIAL_BAUD);
  DEBUG_NL("[setup] Initializing device");

  DHT_init(&dht);
  DHTSensor(&dht);

  // wifi_connect();
   handleWiFiConnection();
  // Uncomment in order to reset the internal energy counter

  Wire.begin(4,15,0);
  // Set pinMode to OUTPUT

  lastVoltage = 0;
  lastCurrent = 0;
  initPzem = true;
  delay(300);
}

void loop()
{
  handleWiFiConnection();
  // fota_loop();
  if(to_var >= 50) //5000ms
  {
    DHTSensor(&dht);
    to_var = 0;
  }

  if(to_log >= 300)
  {
    printData();
    to_log = 0;
  }

  if((to_tb >= 3000) || sendExpress)
  {
    if(current > 0) updateFlags |= UPDATE_PZEM;
    /*if(mms > 0)*/ updateFlags |= UPDATE_MMS;
    updateFlags |= UPDATE_DTH;

    sendData();
    sendExpress = false;
    to_tb = 0;
  }

  if(to_second >= 10)
  {
    if(current > 0) conn++;
    else dcon++;
    to_second = 0;
  }

  systemTick();
  tb.loop();
}

void analogILoop(void)
{
  analog_input1 = analogRead(INA1);
  // analog_input2 = analogRead(INA2);
  // analog_input3 = analogRead(INA3);
  // analog_input4 = analogRead(INA4);
}

void mms_update(void)
{
  static float _mms  = 0;
  analogILoop();
  _mms = mapValue(analog_input1, VOLTAGE_MIN, VOLTAGE_MAX, MMS_MIN, MMS_MAX);
  if(abs(_mms - mms) > 0.7) 
  {
    updateFlags |= UPDATE_MMS;
    sendExpress = true;
  }
  mms = _mms;
  if(mms > 4.5)
  {
    pcf8574.digitalWrite(RELAY_01, LOW);
  }else
  {
    pcf8574.digitalWrite(RELAY_01, HIGH);
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
  sprintf(aux, "[printData] temp: %.2f hum: %.2f ina:%d mms: %.4f", temperature, humidity, analog_input1, mms);
  DEBUG_NL(aux);
  memset(aux, 0, sizeof(aux));
  sprintf(aux, "[PZEM] vac: %.2f cac: %.2f pow: %.2f ene: %.2f fre: %.2f pf: %.2f",voltage, current, power, energy, frequency, pf);
  DEBUG_NL(aux);
  
  DHTSensor(&dht);
  analogILoop();
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
  }
  
  if(updateFlags & UPDATE_PZEM)
  {
    tb.sendTelemetryData("v_ac", voltage);
    tb.sendTelemetryData("c_ac", current);
    tb.sendTelemetryData("pote", power);
    tb.sendTelemetryFloat("fpow", pf);
    tb.sendTelemetryData("frec", frequency);
    tb.sendTelemetryData("ener", energy);
    updateFlags &= ~UPDATE_PZEM;
  }

  if(updateFlags & UPDATE_MMS)
  {
    tb.sendTelemetryData("mms", mms);
    updateFlags &= ~UPDATE_MMS;
  }

  if(updateFlags & UPDATE_DTH)
  {
    tb.sendTelemetryData("temp", temperature);
    tb.sendTelemetryData("humi", humidity);
    updateFlags &= ~UPDATE_DTH;
  }

  if (current > 0)
  {
    tb.sendTelemetryData("conn", conn);
    conn = 0;
  }else
  {
    tb.sendTelemetryData("dcon", dcon);
    dcon = 0;
  }

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
  mms_update();
  pzem004T_loop();
}

void systemTick(void)
{
  if (millis() - cicle_t > TIME_LOOPS)
  {
    to_tb++;
    to_var++;
    to_log++;
    to_second++;
    cicle_t = millis();
  }
}