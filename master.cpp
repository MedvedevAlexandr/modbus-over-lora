#include <SPI.h>
#include <LoRa.h>

// Настройки LoRa (должны совпадать на Master и Slave!)
#define BAND 433E6   // Частота (433 МГц)
#define SF 7         // Spreading Factor
#define BW 125E3     // Bandwidth

// Адрес Slave-устройства
#define SLAVE_ID 0x01

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

  Serial.println("Modbus Master started");
}

void loop() {
  // Формируем Modbus-запрос (функция 0x03 - чтение Holding Registers)
  uint8_t request[8];
  request[0] = SLAVE_ID;       // Адрес Slave
  request[1] = 0x03;           // Код функции (Read Holding Registers)
  request[2] = 0x00;           // Старший байт адреса регистра
  request[3] = 0x00;           // Младший байт адреса (начало с 0)
  request[4] = 0x00;           // Старший байт количества регистров
  request[5] = 0x02;           // Младший байт (читаем 2 регистра)

  // CRC-16 Modbus
  uint16_t crc = calculateCRC(request, 6);
  request[6] = crc & 0xFF;      // Младший байт CRC
  request[7] = crc >> 8;        // Старший байт CRC

  // Отправка запроса
  LoRa.beginPacket();
  LoRa.write(request, 8);
  LoRa.endPacket();

  Serial.println("Modbus request sent");
  Serial.print("Raw request: ");
  for (int i = 0; i < 8; i++) {
    Serial.print(request[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Ожидание ответа (таймаут 1 секунда)
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
    if (LoRa.parsePacket() >= 9) {
      uint8_t response[9];
      for (int i = 0; i < 9; i++) {
        response[i] = LoRa.read();
      }

      Serial.print("Raw response: ");
      for (int i = 0; i < 9; i++) {
        Serial.print(response[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Проверка CRC
      uint16_t receivedCRC = (response[7] << 8) | response[6];
      uint16_t calculatedCRC = calculateCRC(response, 6);
      if (receivedCRC == calculatedCRC && response[0] == SLAVE_ID && response[1] == 0x03) {
        uint16_t reg1 = (response[3] << 8) | response[4];
        uint16_t reg2 = (response[5] << 8) | response[6];
        Serial.print("Received data: ");
        Serial.print(reg1);
        Serial.print(", ");
        Serial.println(reg2);
      } else {
        // Serial.println("Invalid response!");
      }
      break;
    }
  }

  delay(2000); // Задержка между опросами
}

// Функция расчёта CRC-16 Modbus
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