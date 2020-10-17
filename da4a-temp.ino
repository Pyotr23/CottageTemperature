#include <dht.h>
#include <SoftwareSerial.h>

#define DHT11_PIN 7
#define ALARM_PIN 8
#define RX_PIN 2
#define TX_PIN 3

dht DHT;
SoftwareSerial sim800l(TX_PIN, RX_PIN);

const int HIGH_ALARM_HUMIDITY_TRESHOLD = 30;
const int HIGH_ARARM_TEMPERATURE_TRESHOLD = 27;
const int START_DELAY_IN_MS = 1000;

boolean isSent = false;

void setup(){
  pinMode(ALARM_PIN, OUTPUT);  
  Serial.begin(9600);
  Serial.println("Start!");
  sim800l.begin(9600);
  while(!sim800l.available()){             // Зацикливаем и ждем инициализацию SIM800L
    sim800l.println("AT");                  // Отправка команды AT
    delay(1000);                             // Пауза
    Serial.println("Connecting...");         // Печатаем текст
  }
  Serial.println("Connected!");            // Печатаем текст   
}

void loop(){
  if (sim800l.available())           // Ожидаем прихода данных (ответа) от модема...
    Serial.write(sim800l.read());    // ...и выводим их в Serial
  if (Serial.available())           // Ожидаем команды по Serial...
    sim800l.write(Serial.read());    // ...и отправляем полученную команду модему

  sim800l.println("AT+CSQ"); 

  int chk = DHT.read11(DHT11_PIN);
  int temperature = (int)DHT.temperature;
  int humidity = (int)DHT.humidity;
  PrintDhtParameters(temperature, humidity);   
  boolean isAlarm = IsAlarm(temperature, HIGH_ARARM_TEMPERATURE_TRESHOLD);
  if (isAlarm && !isSent) {
    isSent = true;
    SendSMS();
  }
  digitalWrite(ALARM_PIN, isAlarm);  
  delay(5000);
}

boolean IsAlarm(int sensorValue, int treshold){
  return sensorValue >= treshold;
}

void PrintTemperature(int temperature){
  Serial.print("Temperature = ");
  Serial.println(temperature);  
}

void PrintHumidity(int humidity){
  Serial.print("Humidity = ");
  Serial.println(humidity);  
}

void PrintDhtParameters(int temperature, int humidity){
  PrintTemperature(temperature);
  PrintHumidity(humidity);
}

void SendSMS()
{
  Serial.println("Sending SMS...");               //Show this message on serial monitor
  sim800l.print("AT+CMGF=1\r");                   //Set the module to SMS mode
  delay(100);
  sim800l.print("AT+CMGS=\"+79296135951\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  sim800l.print("SIM800l is working");       //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  sim800l.print((char)26);// (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);
}