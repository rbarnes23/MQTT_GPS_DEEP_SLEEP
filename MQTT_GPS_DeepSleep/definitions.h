#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TinyGPS.h>

HardwareSerial Serial1(1);

//Standard Defs not used here
#define DISPLAYAVAIL
#define DEBUG
#define FREQUENCYOFREADING .002e6 //The first number is number of seconds i.e.60

// Choose two free pins
#define GPS_TXPIN 17
#define GPS_RXPIN 16

//  Deep Sleep Settins
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

// RTC Memory - Not used here....yet
RTC_DATA_ATTR int bootCount = 0;


WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Update these with values suitable for your network.
const char* SSID     = "Frontier9328";
const char* PASSWORD = "6164187456";
const char* MQTT_SERVER = "mapmymotion.com";
TinyGPS gps;


static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (Serial1.available())
      gps.encode(Serial1.read());
  } while (millis() - start < ms);
}



static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}


static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("********** ******** ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
            month, day, year, hour, minute, second);
    Serial.println(sz);
  }
  Serial.println();
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
}


// Format the message and send via MQTT
void sendJS( ulong uTime, float lat, float lon ) {
  String msg;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  //Get The MAC id
  uint64_t chipid = ESP.getEfuseMac();
  char cMsg[16];
  snprintf (cMsg, 16, "%04X%8x", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  root["macid"] = cMsg;
  root["utime"] = uTime;
  root["lat"] = lat;
  root["lon"] = lon;

  root.printTo(msg);
  String strs = "TT/" + String(cMsg);
  //Serial.println(msg.length());
  mqttClient.publish(strs.c_str(), msg.c_str());
}


/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

//Calculate Unix Time from NMEA date info
unsigned long unixTimestamp(int year, int month, int day,
                            int hour, int min, int sec)
{
  const short days_since_beginning_of_year[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

  int leap_years = ((year - 1) - 1968) / 4
                   - ((year - 1) - 1900) / 100
                   + ((year - 1) - 1600) / 400;

  long days_since_1970 = (year - 1970) * 365 + leap_years
                         + days_since_beginning_of_year[month - 1] + day - 1;

  if ( (month > 2) && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) )
    days_since_1970 += 1; /* +leap day, if year is a leap year */

  return sec + 60 * ( min + 60 * (hour + 24 * days_since_1970) );
}




