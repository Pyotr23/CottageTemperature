#include <dht.h>
#include <SoftwareSerial.h>

#define DHT11_PIN 4
#define RX_PIN 2
#define TX_PIN 3

const int HIGH_ALARM_HUMIDITY_TRESHOLD = 30;
const int HIGH_ARARM_TEMPERATURE_TRESHOLD = 27;
const int DELAY_IN_MS = 500;   
const int BIG_DELAY_IN_MS = 5000;   
const String PHONE_NUMBER = "+79296135951";            

// символы с последующим сообщением ОК в ответе модуля SIM800L
const String DELIMITER_BETWEEN_REQUEST_AND_RESPONSE = 
  String(char(13)) + String(char(13)) + String(char(10)) + String("OK");  

dht DHT;                                  // класс датчика DHT11
SoftwareSerial simModule(RX_PIN, TX_PIN); // класс обмена данными с модулем SIM800L
boolean isSend = false;                   // флаг отправки сообщения (только для режима тестирования)
boolean isDebug = true;                   // флаг режима отладки (включён вывод в последовательный порт)
String sendText;                          // отправляемое сообщение

void setup(){
  pinMode(DHT11_PIN, INPUT);

  Serial.begin(9600);               
  simModule.begin(9600); 

  delay(DELAY_IN_MS); 
  PrintlnInDebug("Start!");
  ConnectToSimModule();
  SendRequestAndPrintResponse("AT+CMGF=1");  
  SendRequestAndPrintResponse("AT+CNMI=1,2,0,0,0");
  // sms("Hello world", "+79296135951");
}

void loop(){
  delay(BIG_DELAY_IN_MS);

  int chk = DHT.read11(DHT11_PIN);
  int temperature = (int)DHT.temperature;
  int humidity = (int)DHT.humidity; 

  PrintDhtParameters(temperature, humidity);  

  String receivedText = GetReceivedText(); 
  PrintlnInDebug(receivedText);
  if (receivedText == "")
    return;
  else if (receivedText == "Info") {
    PrintlnInDebug("Есть Info!");
    sendText = "Signal=" + GetSignalLevel() + " temperature=" + String(temperature) + " humidity=" + String(humidity);
    SendSms(sendText);
  }  
  PrintlnInDebug(sendText);
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
void SendRequestAndPrintResponse(String request){
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
  // PrintInDebug("Сообщение: ");
  // PrintlnInDebug(text);
  int newLineLastIndex = text.lastIndexOf(String(char(13)));
  // PrintlnInDebug(String(newLineLastIndex));
  text = text.substring(0, newLineLastIndex);
  // PrintlnInDebug(text);
  int firstLetterIndex = text.lastIndexOf(String(char(10))) + 1;
  // PrintlnInDebug(String(firstLetterIndex));
  text = text.substring(firstLetterIndex);
  // PrintlnInDebug(String(text.length()));
  return text;
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
void PrintDhtParameters(int temperature, int humidity){
  PrintlnInDebug("Temperature = " + String(temperature));
  PrintlnInDebug("Humidity = " + String(humidity));  
} 