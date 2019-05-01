#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

#define WIFI_LED 26;
#define MQTT_LED 27;

WiFiClientSecure espClient;
PubSubClient client(espClient);

typedef enum
{
  LISTENING,
  READING
} P1_STATE;

P1_STATE state;
String telegram;

void connectWIFI()
{
  //  DO NOT TOUCH
  //  This is here to force the ESP32 to reset the WiFi and initialise correctly.
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  //  End silly stuff !!!

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT()
{
  /* Loop until reconnected */
  while (!client.connected())
  {
    Serial.print("MQTT connecting ...");
    /* connect now */
    if (client.connect(hostname, mqtt_username, mqtt_password))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200, SERIAL_8N1);

  //Read SD
  connectWIFI();
  client.setServer(mqtt_server, mqtt_port);
  connectMQTT();
  state = LISTENING;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void telegram_store(String line)
{
  telegram += line;
}

void telegram_reset()
{
  telegram = "";
  state = LISTENING;
}

void captureTelegram()
{

  if (Serial.available())
  {
    Serial.println("t");
    while (Serial.available())
    {
      String telegramLine = Serial.readStringUntil('\n');

      switch (state)
      {
      case LISTENING:
        if (telegramLine[0] == '/')
        {
          telegram_store(telegramLine);
          state = READING;
        }
        break;
      case READING:
        telegram_store(telegramLine);
        if (telegramLine[0] == '!')
        {
          DynamicJsonBuffer jsonBuffer;
          JsonObject &object = jsonBuffer.createObject();
          object["telegram"] = telegram;
          String telegramJSON;
          object.printTo(telegramJSON);
          client.publish("pihome-emon-4702", telegramJSON.c_str());
          telegram_reset();
        }
        // Add Line
        break;
      default:
        break;
      }
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    connectMQTT();
  }

  client.loop();
  captureTelegram();
}