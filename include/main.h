#ifndef _MAIN_H_
#define _MAIN_H_
/*----------------------    COMUNES    ----------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#define WIFI_SSID "Thingscontrol"
#define WIFI_PASS "Th1ng5c0ntr0l"

/* ========================== DEFINITIONS ==========================*/
enum StateVariables
{
  DHT22_ONE_DETECTED = BIT0,
  GPS_DATA_VALID = BIT1,
};

enum WiFiState
{
  DISCONNECTED,
  CONNECTING,
  CONNECTED
};

enum UpdateFlags
{
  UPDATE_MMS    = BIT1,
  UPDATE_PZEM   = BIT2,
  UPDATE_DTH    = BIT3
};

/* ========================== DEBUG ==========================*/
#define ENABLE_DEBUG        // Indicates whether serial debugging is enabled or not by default
#define SERIAL_MON Serial   // Serial output for debug console
#define SERIAL_BAUD 115200  // Data rate in bits per second (baud)
#ifdef ENABLE_DEBUG
#define DEBUG_NN(...) printf(__VA_ARGS__)
#define DEBUG_NL(...) \
  printf(__VA_ARGS__); \
  printf("\n")
#else
#define DEBUG_NN(...)
#define DEBUG_NL(...)
#endif

/*----------------------    FOTA    ----------------------*/

// #include <otadrive_esp.h>
// OTAdrive APIkey for this product
// #define APIKEY "COPY_APIKEY_HERE"
// // this app version
// #define FW_VER "v@x.x.x"

/*----------------------    PZEM-004T    ----------------------*/
#include <PZEM004Tv30.h>
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 13
#define PZEM_TX_PIN 12
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

/*----------------------    DTH22    ----------------------*/
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 33 
#define DHTTYPE DHT22

/*----------------------    ANALOG INPUTS    ----------------------*/
#define INA1    36
#define INA2    39
#define INA3    34
#define INA4    35

/*----------------------    VIBRACION    ----------------------*/
#define MMS_MIN         0
#define MMS_MAX         11
#define VOLTAGE_MIN     660
#define VOLTAGE_MAX     4095

/*----------------------    RELAYS    ----------------------*/
#include "PCF8574.h"
#define RELAY_ADDRESS   0x24
#define SDAPIN          4
#define SCLPIN          15
#define RELAY_01        P0
#define RELAY_02        P1
#define RELAY_03        P2
#define RELAY_04        P3
#define RELAY_05        P4
#define RELAY_06        P5

extern PZEM004Tv30 pzem;
extern PCF8574 pcf8574;
extern uint32_t updateFlags;
extern bool sendExpress;
/*----------------------  THINGSBOARD  ----------------------*/
#include <ThingsBoard.h>
#define THINGSBOARD_SERVER      "panel.thingscontrol.cloud"
#define TOKEN                   "328tLf5KGgckQdnhrlTc" // dispositivo motor-001

#endif