/*
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

  Type de carte : Generic ESP8266 Module
*/
#include <ArduinoModbus.h>
#include <ESP8266WiFi.h>
#include <DHT.h>;

//---------------------------------------------------------------
// CONFIGURATION
//---------------------------------------------------------------   
#define SLEEPDURATION 240 // secondes
#define LONGSLEEPDURATION 1800 // secondes
#define NB_TRYWIFI 10 // nbr d'essai connexion WiFi, number of try to connect WiFi
//#define SERIAL_DEBUG

const char* WLAN_SSID = "x";
const char* WLAN_PASSWD = "x";
IPAddress MB_SERVER(192, 168, 0, 199);
const int MB_ADDR = 0;
const int MB_NB = 2;
IPAddress ip(192, 168, 0, 123);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 254);
//---------------------------------------------------------------   

WiFiClient espClient;
ModbusTCPClient modbusTCPClient(espClient);


String MACStr;

//Constants
#define DHTPIN 3     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

uint16_t tab_c[4];
uint16_t tab_h[4];

void Data_create(float *val, uint16_t data[])
{
    uint16_t *sends;
    sends = (uint16_t *) val;
    memcpy(data, sends, sizeof(float));
}


void setup() 
{
  pinMode(3, FUNCTION_3);   
  
  //---------------------------------------------------------------
  // Init WIFI et Modbus
  //---------------------------------------------------------------   
#if defined(SERIAL_DEBUG)
    Serial.begin(115200);
#endif
  
  delay(10);

  MACStr = WiFi.macAddress();
#if defined(SERIAL_DEBUG)
    Serial.println(MACStr);
#endif  

  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );

  WiFi.setOutputPower(20.5);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode( WIFI_STA );
  WiFi.begin( WLAN_SSID, WLAN_PASSWD );

  dht.begin();
  //---------------------------------------------------------------   


  //---------------------------------------------------------------
  // Lecture température
  //---------------------------------------------------------------
  float temp_c;
  float humidity;

  // Read values from the sensor
  temp_c = dht.readTemperature();
  humidity = dht.readHumidity();

#if defined(SERIAL_DEBUG)
  Serial.print("Temperature raw : ");
  Serial.print(temp_c, DEC);
  Serial.print(" C / ");
  Serial.print("Humidity raw : ");
  Serial.print(humidity);
  Serial.println("%");
#endif   

  int temp_c_TMP = (int) (temp_c * 10.0);
  Serial.println(temp_c_TMP, DEC);
  temp_c = (float) temp_c_TMP / 10.0;

  int humidity_TMP = (int) (humidity * 1.0);
  humidity = (float) humidity_TMP / 1.0;  

  Data_create(&temp_c,tab_c);
  Data_create(&humidity,tab_h);

#if defined(SERIAL_DEBUG)
  Serial.print("Temperature: ");
  Serial.print(temp_c, DEC);
  Serial.print(" C / ");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
#endif 
  //---------------------------------------------------------------
  

  //---------------------------------------------------------------
  // Vérification de la connexion sinon deepsleep
  //---------------------------------------------------------------     
  int _try = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#if defined(SERIAL_DEBUG)
    Serial.print(".");
#endif   
   
    _try++;
    
    if ( _try >= NB_TRYWIFI )
    {
#if defined(SERIAL_DEBUG)      
      Serial.print("WIFI failed, Going to sleep");
#endif         
      //Impossible to connect WiFi network, go to deep sleep
      ESP.deepSleep(LONGSLEEPDURATION * 1000000);
    }    
  }

#if defined(SERIAL_DEBUG)
    Serial.println(WiFi.localIP());
#endif  
  //---------------------------------------------------------------  


  //---------------------------------------------------------------
  // Modbus
  //---------------------------------------------------------------
  if (!modbusTCPClient.connected())
  {
      // client not connected, start the Modbus TCP client
#if defined(SERIAL_DEBUG)      
      Serial.println("Attempting to connect to Modbus TCP server");
#endif

      int res = modbusTCPClient.begin(MB_SERVER);

#if defined(SERIAL_DEBUG)       
      if (!res)
      {
            Serial.println("Modbus TCP Client failed to connect!");
      }
      else
      {
            Serial.println("Modbus TCP Client connected");
      }
#endif      
  }
  
  if (modbusTCPClient.connected())
  {
      // client connected
      uint16_t LF = modbusTCPClient.holdingRegisterRead(14288);
 
      modbusTCPClient.beginTransmission(HOLDING_REGISTERS, 14318, 6);
      modbusTCPClient.write(LF);
      modbusTCPClient.write(0);
      modbusTCPClient.write(tab_c[0]);
      modbusTCPClient.write(tab_c[1]);
      modbusTCPClient.write(tab_h[0]);
      modbusTCPClient.write(tab_h[1]);
      modbusTCPClient.endTransmission();
  }

  modbusTCPClient.stop();
  WiFi.disconnect(); 
  WiFi.mode( WIFI_OFF );
  //---------------------------------------------------------------



  //---------------------------------------------------------------
  // Sleep
  //---------------------------------------------------------------  
#if defined(SERIAL_DEBUG)
    Serial.print("Going to sleep");
#endif   
     
  ESP.deepSleep(SLEEPDURATION * 1000000);
  //---------------------------------------------------------------       
}

void loop()
{
}



/*
Add this to your code at the beginning of set void setup():

//********** CHANGE PIN FUNCTION  TO GPIO **********
//GPIO 1 (TX) swap the pin to a GPIO.
pinMode(1, FUNCTION_3); 
//GPIO 3 (RX) swap the pin to a GPIO.
pinMode(3, FUNCTION_3); 
//**************************************************
You will no longer be able to use the Serial Monitor as TX will now be a GPIO pin and not transmit Serial data. You can still Flash your device as when you boot the device in flash mode it converts GPIO1 and GPIO3 back to TX/RX. Once you reboot into regular running mode GPIO1 and GPIO3 will go back to being GPIO pins.

To change GPIO1 and GPIO3 back to being TX/RX for regular Serial Monitor use add this to your code at the beginning of set void setup():

//********** CHANGE PIN FUNCTION  TO TX/RX **********
//GPIO 1 (TX) swap the pin to a TX.
pinMode(1, FUNCTION_0); 
//GPIO 3 (RX) swap the pin to a RX.
pinMode(3, FUNCTION_0); 
//***************************************************

*/
