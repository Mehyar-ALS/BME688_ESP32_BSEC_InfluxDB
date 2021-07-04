#include <Arduino.h>
#include <WiFi.h>
#include <InfluxDbClient.h>
#include "bsec.h"

void checkIaqSensorStatus(void);
void errLeds(void);
void wifi(void);
void influxDB(void);

Bsec iaqSensor;
String output;

#define INFLUXDB_URL "http://192.168.x.x:8086"
#define INFLUXDB_DB_NAME "DataBaseName"
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point point("BME688");

const char *ssid = "SSID";
const char *password = "PASSWORD";

void setup(void)
{
  Serial.begin(115200);
  while (!Serial)
    delay(10); // wait for console
  Wire.begin();

  wifi();
  influxDB();

  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  output = "raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%], Static IAQ, CO2 equivalent, breath VOC equivalent";
  Serial.println(output);
}

void loop(void)
{
  point.clearFields();
  output = "";

  if (iaqSensor.run())
  { // If new data is available
    point.addField("Raw_Temperature", iaqSensor.rawTemperature);
    point.addField("Temperature", iaqSensor.temperature);
    point.addField("Pressure", iaqSensor.pressure);
    point.addField("Raw_Relative_Humidity", iaqSensor.rawHumidity);
    point.addField("Relative_Humidity", iaqSensor.humidity);
    point.addField("gas", iaqSensor.gasResistance);
    point.addField("IAQ", iaqSensor.iaq);
    point.addField("IAQ_accuracy", iaqSensor.iaqAccuracy);
    point.addField("Static_IAQ", iaqSensor.staticIaq);
    point.addField("CO2_equivalent", iaqSensor.co2Equivalent);
    point.addField("breath_VOC_equivalent", iaqSensor.breathVocEquivalent);

    if (WiFi.status() != WL_CONNECTED)
    {
      wifi();
    }

    if (!client.writePoint(point))
    {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
      Serial.println("Trying to reconnect with InfluxDB");
      influxDB();
    }

    output += String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.rawHumidity);
    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaq);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    output += ", " + String(iaqSensor.staticIaq);
    output += ", " + String(iaqSensor.co2Equivalent);
    output += ", " + String(iaqSensor.breathVocEquivalent);
    Serial.println(output);
  }
  else
  {
    checkIaqSensorStatus();
  }
  delay(1000);
}

void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK)
  {
    if (iaqSensor.status < BSEC_OK)
    {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK)
  {
    if (iaqSensor.bme680Status < BME680_OK)
    {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

void wifi(void)
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void influxDB(void)
{
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}