#include <base64.h>
#include <ESP8266WiFi.h>
#include "settings.h"

// const char* _ssid = "UMKA";
// const char* _password = "2718281e";
// const char* _GMailServer = "smtp.gmail.com";
// const char* _mailUser = "2chilavert3@gmail.com";
// const char* _mailPassword = "zevod2323";

WiFiClientSecure wiFiClient;

void setup() {
  Serial.begin(115200);
  delay(10);
  ConnectToWiFi();
  delay(1000);
  // SendEmail();
}

void loop() {

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
  if (!response())
    return 0;

  Serial.println(F("Отправка команды о начале передачи письма"));
  wiFiClient.println("EHLO gmail.com");
  if (!response())
    return 0;
  
  Serial.println(F("Отправка команды о начале авторизации"));
  wiFiClient.println("auth login");
  if (!response())
    return 0;

  Serial.println(F("Отправка логина"));
  wiFiClient.println(base64::encode(MAIL_FROM));
  if (!response())
    return 0;

  Serial.println(F("Отправка пароля"));
  wiFiClient.println(base64::encode(MAIL_PASSWORD));
  if (!response())
    return 0;

  Serial.println(F("Отправка адреса почты автора письма"));
  String mailFrom = String("MAIL FROM: <" + MAIL_FROM + ">");
  wiFiClient.println(mailFrom);
  if (!response())
    return 0;

  Serial.println(F("Sending To"));
  String mailRcptTo = String("RCPT To: <" + MAIL_TO + ">");
  wiFiClient.println(mailRcptTo);  // Повторить для каждого получателя 
  if (!response())
    return 0;

  Serial.println(F("Отправка команды о начале передачи тела письма"));
  wiFiClient.println(F("DATA"));
  if (!response())
    return 0;


  Serial.println(F("Sending email"));
  // recipient address (include option display name if you want)
  String mailTo = String("To: <" + MAIL_TO + ">");
  wiFiClient.println(F(mailTo);
  // "Home Alone Group" - название группы получателей. Будет отображаться в строке получателей.
  // wiFiClient.println(F("To: Home Alone Group<totally@made.up>")); 
  
  wiFiClient.println(F(String("From: " + MAIL_FROM));
  wiFiClient.println(F("Subject: Your Arduino\r\n"));
  wiFiClient.println(text);
  // Каждую новую строку нужно отправлять отдельно.
  // wiFiClient.println(F("In the last hour there was: 8 activities detected. Please check all is well."));
  // wiFiClient.println(F("This email will NOT be repeated for this hour.\n"));
  // wiFiClient.println(F("This email was sent from an unmonitored email account - please do not reply."));
  // wiFiClient.println(F("Love and kisses from Dougle and Benny. They wrote this sketch."));

  // Для окончания отправки необходимо отправить отдельную точку.
  wiFiClient.println(F("."));
  if (!response())
    return 0;

  Serial.println(F("Завершение отправки"));
  wiFiClient.println(F("QUIT"));
  if (!response())
    return 0;

  wiFiClient.stop();
  Serial.println(F("Соединение с SMTP-сервером разорвано"));
  return 1;
}

// Check response from SMTP server
byte response()
{
  // Wait for a response for up to X seconds
  int loopCount = 0;
  while (!wiFiClient.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if (loopCount > 10000) {
      wiFiClient.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  // Take a snapshot of the response code
  byte respCode = wiFiClient.peek();
  while (wiFiClient.available())
  {
    Serial.write(wiFiClient.read());
  }

  if (respCode >= '4')
  {
    Serial.print("Failed in eRcv with response: ");
    Serial.print(respCode);
    return 0;
  }
  return 1;
}