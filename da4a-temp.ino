#include <dht.h>
#include <SoftwareSerial.h>

// Пин приёма данных от модуля SIM800L
const int RX_PIN = 2;     
// Пин передачи данных модулю SIM800L
const int TX_PIN = 3;     

// Пин датчика температуры и влажности
const int DHT11_PIN = 4;  
// Пин реле
const int RELAY_PIN = 5;  

// Значение верхнего температурного порога
const int HIGH_TEMPERATURE_TRESHOLD = 26;   
// Значение нижнего температурного порога
const int LOW_TEMPERATURE_TRESHOLD = 23;    
// Небольшая задержка (в мс)
const int DELAY_IN_MS = 500;                
// Большая задержка (для основного цикла, в мс)
const int BIG_DELAY_IN_MS = 5000;           
// Номер телефона, на который будут отправляться смски. Всем привет!
const String PHONE_NUMBER = "+79296135951";  
// Период времени (в часах), в течение которого будет замкнуто реле 
const int WARM_HOUSE_PERIOD_IN_HOURS = 3; 
// const int WARM_HOUSE_PERIOD_IN_HOURS = 6; 

// Количество миллисекунд в часе
const long MS_IN_HOUR = 60000; 
// const long MS_IN_HOUR = 3600000;   

// Название команды для предоставления информации по СМС
const String INFO_COMMAND = "info"; 
// Название режима "Тёплый дом" и соответствующей команды
const String WARM_HOUSE = "warm house";     
// Название режима "Простой"
const String DOWNTIME = "downtime";
// Название режима "Нагрев"
const String HEATING = "heating";

// Символы с последующим сообщением ОК в ответе модуля SIM800L
const String DELIMITER_BETWEEN_REQUEST_AND_RESPONSE = 
  String(char(13)) + String(char(13)) + String(char(10)) + String("OK");  

// Класс датчика DHT11
dht DHT;   
// Класс обмена данными с модулем SIM800L
SoftwareSerial simModule(RX_PIN, TX_PIN); 
// Флаг режима отладки (включён вывод в последовательный порт)
boolean isDebug = true;   
// Текст отправляемого SMS
String sendText;   
// Режим, в котором сейчас находится устройство                       
String mode = HEATING;  
// Счётчик количества итераций основного цикла в режиме "тёплый дом"                            
int warmHouseCounter = 0; 
// Текущее значение температуры в градусах Цельсия
int temperature = 0;
// Текущее значение влажности в процентах
int humidity = 0;

// количество итераций основного цикла в режиме "тёплый дом"
long warmHouseTickNumber = WARM_HOUSE_PERIOD_IN_HOURS * MS_IN_HOUR / BIG_DELAY_IN_MS;

void setup(){
  pinMode(DHT11_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT); 

  Serial.begin(9600);               
  simModule.begin(9600); 
  digitalWrite(RELAY_PIN, HIGH);

  delay(DELAY_IN_MS); 
  PrintlnInDebug("Start!");
  ConnectToSimModule();
  SendRequest("AT+CMGF=1");  
  SendRequest("AT+CNMI=1,2,0,0,0");
}

void loop(){  
  delay(BIG_DELAY_IN_MS);
  if (mode == WARM_HOUSE)
    warmHouseCounter++;

  PrintlnInDebug(GetSignalLevel());
  PrintInDebug(String(warmHouseTickNumber) + " ");
  PrintlnInDebug(String(warmHouseCounter));

  int chk = DHT.read11(DHT11_PIN);
  temperature = (int)DHT.temperature;
  humidity = (int)DHT.humidity;
  PrintDhtParameters();  

  digitalWrite(RELAY_PIN, IsOnRelay());  
  
  String receivedText = GetReceivedText();    
  if (receivedText == "")
    return;   
  PrintlnInDebug(receivedText);
  if (receivedText == INFO_COMMAND)
    sendText = "m=" + mode + "; " + GetParametersString();
  else if (receivedText == WARM_HOUSE){
    mode = WARM_HOUSE;
    sendText = "Start warm house; " + GetParametersString();
  }
    
  SendSms(sendText);
  PrintlnInDebug(sendText);  
}

