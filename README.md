# The Box 3.3

## Устройство для контроля температуры

### Настройка ПО
1. Установить "Arduino IDE". 
2. *File -> Preferences -> Settings -> Additional Boards Manager URLs*: вставить http://arduino.esp8266.com/stable/package_esp8266com_index.json
3. *Tools -> Board*: выбрать "LOLIN(WEMOS) D1 R2 & mini"
4. Поставить две библиотеки
    - **base64** by Densaugeo
    - **DHT sensor library for ESPx** by beegee_tokyo
5. Создать файл *settings.h* и заполнить константы значениями.

Для просмотра последовательного порта на Linux необходимо предоставить разрешение с помощью команды  
`sudo chown ИМЯ_ПОЛЬЗОВАТЕЛЯ НАЗВАНИЕ_ПОРТА`  
Например, *sudo chown pyotr /dev/ttyUSB0*