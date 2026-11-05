# ESP32 Barograph

Автономный цифровой барограф / метеостанция на **ESP32-WROOM** (Arduino IDE): BMP280/BME280, история 72 ч (SD-карта в CSV или внутренняя flash), Web UI без интернета, REST API, NTP (Europe/Moscow), локальный прогноз погоды по тренду давления, резервная точка доступа.

Исходный код прошивки — в папке [`ESP32_Barograph`](ESP32_Barograph) (папку можно открывать в Arduino IDE напрямую). Подключение датчика и SD, установка, первый запуск, описание API и хранения данных — в [`ESP32_Barograph/README.md`](ESP32_Barograph/README.md).

Библиотеки: **Adafruit BME280 Library**, **Adafruit BMP280 Library**, **Adafruit Unified Sensor**. Плата: ESP32 Dev Module, разметка «Default 4MB with spiffs».

Приложение для ПК (Windows/Ubuntu: синхронизация, графики, экспорт в Excel/SQLite/CSV): [Esp32-barograph-pc](https://github.com/mikenilsonforeva-ux/Esp32-barograph-pc).
