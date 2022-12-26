/*
  Arduino 1.8.12

  Type de carte : Arduino nano (old bootloader)
  
  Modification de la librairie "SHT1x" nécessaire => prendre la version jointe.

  ArduinoModbus V1.0.7

*/
#include <ArduinoModbus.h>
#include <SHT1x.h>

//---------------------------------------------------------------
// CONFIGURATION
//---------------------------------------------------------------
#define MB_SLAVE_ID 100

#define dataPin 4
#define clockPin 3

//RE/DE RS485 sur D2

//#define SERIAL_DEBUG
//---------------------------------------------------------------

SHT1x sht1x(dataPin, clockPin);

int Cpt_Reception;

void setup()
{
  //---------------------------------------------------------------
  // Init Modbus
  //---------------------------------------------------------------
  delay(10);

  ModbusRTUServer.begin(MB_SLAVE_ID, 9600);

  int res = ModbusRTUServer.configureHoldingRegisters(0, 10);

  Cpt_Reception = 0;
  //---------------------------------------------------------------
}

void loop()
{
  //---------------------------------------------------------------
  // Modbus
  //---------------------------------------------------------------
  int rc;
  
  rc = ModbusRTUServer.poll();

  if (rc > 0)
  {
    ReadTemperature();
  }
  //---------------------------------------------------------------
}

void ReadTemperature()
{
  Cpt_Reception++;

  //---------------------------------------------------------------
  // Lecture température
  //---------------------------------------------------------------
  float temp_c;
  float humidity;
  int mb_res;
  
  // Read values from the sensor
  sht1x.readAll();
  temp_c = sht1x.getTemperatureC();
  humidity = sht1x.getHumidity();  

#if defined(SERIAL_DEBUG)
  Serial.print("Temperature raw : ");
  Serial.print(temp_c, DEC);
  Serial.print(" C / ");
  Serial.print("Humidity raw : ");
  Serial.print(humidity);
  Serial.println("%");
#endif

  int temp_c_TMP = (int) (temp_c * 10.0);
  temp_c = (float) temp_c_TMP / 10.0;

  int humidity_TMP = (int) (humidity * 1.0);
  humidity = (float) humidity_TMP / 1.0;

  mb_res = ModbusRTUServer.holdingRegisterWrite(0, Cpt_Reception);
  mb_res = ModbusRTUServer.holdingRegisterWrite(1, temp_c_TMP);
  mb_res = ModbusRTUServer.holdingRegisterWrite(2, humidity_TMP);
  //---------------------------------------------------------------
}
