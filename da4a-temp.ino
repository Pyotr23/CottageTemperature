#include <dht.h>
#include <SoftwareSerial.h>

// Флаг режима отладки (включён вывод в последовательный порт)
const boolean IS_DEBUG = true;   

// Пин приёма данных от модуля ESP8266
const int RX_PIN = 0;     
// Пин передачи данных модулю ESP8266
const int TX_PIN = 1;     

// Пин датчика температуры и влажности
const int DHT11_PIN = 4;  
// Пин реле
const int RELAY_PIN = 5;  

// Значение верхнего температурного порога
const int HIGH_TEMPERATURE_TRESHOLD = 26;   
// Значение нижнего температурного порога
const int LOW_TEMPERATURE_TRESHOLD = 23;   

// Небольшая задержка (в мс)
const int DELAY_IN_MS = 1000;                
// Большая задержка (для основного цикла, в мс)
const int BIG_DELAY_IN_MS = 5000;           

// Название режима "Простой"
const char DOWNTIME[] = "downtime";
// Название режима "Нагрев"
const char HEATING[] = "heating";

// Режим, в котором сейчас находится устройство                       
String workMode = HEATING;  

// Класс датчика DHT11
dht DHT;   
// Класс обмена данными с модулем ESP8266
SoftwareSerial wifiModule(RX_PIN, TX_PIN); 

// Текущее значение температуры (в градусах Цельсия)
int temperature = 0;
// Текущее значение влажности (в процентах)
int humidity = 0;

void setup(){
  if (IS_DEBUG)
    Serial.begin(115200);               
  wifiModule.begin(115200); 
  delay(DELAY_IN_MS);
  Serial.println("Preparing...");

  pinMode(DHT11_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  Serial.println("Clear");
  while (wifiModule.available()) 
    wifiModule.read();
  Serial.println("Ready");
}

void loop(){  
  if (Serial.available()){
    delay(1);
    int symbolNumber = Serial.read();
    Serial.print(symbolNumber);
    Serial.print(" ");    
    Serial.println((char)symbolNumber);
    wifiModule.write((char)symbolNumber);
  }

  if (wifiModule.available()) {
    delay(1);
    int wifiNumber = wifiModule.read();
    Serial.print(wifiNumber);
    Serial.print(" ");    
    Serial.println((char)wifiNumber);
  }

  // if (Serial.available()) {
  //   int symbolNumber = Serial.read();
  //   Serial.println();
  //   wifiModule.write(symbolNumber);
  // }    
}


// Уровень сигнала, подаваемого на вход реле, исходя из значения температуры и её порогов.
// Если температура выше (или равна) значения верхнего порога, то подаётся 1 и реле РАЗМЫКАЕТСЯ.
// Если температура стала ниже (или равна) значения нижнего порога, то подаётся 0 и реле замыкается.
boolean IsOnRelay(){
  if (temperature >= HIGH_TEMPERATURE_TRESHOLD){
    workMode = DOWNTIME;
    return true; 
  } 

  if (temperature <= LOW_TEMPERATURE_TRESHOLD){
    workMode = HEATING; 
    return false;   
  }
   
  return workMode == DOWNTIME;
}


// Вывести текст в последовательный порт в режиме отладки.
void PrintInDebug(String text){
  if (IS_DEBUG){
    Serial.print(text);
  }
}

// Вывести текст с новой строкой в последовательный порт в режиме отладки.
void PrintlnInDebug(String text){
  if (IS_DEBUG){
    Serial.println(text);
  }
}

// Напечатать значения температуры и влажности.
void PrintDhtParameters(){
  PrintlnInDebug("Temperature = " + String(temperature));
  PrintlnInDebug("Humidity = " + String(humidity));  
} 