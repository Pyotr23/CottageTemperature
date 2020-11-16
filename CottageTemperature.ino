#include <DHTesp.h>
#include <base64.h>
#include <ESP8266WiFi.h>
#include "settings.h"

// вывод для управления реле
const uint8_t RELAY_PIN = D5;
// вывод для управления датчиком влажности и температуры
const uint8_t DHT11_PIN = D6;

// адрес SMTP-сервера
const char* SMTP_SERVER_ADDRESS = "smtp.gmail.com";
// порт подключения к SMTP-серверу
const int SMTP_SERVER_PORT = 465;
// время (в мс) ожидания ответа от сервера, по истечении которого закрывается подключение
const int DELAY_FOR_RESPONSE = 10000;
// сообщение при включённом реле 
const char* HEATING = "Идёт нагрев.";
// сообщение при выключенном реле
const char* DOWNTIME = "В доме тепло.";
// продолжительная задержка (в мс, для основного цикла)
const int BIG_DELAY = 5000;

// переменная, представляющая WiFi-клиент
WiFiClientSecure wiFiClient;
// текущее положение дел
String currentMode = DOWNTIME;
// текущее значение температуры
int temperature;
// текущее значение влажности
int humidity;
// датчик температуры и влажности DHT
DHTesp dhtModule;

void setup() {
  Serial.begin(115200);
  dhtModule.setup(DHT11_PIN, DHTesp::DHT11);
  ConnectToWiFi();
  delay(1000);
  SendEmail("The Box v2.0 включен.");
}
 
void loop() {  
  temperature = (int)dhtModule.getTemperature();
  Serial.println(temperature);  
  humidity = (int)dhtModule.getHumidity();
  Serial.println(humidity);
  delay(BIG_DELAY);                       
}

// Подключиться к WiFi, используя конфигурацию из файла "settings.h".
void ConnectToWiFi(){
  Serial.print("Попытка подключения к сети ");
  Serial.println(SSID);

  WiFi.begin(SSID, SSID_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("*");
  }

  Serial.println();
  Serial.print("IP-адрес модуля: ");
  Serial.println(WiFi.localIP());
}

// Отправить письмо
byte SendEmail(String text)
{
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
  String mailRcptTo = String("RCPT To: <" + MAIL_TO + ">");
  wiFiClient.println(mailRcptTo);  // Повторить для каждого получателя 
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Отправка команды о начале передачи тела письма"));
  wiFiClient.println(F("DATA"));
  if (!IsReceiveResponse())
    return 0;

  Serial.println(F("Sending email"));
  // recipient address (include option display name if you want)
  String mailTo = String("To: <" + MAIL_TO + ">");
  wiFiClient.println(mailTo);
  // "Home Alone Group" - название группы получателей. Будет отображаться в строке получателей.
  // wiFiClient.println(F("To: Home Alone Group<totally@made.up>")); 
  
  wiFiClient.println(String("From: " + GMAIL_FROM));
  wiFiClient.println(F("Subject: Your Arduino\r\n"));
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
