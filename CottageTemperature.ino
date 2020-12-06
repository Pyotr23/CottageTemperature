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

// адрес SMTP-сервера
const char* SMTP_SERVER_ADDRESS = "smtp.gmail.com";
// порт подключения к SMTP-серверу
const int SMTP_SERVER_PORT = 465;
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
const int LONG_DELAY = 30000;
// небольшая задержка (в мс)
const int SHORT_DELAY = 1000;

// текст сообщения при включении
const String WELCOME_MESSAGE = "The Box v3.0 включен";

// период времени (в часах), по истечении которого будет отправлено сообщение с текущими параметрами,
// если режим устройства не менялся 
const int PERIOD_WITHOUT_CHANGES_IN_HOURS = 6; 
// количество миллисекунд в часе
const long MS_IN_HOUR = 3600000;
// количество итераций основного цикла без изменений режима до отправки письма 
const long pauseTickNumber = PERIOD_WITHOUT_CHANGES_IN_HOURS * MS_IN_HOUR / LONG_DELAY;

// переменная, представляющая WiFi-клиент
WiFiClientSecure wiFiClient;
// значение счётчика, который считает итерации до отправки письма
long counter;
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

void setup() {
  Serial.begin(115200);
  dhtModule.setup(DHT11_PIN, DHTesp::DHT11);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  ConnectToWiFi();
  delay(SHORT_DELAY);
  WriteParameters(); 
  SendEmail(GetInitialMessageText());
}
 
void loop() {  
  WriteParameters();  
  digitalWrite(RELAY_PIN, IsOnRelay());
  PrintData();

  delay(LONG_DELAY); 

  counter++;
  if (counter >= pauseTickNumber){
    if (currentMode == DOWNTIME)
      SendEmail(GetMessageText(GOOD_CLIMATE));
    else 
      SendEmail(GetMessageText(HEATING_CONTINUES));    
  }                      
}

// Вывести температуру, влажность и сообщение текущего режима.
void PrintData(){
  Serial.print(counter);
  Serial.print(" ");
  Serial.print(pauseTickNumber);
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
// Если температура выше (или равна) значения верхнего порога, то подаётся 0 и реле размыкается.
// Если температура стала ниже (или равна) значения нижнего порога, то подаётся 1 и реле замыкается.
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

// Подключиться к WiFi, используя конфигурацию из файла "settings.h".
void ConnectToWiFi(){
  Serial.print("Попытка подключения к сети ");
  Serial.println(SSID);

  WiFi.begin(SSID, SSID_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(SHORT_DELAY);
    Serial.print("*");
  }

  Serial.println();
  Serial.print("IP-адрес модуля: ");
  Serial.println(WiFi.localIP());
}

// Отправить письмо
byte SendEmail(String text)
{
  counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(SHORT_DELAY);
    Serial.print("*");
  }

  Serial.println("Попытка подключения к SMTP-серверу");
  wiFiClient.setInsecure();
  if (wiFiClient.connect(SMTP_SERVER_ADDRESS, SMTP_SERVER_PORT)) 
    Serial.println(F("Подключено!"));
  else {
    Serial.print(F("Соединение не выполнено"));
    return 0;
  }
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Отправка команды о начале передачи письма"));
  wiFiClient.println("EHLO gmail.com");
  if (!IsReceiveResponse())
    return 0;
  
  Serial.println(F("Отправка команды о начале авторизации"));
  wiFiClient.println("auth login");
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Отправка логина"));
  wiFiClient.println(base64::encode(GMAIL_FROM));
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Отправка пароля"));
  wiFiClient.println(base64::encode(GMAIL_PASSWORD));
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Отправка адреса почты автора письма"));
  String mailFrom = String("MAIL FROM: <" + GMAIL_FROM + ">");
  wiFiClient.println(mailFrom);
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Sending To"));
  String mailRcptToFirst = String("RCPT To: <" + MAIL_TO_FIRST + ">");
  wiFiClient.println(mailRcptToFirst);  // Повторить для каждого получателя 
  String mailRcptToSecond = String("RCPT To: <" + MAIL_TO_SECOND + ">");
  wiFiClient.println(mailRcptToSecond);
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Отправка команды о начале передачи тела письма"));
  wiFiClient.println(F("DATA"));
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Sending email"));
  // recipient address (include option display name if you want)
  String mailTo = String("To: <" + MAIL_TO_FIRST + ">, <" + MAIL_TO_SECOND + ">");
  wiFiClient.println(mailTo);
  // "Home Alone Group" - название группы получателей. Будет отображаться в строке получателей.
  // wiFiClient.println(F("To: Home Alone Group<totally@made.up>")); 
  
  wiFiClient.println(String("From: " + GMAIL_FROM));
  wiFiClient.println(F("Subject: The Box 2.0\r\n"));
  wiFiClient.println(text);
  // Каждую новую строку нужно отправлять отдельно.
  // wiFiClient.println(F("In the last hour there was: 8 activities detected. Please check all is well."));
  // wiFiClient.println(F("This email will NOT be repeated for this hour.\n"));
  // wiFiClient.println(F("This email was sent from an unmonitored email account - please do not reply."));
  // wiFiClient.println(F("Love and kisses from Dougle and Benny. They wrote this sketch."));

  // Для окончания отправки необходимо отправить отдельную точку.
  wiFiClient.println(F("."));
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Завершение отправки"));
  wiFiClient.println(F("QUIT"));
  if (!IsReceiveResponse())
    return 0;

  wiFiClient.stop();
  Serial.println(F("Соединение с SMTP-сервером разорвано"));
  return 1;
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
