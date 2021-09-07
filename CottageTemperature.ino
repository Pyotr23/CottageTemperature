#include <DHTesp.h>

// Вывод для управления датчиком влажности и температуры.
const uint8_t DHT11_PIN = 2;

// Значение нижнего температурного порога.
const int LOW_TEMPERATURE_TRESHOLD = 10;  
// Значение верхнего температурного порога.
const int HIGH_TEMPERATURE_TRESHOLD = 30;
// Версия устройства.
const String VERSION = "4.0"; 

// Датчик температуры и влажности DHT.
DHTesp dhtModule;

// Текущее значение температуры.
int temperature;
// Текущее значение влажности.
int humidity;

// Флаг непрерывного нагрева. Установленные пороги при выставленном флаге игнорируются.
bool isContiniousHeating = false;
// Флаг нагрева.
bool isHeating = false;
// Флаг предыдущего состояния нагрева.
bool wasHeating = false;

void setup()
{
    Serial.begin(9600);  
    dhtModule.setup(DHT11_PIN, DHTesp::DHT11);  
}
 
void loop() {
    delay(3000);

    if (Serial.available() > 0){
        String command = GetSerialPortText();    

        Serial.println(command);
        Serial.println("----------");
      
        int index;
 
        if (command.startsWith("/min")){
            index = command.indexOf(':');
            LOW_TEMPERATURE_TRESHOLD = command.substring(index + 1).toInt();            
        }            
        else if (command.startsWith("/max")){
            index = command.indexOf(':');
            HIGH_TEMPERATURE_TRESHOLD = command.substring(index + 1).toInt();
        }
        else if (command.startsWith("/heat")){
            index = command.indexOf(':');
            isContiniousHeating = command.substring(index + 1).toInt();
        }
        else if (command.startsWith("/info")){
            HIGH_TEMPERATURE_TRESHOLD = 23;
        }

        WriteParameters();

        Serial.println(LOW_TEMPERATURE_TRESHOLD);
        Serial.println(HIGH_TEMPERATURE_TRESHOLD);
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
        if (incomingByte == 10 || incomingByte == 13)   // символы конца строки пропускаются
            continue;
        text += incomingByte;
    }   
    return text;   
}

// Уровень сигнала, подаваемого на вход реле, исходя из значения температуры и её порогов.
// Если температура выше (или равна) значения верхнего порога, то подаётся 1 и KY-019 размыкается.
// Если температура стала ниже (или равна) значения нижнего порога, то подаётся 0 и KY-019 замыкается.
boolean IsOnRelay(){
    if (isContiniousHeating){
        isHeating = false;
        return false;
    }


    if (temperature >= HIGH_TEMPERATURE_TRESHOLD){
    wasHeating = isHeating;
    isHeating = false; 
    // if (wasHeating != isHeating)    // сообщение, что прекратился нагрев       
    //   SendEmail(GetMessageText(currentMode));  
    return true; 
    } 

    if (temperature <= LOW_TEMPERATURE_TRESHOLD){
    wasHeating = isHeating;
    isHeating = true; 
    // if (wasHeating != isHeating)       
    //   SendEmail(GetMessageText(currentMode));   // сообщение, что начался нагрев  
    return false;   
    }

    return !isHeating;
}
