/* This sample code demonstrates the normal use of a TinyGPS object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 3(rx) and 4(tx).
*/

#include "definitions.h"
#include <HardwareSerial.h>
#include <SPI.h>


void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, GPS_RXPIN, GPS_TXPIN);
  setup_wifi();
  setupMqtt();

  Serial.print("Simple TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println("by Mikal Hart");
  Serial.println();
  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  getGpsReading();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");
  esp_deep_sleep_start();

}

void getGpsReading() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;


  // For one second we parse GPS data and report some key values
  // for (unsigned long start = millis(); millis() - start < 1000;)
  // {
  while (Serial1.available())
  {
    char c = Serial1.read();
    //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
    if (gps.encode(c)) // Did a new valid sentence come in?

      newData = true;
  }
  //}

  if (newData)
  {
    float flat, flon;
    unsigned long fix_age, time, date, speed, course;

    smartdelay(2000);

    gps.f_get_position(&flat, &flon, &fix_age);
    if (fix_age != TinyGPS::GPS_INVALID_AGE) {
      int year;
      byte month, day, hour, minute, second, hundredths;

      gps.crack_datetime(&year, &month, &day,
                         &hour, &minute, &second, &hundredths, &fix_age);

      unsigned long epoch = unixTimestamp(year, month, day, hour, minute, second);

      //      Serial.println(epoch);
      //      Serial.print(" SAT=");
      //      Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
      //      Serial.print(" PREC=");
      //      Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
      //Send the data
      sendJS(epoch, flat, flon);
    }
  }

}



void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// The callback is for receiving messages back from other sources
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

// Reconnect to the network if connection is lost
void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // mqttClient.publish("TT", "Connected to Mqtt");
      // ... and resubscribe
      mqttClient.subscribe("TT");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Setup Mqtt
void setupMqtt()
{
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
  while (!mqttClient.connected()) {
    reconnect();
  }
  //Serial.println();
  //Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
}


//Nothing to loop thru because deep sleep
void loop()
{
  //  if (!mqttClient.connected()) {
  //    reconnect();
  //  }
  //  mqttClient.loop();

  //    long now = millis();
  //    if (now - lastMsg > FREQUENCYOFREADING) {
  //  getGpsReading();
  //      lastMsg = now;
  //    }
}



