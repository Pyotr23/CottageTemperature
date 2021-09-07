#include <DHTesp.h>

// вывод для управления датчиком влажности и температуры
const uint8_t DHT11_PIN = 2;
// датчик температуры и влажности DHT
DHTesp dhtModule;
// текущее значение температуры
int temperature;
// текущее значение влажности
int humidity;

char incomingByte;
int min = 10;
int max = 30;
int index;
bool isContiniousHeating = false;
String command;

void setup()
{
    Serial.begin(9600);  
    dhtModule.setup(DHT11_PIN, DHTesp::DHT11);  
}
 
void loop() {
    delay(3000);

    if (Serial.available() > 0){
        command = GetSerialPortText();    

        Serial.println(command);
        Serial.println("----------");
      
        if (command.startsWith("/min")){
            index = command.indexOf(':');
            min = command.substring(index + 1).toInt();            
        }            
        else if (command.startsWith("/max")){
            index = command.indexOf(':');
            max = command.substring(index + 1).toInt();
        }
        else if (command.startsWith("/heat")){
            index = command.indexOf(':');
            isContiniousHeating = command.substring(index + 1).toInt();
        }
        else if (command.startsWith("/info")){
            max = 23;
        }

        WriteParameters();

        Serial.println(min);
        Serial.println(max);
        Serial.println(isContiniousHeating);
        Serial.println(temperature);
        Serial.println(humidity);

        command = "";
    } 
}

// Записать текущие значения температуры и влажности.
void WriteParameters(){
  temperature = (int)dhtModule.getTemperature();
  humidity = (int)dhtModule.getHumidity();
}

String GetSerialPortText(){
    String text;
    while (Serial.available() > 0) {  
        char incomingByte = Serial.read();            
        if (incomingByte == 10 || incomingByte == 13)   // выход из метода при достижении символов конца строки
            continue;
        text += incomingByte;
    }   
    return text;   
}