// Получить строку со значениями уровня сигнала, температуры и влажности.
String GetParametersString(){
  return "s=" + GetSignalLevel() + "; t=" + String(temperature) + "; h=" + String(humidity);
}

// Отправить сообщение.
void SendSms(String text) 
{    
  PrintlnInDebug("SMS send started");    
  simModule.println("AT+CMGS=\"" + PHONE_NUMBER + "\"");    
  delay(DELAY_IN_MS);    
  simModule.print(text);    
  delay(DELAY_IN_MS);    
  simModule.print((char)26);    
  delay(DELAY_IN_MS);    
  PrintlnInDebug("SMS send finish.");   
}

// Уровень сигнала, подаваемого на вход реле, исходя из значения температуры и её порогов.
// Если температура выше (или равна) значения верхнего порога, то подаётся 1 и реле РАЗМЫКАЕТСЯ.
// Если температура стала ниже (или равна) значения нижнего порога, то подаётся 0 и реле замыкается.
boolean IsOnRelay(){
  if (mode == WARM_HOUSE){
    if (warmHouseCounter < warmHouseTickNumber)
      return false;
    SendSms("Stop warm house; " + GetParametersString());
    warmHouseCounter = 0;
    mode = HEATING;
  } 

  if (temperature >= HIGH_TEMPERATURE_TRESHOLD){
    mode = DOWNTIME;
    return true; 
  } 

  if (temperature <= LOW_TEMPERATURE_TRESHOLD){
    mode = HEATING; 
    return false;   
  }
   
  return mode == DOWNTIME;
}

// Циклически пытаться соединиться с SIM800L до успеха. 
void ConnectToSimModule(){
  while(!simModule.available()){             
    simModule.println("AT");
    PrintlnInDebug("Connecting...");         
    delay(DELAY_IN_MS);    
  }
  PrintlnInDebug("Connected!");           
  PrintResponse();  
}

// Отправить запрос к SIM800L и напечатать ответ.
void SendRequest(String request){
  simModule.println(request);
  delay(DELAY_IN_MS);
  PrintResponse();
}

// Напечатать ответ от SIM800L в одну строку с двоеточием перед ОК.
void PrintResponse(){  
  String response = GetResponse();
  response.replace(DELIMITER_BETWEEN_REQUEST_AND_RESPONSE, ": OK");
  PrintInDebug(response);    
}

// Получить уровень сигнала мобильной связи в формате <XX,X>.
String GetSignalLevel(){
  simModule.println("AT+CSQ");
  delay(DELAY_IN_MS);
  String response = GetResponse();
  int signalStartIndex = response.indexOf(':') + 2;
  response = response.substring(signalStartIndex);
  int signalEndIndex = response.indexOf(char(13));
  return response.substring(0, signalEndIndex);
}

// Прочитать ответ от SIM800L.
String GetResponse(){
  String response = "";
  while(simModule.available()){ 
    char symbolNumber = simModule.read();
    if (symbolNumber != 0)           
      response += char(symbolNumber); 
  }  
  return response;
}

// Получить текст сообщения.
// Он находится между последним символом №13 и предпоследним символом №10.
String GetReceivedText()
{
  delay(DELAY_IN_MS);
  if (!simModule.available())
    return "";

  String text = ""; 
  while(simModule.available()) 
  {
    text += char(simModule.read());
  }  
  text.toLowerCase();
  PrintlnInDebug(text);
  if (text.indexOf(INFO_COMMAND) != -1) {    
    return INFO_COMMAND;
  }

  if (text.indexOf(WARM_HOUSE) != -1) {
    return WARM_HOUSE;
  } 

  return "";
}

// Очистка последовательного порта от данных.
void ClearSerialPort(){
  while (Serial.available()) 
  {
    Serial.read(); 
  }
}

// Вывести текст в последовательный порт в режиме отладки.
void PrintInDebug(String text){
  if (isDebug){
    Serial.print(text);
  }
}

// Вывести текст с новой строкой в последовательный порт в режиме отладки.
void PrintlnInDebug(String text){
  if (isDebug){
    Serial.println(text);
  }
}

// Напечатать значения температуры и влажности.
void PrintDhtParameters(){
  PrintlnInDebug("Temperature = " + String(temperature));
  PrintlnInDebug("Humidity = " + String(humidity));  
} 