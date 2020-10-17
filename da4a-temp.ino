#include <dht.h>
#include <SoftwareSerial.h>

#define DHT11_PIN 4
#define ALARM_PIN 8
#define RX_PIN 2
#define TX_PIN 3

dht DHT;
SoftwareSerial SIM800(RX_PIN, TX_PIN);

const int HIGH_ALARM_HUMIDITY_TRESHOLD = 30;
const int HIGH_ARARM_TEMPERATURE_TRESHOLD = 27;
const int START_DELAY_IN_MS = 1000;

boolean isSent = false;
long lastcmd = millis();

void setup(){
  Serial.begin(9600);               // Скорость обмена данными с компьютером
  SIM800.begin(9600); 
  delay(5000); 
  Serial.println("Start!");
  while(!SIM800.available()){             // Зацикливаем и ждем инициализацию SIM800L
    SIM800.println("AT");                  // Отправка команды AT
    delay(1000);                             // Пауза
    Serial.println("Connecting...");         // Печатаем текст
  }
  Serial.println("Connected!");            // Печатаем текст
  SIM800.println("AT+CMGF=1");           // Отправка команды AT+CMGF=1
  delay(1000);                             // Пауза
  SIM800.println("AT+CSCS=\"GSM\"");  
  delay(1000);                             // Пауза 
  // sms("Hello world", "+79296135951");
}

void loop(){
  if (SIM800.available())           // Ожидаем прихода данных (ответа) от модема...
    Serial.write(SIM800.read());    // ...и выводим их в Serial
  // if (Serial.available())           // Ожидаем команды по Serial...
  //   SIM800.write(Serial.read());    // ...и отправляем полученную команду модему

  // if (millis() - lastcmd > 5000) {  // Прошло ли 5 секунд
  //   lastcmd = millis();             // Фиксируем время
  //   SIM800.println("AT+CSQ");       // Запрашиваем информацию о качестве сигнала
  // }
}

void sms(String text, String phone) 
{    
  Serial.println("SMS send started");    
  SIM800.println("AT+CMGS=\"" + phone + "\"");    
  delay(1000);    
  SIM800.print(text);    
  delay(300);    
  SIM800.print((char)26);    
  delay(300);    
  Serial.println("SMS send finish");    
  delay(3000);  
}