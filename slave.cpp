#include <SPI.h>
#include <LoRa.h>

// Настройки LoRa (должны совпадать на Master и Slave!)
#define BAND 433E6   // Частота (433 МГц)
#define SF 7         // Spreading Factor
#define BW 125E3     // Bandwidth

// Адрес Slave-устройства
#define SLAVE_ID 0x01

// Modbus-регистры (данные)
uint16_t holdingRegs[4] = {0};

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Инициализация LoRa
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  // Настройка параметров LoRa
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(BW);

  // Инициализация регистров (пример)
  holdingRegs[0] = 1234;
  holdingRegs[1] = 5678;

  Serial.println("Modbus Slave started");
}

void loop() {
  // Ожидание запроса
  if (LoRa.parsePacket() >= 8) {
    uint8_t request[8];
    for (int i = 0; i < 8; i++) {
      request[i] = LoRa.read();
    }

    Serial.print("Raw request: ");
    for (int i = 0; i < 8; i++) {
      Serial.print(request[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Проверка CRC
    uint16_t receivedCRC = (request[7] << 8) | request[6];
    uint16_t calculatedCRC = calculateCRC(request, 6);
    if (receivedCRC == calculatedCRC && request[0] == SLAVE_ID && request[1] == 0x03) {
      // Обновление данных (пример)
      holdingRegs[0] = millis() % 1000;
      holdingRegs[1] = analogRead(A0);

      // Формируем ответ
      uint8_t response[9];
      response[0] = SLAVE_ID;      // Адрес Slave
      response[1] = 0x03;          // Код функции
      response[2] = 0x04;          // Количество байт данных (2 регистра * 2)
      response[3] = 0x01;          // Старший байт 1-го регистра
      response[4] = 0x02;          // Младший байт 1-го регистра
      response[5] = 0x03;          // Старший байт 2-го регистра
      response[6] = 0x04;          // Младший байт 2-го регистра

      // CRC-16 Modbus
      uint16_t crc = calculateCRC(response, 7);
      response[7] = crc & 0xFF;    // Младший байт CRC
      response[8] = crc >> 8;      // Старший байт CRC

      // Отправка ответа
      LoRa.beginPacket();
      LoRa.write(response, 9);
      LoRa.endPacket();

      Serial.println("Response sent");
      Serial.print("Raw packet: ");
      for (int i = 0; i < 9; i++) {
        Serial.print(response[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

// Функция расчёта CRC-16 Modbus (аналогичная Master)
uint16_t calculateCRC(uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}