/*
  Arduino 1.8.12

  Type de carte : Arduino nano (old bootloader)
  
  Modification de la librairie "SHT1x" nécessaire (à tester) :
  - Dans le fichier "SHT1x.cpp" fonction "void SHT1x::waitForResultSHT(int _dataPin)" :
	Passer le delay(10) en delay(1)
	Voir également pour basser tous les délais

  La modification ci-dessous n'est plus vraie avec la lib 1.0.7 d'ArduinoModbus
  
  Modification de la librairie "ArduinoModbus" nécessaire :
  - Dans le fichier "ModbusRTUServer.cpp" fonction "void ModbusRTUServerClass::poll()" :
      int ModbusRTUServerClass::poll()
      {
        uint8_t request[MODBUS_RTU_MAX_ADU_LENGTH];
        int rc;
      
        rc = -1;
        
        int requestLength = modbus_receive(_mb, request);
      
        if (requestLength > 0)
        {
          rc = modbus_reply(_mb, request, requestLength, &_mbMapping);
        }
      
        return rc;
      }

  - Dans le fichier "ModbusRTUServer.h" :
      remplacer   virtual void poll(); par   virtual int poll();

  - Dans le fichier "ModbusServer.h" :
      remplacer   virtual void poll() = 0; par   virtual int poll() = 0;;

  - Dans le fichier "ModbusTCPServer.h" :
      remplacer   virtual void poll(); par   virtual int poll();

  - Dans le fichier "ModbusTCPServer.cpp" fonction "void ModbusRTUServerClass::poll()" :
      int ModbusTCPServer::poll()
      {
        int rc;
      
        rc = -1;  
        
        if (_client != NULL) {
          uint8_t request[MODBUS_TCP_MAX_ADU_LENGTH];
      
          int requestLength = modbus_receive(_mb, request);
      
          if (requestLength > 0)
        {
            modbus_reply(_mb, request, requestLength, &_mbMapping);
          }
        }
      
        return rc;  
      }
            
*/
#include <ArduinoModbus.h>
#include <SHT1x.h>

//---------------------------------------------------------------
// CONFIGURATION
//---------------------------------------------------------------
#define MB_SLAVE_ID 100

#define dataPin 4
#define clockPin 3

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
  temp_c = sht1x.readTemperatureC();
  humidity = sht1x.readHumidity();

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
