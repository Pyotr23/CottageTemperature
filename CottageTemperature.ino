#include <DHTesp.h>
#include <base64.h>
#include <ESP8266WiFi.h>
#include "settings.h"

// вывод для управления реле
const uint8_t RELAY_PIN = D5;
// вывод для управления датчиком влажности и температуры
const uint8_t DHT11_PIN = D6;

// Значение нижнего температурного порога
const int LOW_TEMPERATURE_TRESHOLD = 5;  
// Значение верхнего температурного порога
const int HIGH_TEMPERATURE_TRESHOLD = 9;
// Версия устройства
const String VERSION = "3.3";     

// время (в мс) ожидания ответа от сервера, по истечении которого закрывается подключение
const int DELAY_FOR_RESPONSE = 10000;

// сообщение при включённом реле 
const char* HEATING = "Включились нагреватели";
// сообщение при выключенном реле
const char* DOWNTIME = "В доме стало тепло";
// сообщение, что долго идёт нагрев
const char* HEATING_CONTINUES = "Продолжается нагрев";
// сообщение, что в доме хорошо
const char* GOOD_CLIMATE = "Тёплый дом ждёт";

// продолжительная задержка (в мс, для основного цикла)
const int LONG_DELAY = 60000;
// небольшая задержка (в мс)
const int SHORT_DELAY = 1000;

// текст сообщения при включении
const String WELCOME_MESSAGE = "The Box " + VERSION + " включен";

// период времени (в часах), по истечении которого будет отправлено сообщение с текущими параметрами,
// если режим устройства не менялся 
const int PERIOD_WITHOUT_CHANGES_IN_HOURS = 2; 
// количество миллисекунд в часе
const long MS_IN_HOUR = 3600000;
// количество отправленных писем с малым интервалом в начале работы
const int START_MAIL_COUNT = 3;

// количество итераций основного цикла без изменений режима до отправки письма 
const long PAUSE_TICK_NUMBER = PERIOD_WITHOUT_CHANGES_IN_HOURS * MS_IN_HOUR / LONG_DELAY;
// количество секунд для подключения к WiFi (в мс)
const int TIME_FOR_WIFI_CONNECTION = 15000;
// количество итераций для подключения к WiFi
const int WIFI_CONNECTION_TICK_NUMBER = TIME_FOR_WIFI_CONNECTION / SHORT_DELAY;

// переменная, представляющая WiFi-клиент
WiFiClientSecure wiFiClient;
// значение счётчика, который считает итерации до отправки письма
long counter;
// значение счётчика отправленных писем с малым интервалом в начале работы
int startCounter;
// текущее положение дел
String currentMode = DOWNTIME;
// режим перед сравнением
String oldWorkMode;
// текущее значение температуры
int temperature;
// текущее значение влажности
int humidity;
// датчик температуры и влажности DHT
DHTesp dhtModule;
// флаг успешной отправки письма
bool wasSendEmail = true;

void setup() {
  Serial.begin(115200);
  dhtModule.setup(DHT11_PIN, DHTesp::DHT11);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // при использовании реле KY-019 необходимо разомкнуть его при старте  
  WriteParameters(); 
  delay(SHORT_DELAY);
  SendEmail(GetInitialMessageText());
}
 
void loop() {  
  WriteParameters();  
  digitalWrite(RELAY_PIN, IsOnRelay());
  PrintData();

  delay(LONG_DELAY); 

  if (startCounter < START_MAIL_COUNT) {
    startCounter++;
    if (currentMode == DOWNTIME)
      SendEmail(GetMessageText(GOOD_CLIMATE));
    else 
      SendEmail(GetMessageText(HEATING_CONTINUES));  
    return;  
  }

  counter++;
  
  if (counter < PAUSE_TICK_NUMBER && wasSendEmail){
    return;
  }
  
  if (currentMode == DOWNTIME)
    SendEmail(GetMessageText(GOOD_CLIMATE));
  else 
    SendEmail(GetMessageText(HEATING_CONTINUES));    
}

// Вывести температуру, влажность и сообщение текущего режима.
void PrintData(){
  Serial.print(counter);
  Serial.print(" ");
  Serial.print(PAUSE_TICK_NUMBER);
  Serial.print(" ");
  Serial.print(temperature);  
  Serial.print(" ");
  Serial.print(humidity);
  Serial.print(" ");
  Serial.println(currentMode);
}

// Получить информационную строку.
String GetMessageText(String initialPart){
  return initialPart + " (температура = " + String(temperature) + "C, влажность = " + String(humidity) + ")."; 
}

// Получить информационную строку при включении.
String GetInitialMessageText(){
  return WELCOME_MESSAGE + " (диапазон поддерживаемой температуры = [" + LOW_TEMPERATURE_TRESHOLD
    + "..." + HIGH_TEMPERATURE_TRESHOLD + "], температура = " + String(temperature) 
    + "C, влажность = " + String(humidity) + "%)."; 
}

// Записать текущие значения температуры и влажности.
void WriteParameters(){
  temperature = (int)dhtModule.getTemperature();
  humidity = (int)dhtModule.getHumidity();
}

