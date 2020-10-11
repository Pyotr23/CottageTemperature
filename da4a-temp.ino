#include <dht.h>

dht DHT;
const int HIGH_ALARM_HUMIDITY_TRESHOLD = 30;

#define DHT11_PIN 7
#define ALARM_PIN 8

void setup(){
  pinMode(ALARM_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop(){
  int chk = DHT.read11(DHT11_PIN);
  int temperature = (int)DHT.temperature;
  int humidity = (int)DHT.humidity;
  printDhtParameters(temperature, humidity);   
  boolean alarm = isAlarm(humidity, HIGH_ALARM_HUMIDITY_TRESHOLD);
  digitalWrite(ALARM_PIN, alarm);  
  delay(5000);
}

boolean isAlarm(int sensorValue, int treshold){
  return sensorValue >= treshold;
}

void printTemperature(int temperature){
  Serial.print("Temperature = ");
  Serial.println(temperature);  
}

void printHumidity(int humidity){
  Serial.print("Humidity = ");
  Serial.println(humidity);  
}

void printDhtParameters(int temperature, int humidity){
  printTemperature(temperature);
  printHumidity(humidity);
}