#ifndef _MAIN_H_
#define _MAIN_H_
/*----------------------    COMUNES    ----------------------*/
#include <Arduino.h>
#include <WiFi.h>
//#include <WiFiClientSecure.h> 
#include <Wire.h>
#define WIFI_SSID "cleanlight1234"
#define WIFI_PASS "12345678"

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

/*----------------------    LAN8720A    ----------------------*/
//#define ETH_PHY_ADDR    1   // Dirección del PHY (generalmente 1)
#define ETH_POWER_PIN   -1  // Sin pin para controlar la alimentación (si es necesario)
#define ETH_MDIO_GPIO   18  // Pin para MDIO
#define ETH_MDC_GPIO    23  // Pin para MDC
#define ETH_RST_GPIO    -1  // Sin pin de reset, si el módulo no lo necesita
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT  // Usar GPIO0 para generar el reloj de 50 MHz

/* ----------------------   RS485    ---------------------- */
#include <ModbusRTU.h>

#define MODBUS_TX_PIN     32
#define MODBUS_RX_PIN     35
#define RS485_BAUD_RATE   9600
#define REGN              10
#define SLAVE_ID          1
#define MODBUS_SERIAL     Serial2
#define RE_DE_PIN 21

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

/*----------------------    RELAYS    ----------------------*/
#define RELAY_01        15
#define RELAY_02        2

extern uint32_t updateFlags;
extern bool sendExpress;

/*----------------------    GPS    ----------------------*/
#define GPS_TX_SERIAL_RX 4
#define GPS_RX_SERIAL_TX 16
#define SERIAL_BAUD_GPS 9600
#define GPS_SERIAL Serial1

/*----------------------  THINGSBOARD  ----------------------*/
#include <ThingsBoard.h>
#define THINGSBOARD_SERVER      "iot.cleanlight.cl"
#define TOKEN                   "lk5XeW1V52ao5rsQzTHb" // dispositivo motor-001

#endif