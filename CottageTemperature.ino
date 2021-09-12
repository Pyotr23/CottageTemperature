#include <DHTesp.h>

// Вывод для управления датчиком влажности и температуры.
const uint8_t DHT11_PIN = 2;
const uint8_t RELE1_PIN = 3;
const uint8_t RELE2_PIN = 4;

// Версия устройства.
const char VERSION[] = "4.0"; 

// Пауза в каждой итерации основного цикла, в мс.
double loopDelay = 10000;
// Периодичность уведомлений, в мс.
double notificationDelay = 60000;
int notificationCountValue = ceil(notificationDelay / loopDelay);
int notificationCounter = 0;

// Значение нижнего температурного порога.
int lowTemperatureTreshold = 23;  
// Значение верхнего температурного порога.
int highTemperatureTreshold = 24;

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
    pinMode(RELE1_PIN, OUTPUT);
    pinMode(RELE2_PIN, OUTPUT);
    dhtModule.setup(DHT11_PIN, DHTesp::DHT11);  
    digitalWrite(RELE1_PIN, HIGH);
    digitalWrite(RELE2_PIN, HIGH);
}
 
void loop() {    
    if (notificationCounter++ == notificationCountValue){
        Serial.print(lowTemperatureTreshold);
        Serial.print("---");
        Serial.print(highTemperatureTreshold);
        Serial.print("---");
        Serial.print(isContiniousHeating);
        Serial.print("---");
        Serial.print(temperature);
        Serial.print("---");
        Serial.print(humidity);
        Serial.print("---");
        Serial.print(isHeating);
        Serial.print("---");
        Serial.println(IsOnRelay());

        notificationCounter = 1;
    }

    delay(loopDelay);    

    WriteParameters();
    digitalWrite(RELE1_PIN, IsOnRelay());
    digitalWrite(RELE2_PIN, IsOnRelay());

    if (Serial.available() > 0){
        String command = GetSerialPortText();    

        Serial.println(command);
        Serial.println("----------");
      
        int index;
 
        if (command.startsWith("/min")){
            index = command.indexOf(':');
            lowTemperatureTreshold = command.substring(index + 1).toInt();            
        }            
        else if (command.startsWith("/max")){
            index = command.indexOf(':');
            highTemperatureTreshold = command.substring(index + 1).toInt();
        }
        else if (command.startsWith("/heat")){
            index = command.indexOf(':');
            isContiniousHeating = command.substring(index + 1).toInt();
        }
        else if (command.startsWith("/info")){
            highTemperatureTreshold = 23;
        }

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

    if (temperature >= highTemperatureTreshold){
    wasHeating = isHeating;
    isHeating = false; 
    if (wasHeating != isHeating){   // сообщение, что прекратился нагрев 
        Serial.println("Прекратился нагрев");
        // digitalWrite(RELE1_PIN, HIGH);
        // digitalWrite(RELE2_PIN, HIGH);
    }         
    //   SendEmail(GetMessageText(currentMode));  
    return true; 
    } 

    if (temperature <= lowTemperatureTreshold){
    wasHeating = isHeating;
    isHeating = true; 
    if (wasHeating != isHeating) {  // сообщение, что начался нагрев  
        Serial.println("Пошёл нагрев");
        // digitalWrite(RELE1_PIN, LOW);
        // digitalWrite(RELE2_PIN, LOW);
    }      
    //   SendEmail(GetMessageText(currentMode));   
    return false;   
    }

    return !isHeating;
}