// Уровень сигнала, подаваемого на вход реле, исходя из значения температуры и её порогов.
// Если температура выше (или равна) значения верхнего порога, то подаётся 1 и KY-019 размыкается.
// Если температура стала ниже (или равна) значения нижнего порога, то подаётся 0 и KY-019 замыкается.
boolean IsOnRelay(){
  if (temperature >= HIGH_TEMPERATURE_TRESHOLD){
    oldWorkMode = currentMode;
    currentMode = DOWNTIME; 
    if (oldWorkMode != currentMode)       
      SendEmail(GetMessageText(currentMode));  
    return true; 
  } 

  if (temperature <= LOW_TEMPERATURE_TRESHOLD){
    oldWorkMode = currentMode;
    currentMode = HEATING; 
    if (oldWorkMode != currentMode)       
      SendEmail(GetMessageText(currentMode));  
    return false;   
  }
   
  return currentMode != HEATING;
}

// Удалось ли подключиться к WiFi, используя конфигурацию из файла "settings.h".
bool IsConnectToWiFi(){
  Serial.print("Попытка подключения к сети ");
  Serial.println(SSID);

  WiFi.begin(SSID, SSID_PASSWORD);
  
  for (int i = 0; i < WIFI_CONNECTION_TICK_NUMBER; i++){
    delay(SHORT_DELAY);
    Serial.print("*");
    if (WiFi.status() == WL_CONNECTED)
      break;
  }

  if (WiFi.status() != WL_CONNECTED)
    return false;

  Serial.println();
  Serial.print("IP-адрес модуля: ");
  Serial.println(WiFi.localIP());
  return true;
}

// Отправить письмо
void SendEmail(String text)
{
  if (!IsConnectToWiFi()){
    wasSendEmail = false;
    return;
  }    

  counter = 0;  

  Serial.println("Попытка подключения к SMTP-серверу");
  wiFiClient.setInsecure();
  if (wiFiClient.connect(SMTP_SERVER_ADDRESS, SMTP_SERVER_PORT)) 
    Serial.println(F("Подключено!"));
  else {
    Serial.print(F("Соединение не выполнено"));
    wasSendEmail = false;
    return;
  }
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  Serial.println(F("Отправка команды о начале передачи письма"));
  wiFiClient.println("EHLO gmail.com");
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }
  
  Serial.println(F("Отправка команды о начале авторизации"));
  wiFiClient.println("auth login");
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  Serial.println(F("Отправка логина"));
  wiFiClient.println(base64::encode(SMTP_SERVER_LOGIN));
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  Serial.println(F("Отправка пароля"));
  wiFiClient.println(base64::encode(SMTP_SERVER_PASSWORD));
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }
    
  Serial.println(F("Отправка адреса почты автора письма"));
  String mailFrom = String("MAIL FROM: <" + SMTP_SERVER_LOGIN + ">");
  wiFiClient.println(mailFrom);
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  Serial.println(F("Sending To"));
  String mailRcptToFirst = String("RCPT To: <" + MAIL_TO_FIRST + ">");
  wiFiClient.println(mailRcptToFirst);  // Повторить для каждого получателя 
  String mailRcptToSecond = String("RCPT To: <" + MAIL_TO_SECOND + ">");
  wiFiClient.println(mailRcptToSecond);
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }    

  Serial.println(F("Отправка команды о начале передачи тела письма"));
  wiFiClient.println(F("DATA"));
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  Serial.println(F("Sending email"));
  // recipient address (include option display name if you want)
  String mailTo = String("To: <" + MAIL_TO_FIRST + ">, <" + MAIL_TO_SECOND + ">");
  wiFiClient.println(mailTo);
  // "Home Alone Group" - название группы получателей. Будет отображаться в строке получателей.
  // wiFiClient.println(F("To: Home Alone Group<totally@made.up>")); 
  
  wiFiClient.println(String("From: " + SMTP_SERVER_LOGIN));
  String subject = "Subject: The Box " + VERSION + "\r\n";
  wiFiClient.println(subject);
  wiFiClient.println(text);
  // Каждую новую строку нужно отправлять отдельно.
  // wiFiClient.println(F("In the last hour there was: 8 activities detected. Please check all is well."));
  // wiFiClient.println(F("This email will NOT be repeated for this hour.\n"));
  // wiFiClient.println(F("This email was sent from an unmonitored email account - please do not reply."));
  // wiFiClient.println(F("Love and kisses from Dougle and Benny. They wrote this sketch."));

  // Для окончания отправки необходимо отправить отдельную точку.
  wiFiClient.println(F("."));
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  Serial.println(F("Завершение отправки"));
  wiFiClient.println(F("QUIT"));
  if (!IsReceiveResponse()){
    wasSendEmail = false;
    return;
  }

  wiFiClient.stop();
  Serial.println(F("Соединение с SMTP-сервером разорвано")); 

  wasSendEmail = true;
}

// Получен ли ответ от WiFi-клиента.
byte IsReceiveResponse()
{
  int loopCount = 0;
  while (!wiFiClient.available()) {
    delay(1);
    loopCount++;
    if (loopCount > DELAY_FOR_RESPONSE) {
      wiFiClient.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  // Получение кода ответа.
  byte respCode = wiFiClient.peek();

  while (wiFiClient.available())
    Serial.write(wiFiClient.read());

  if (respCode >= '4')
  {
    Serial.print("Failed in eRcv with IsReceiveResponse: ");
    Serial.print(respCode);
    return 0;
  }
  return 1;
}
