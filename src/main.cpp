// Copyright (C) 2024 Matías S. Ávalos (@tute_avalos)
//
// This file is part of esp8266-io-board-websocket.
//
// esp8266-io-board-websocket is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// esp8266-io-board-websocket is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with esp8266-io-board-websocket.  If not, see
// <https://www.gnu.org/licenses/>.

#include <AHT10.h>
#include <Arduino.h>
#include <BH1750.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
#include <LittleFS.h>
#include <PeriodicTaskManager.h>
#include <Wire.h>

// El baudrate debe modifcarse en el platformio.ini
#ifndef BAUD_RATE
#define BAUD_RATE 74880
#endif
#define LEN(X) (sizeof(X) / sizeof(X[0]))

/* Pines utilizados */
const int RGB[]{D7, D6, D5}; // R=D7, G=D6, B=D5
const int BTNS[]{D3, D4};    // BTN1=D3, BTN2=D4

// Semi-ciclo de un SENO con 128 posiciones
const uint8_t SINE_LUT[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05,
    0x05, 0x06, 0x07, 0x09, 0x0A, 0x0B, 0x0C, 0x0E, 0x0F, 0x11, 0x12, 0x14,
    0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 0x21, 0x23, 0x25, 0x28, 0x2A, 0x2C,
    0x2F, 0x31, 0x34, 0x36, 0x39, 0x3B, 0x3E, 0x41, 0x43, 0x46, 0x49, 0x4C,
    0x4F, 0x52, 0x55, 0x58, 0x5A, 0x5D, 0x61, 0x64, 0x67, 0x6A, 0x6D, 0x70,
    0x73, 0x76, 0x79, 0x7C, 0x80, 0x83, 0x86, 0x89, 0x8C, 0x8F, 0x92, 0x95,
    0x98, 0x9B, 0x9E, 0xA2, 0xA5, 0xA7, 0xAA, 0xAD, 0xB0, 0xB3, 0xB6, 0xB9,
    0xBC, 0xBE, 0xC1, 0xC4, 0xC6, 0xC9, 0xCB, 0xCE, 0xD0, 0xD3, 0xD5, 0xD7,
    0xDA, 0xDC, 0xDE, 0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEB, 0xED, 0xEE,
    0xF0, 0xF1, 0xF3, 0xF4, 0xF5, 0xF6, 0xF8, 0xF9, 0xFA, 0xFA, 0xFB, 0xFC,
    0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF};

/* Información para conectarse al WiFi (Modo AP) */
const char *SSID{"ESP8266 IO Board"};
const char *PSWD{"asdf1234"};
// Mensaje que se envía cuando llega un comando que no está en la lista
// ver onWebSocketEvent()
const char *BADREQ{"{\"error\": \"No es un comando válido.\"}"};

// Variables del servidor y websocket
AsyncWebServer server{80};
AsyncWebSocket ws{"/ws"};

/* Periféricos y uso interno: */
LiquidCrystal_I2C *lcd{nullptr};
const uint8_t LCD_ADDRSS[]{0x3F, 0x27}; // posibles direcciones para el LCD
volatile bool is_lcd_connected{false};
// Textos en el display (fila 1 y fila 2)
String lcdrows[2]{"", ""};

AHT10 *aht10{nullptr};
volatile bool is_aht_connected{false};
volatile float tmp{0};
volatile float hum{0};

BH1750 *bh1750{nullptr};
const uint8_t BH1750ADDR{0x23};
volatile bool is_bh_connected{false};
volatile float lx{0};

// Tareas periódicas:
PeriodicTaskManager pTasker;

// Último estado del botón registrado (presionado/no-presionado)[ON/OFF]
volatile bool last_btn_states[LEN(BTNS)]{};
// Valor del RGB en formato '#RRGGBB'
String rgb_value{};

// Valor del LDR en la placa
volatile uint16_t lrd_value = 0;

// Indica si el botón está presionado desde el cliente web
volatile bool is_webbtn_pressed[LEN(BTNS)]{};

/**
 * @brief Se utiliza para leer el estado de los BTNS
 *
 * Si desde el WebSocket se pulsó el "botón", la función
 * siempre devuelve 0 (presionado) para ese pin. Sino devuelve
 * el resultado de digitalRead
 *
 * @param pin pin del cual leer el estado del pin
 * @return int estado del pin (físico o virtual según corresponda)
 */
int readBtnFrom(uint8_t pin) {
  for (size_t i{0}; i < LEN(BTNS); i++) {
    if (pin == BTNS[i] && is_webbtn_pressed[i]) {
      return 0;
    }
  }
  return digitalRead(pin);
}

/**
 * @brief Lee el valor del LDR asociado al ADC
 *
 * @param id designado por el PeriodicTaskManager
 */
void readLDR(uint8_t id __unused) { lrd_value = analogRead(A0); }

/**
 * @brief Lectura de temperatura y humedad
 *
 * @param id designado por el PeriodicTaskManager
 */
void readAHT10(uint8_t id __unused) {
  if (is_aht_connected) {
    float t = aht10->readTemperature();
    float h = aht10->readHumidity();
    if (t != AHT10_ERROR) {
      tmp = t;
    }
    if (h != AHT10_ERROR) {
      hum = h;
    }
  }
}

/**
 * @brief Lectura del luxómetro
 *
 * @param id designado por el PeriodicTaskManager
 */
void readBH1750(uint8_t id __unused) {
  if (is_bh_connected) {
    if (bh1750->measurementReady()) {
      float l = bh1750->readLightLevel();
      if (l > 0) {
        lx = l;
      }
    }
  }
}

/**
 * @brief Verifica si hay un dispositivo conectado en el BUS I2C
 *
 * Envía un paquete a la dirección address, si el dispositivo
 * contesta, entonces devuelve un true, sino false.
 *
 * @param address dirección a verificar
 * @return true si se encuentra en la linea del I2C
 * @return false si no se encuentra en la linea del I2C
 */
bool isI2CDevicePresent(byte address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0); // Devuelve true si no hay error (dispositivo presente)
}

/**
 * @brief Verifica que el LCD esté conectado y lo inicializa
 *
 * Cuando encuentra que se conectó un nuevo lcd (o el mismo), lo inicializa
 * soporta un único LCD conectado a la vez.
 *
 * @param id no utilizado, pid que le otorga el PeriodicTaskManager
 */
void initLCD(uint8_t id __unused) {
  static uint8_t last_addr{0};
  uint8_t conn_addr{0};
  for (auto &addr : LCD_ADDRSS) {
    if (isI2CDevicePresent(addr)) {
      conn_addr = addr;
      break;
    }
  }
  is_lcd_connected = conn_addr != 0;

  if (last_addr != conn_addr && is_lcd_connected) {
    if (lcd != nullptr) { // no hay problema al hacer delete
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
      delete lcd;
#pragma GCC diagnostic pop
    }
    lcd = new LiquidCrystal_I2C{conn_addr, 16, 2};
    lcd->init();
    lcd->backlight();
    // Se escribe el último texto enviado:
    lcd->print(lcdrows[0]);
    lcd->setCursor(0, 1);
    lcd->print(lcdrows[1]);
  }
  last_addr = conn_addr;
}

/**
 * @brief Inicializa el sensor de temperatura y humedad i2c
 *
 * Cuando se conecta el AHT10 se inicializa y se detecta si
 * fue desconectado, desalocando la memoria. Aparece y desaparece
 * de la interfaz gráfica web según corresponde.
 *
 * @param id no se utiliza, es el id del proceso periódico.
 */
void initAHT10(uint8_t id __unused) {
  if (isI2CDevicePresent(AHT10_ADDRESS_0X38)) {
    if (aht10 == nullptr) {
      aht10 = new AHT10(AHT10_ADDRESS_0X38);
      is_aht_connected = aht10->begin();
      if (!is_aht_connected) {
        delete aht10;
        aht10 = nullptr; // Se vuelve a nullptr sino queda el valor anterior
      }
    }
  } else {
    is_aht_connected = false;
    if (aht10 != nullptr) {
      delete aht10;
      aht10 = nullptr; // Se vuelve a nullptr sino queda el valor anterior
    }
  }
}

/**
 * @brief Inicializa el sensor de luz i2c (luxómetro)
 *
 * Cuando se conecta el BH1750 se inicializa y se detecta si
 * fue desconectado, desalocando la memoria. Aparece y desaparece
 * de la interfaz gráfica web según corresponde.
 *
 * @param id no se utiliza, es el id del proceso periódico.
 */
void initBH1750(uint8_t id __unused) {
  if (isI2CDevicePresent(BH1750ADDR)) {
    if (bh1750 == nullptr) {
      bh1750 = new BH1750(BH1750ADDR);
      is_bh_connected = bh1750->begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
      if (!is_bh_connected) {
        delete bh1750;
        bh1750 = nullptr; // Se vuelve a nullptr sino queda el valor anterior
      }
    }
  } else {
    is_bh_connected = false;
    if (bh1750 != nullptr) {
      delete bh1750;
      bh1750 = nullptr; // Se vuelve a nullptr sino queda el valor anterior
    }
  }
}

/**
 * @brief Algoritmo con antirebote que lee los botones
 *
 * Lee los botones declarados en BTNS. Solo deja cargado el valor
 * del pulsador en last_btn_states (state).
 *
 * Si se presiona el primer boton y cualquier otro (al mismo tiempo),
 * se restaura la tarea del seno en el rgb.
 *
 * @param id asignado por el PeriodicTaskManager, no se utiliza.
 */
void readBtns(uint8_t id __unused) {
  static uint8_t reads_btn[LEN(BTNS)]{0};
  for (size_t i{0}; i < LEN(BTNS); i++) {
    reads_btn[i] <<= 1;
    reads_btn[i] |= readBtnFrom(BTNS[i]);

    if (reads_btn[i] == 0x00) {
      if (last_btn_states[i] != true) {
        if (i > 0 && last_btn_states[0]) {
          pTasker.unpause("rgb");
        }
        last_btn_states[i] = true;
      }
    } else if (reads_btn[i] == 0xFF) {
      if (last_btn_states[i] != false) {
        last_btn_states[i] = false;
      }
    }
  }
}

/**
 * @brief Intercala los colores del LED RGB con los valores de un seno
 *
 * Se utiliza una look up table (SINE_LUT) con medio seno para incrementar
 * y decrementar la intensidad del LED RGB.
 *
 * @param id asignado por el PerdiodicTaskManager, no se utiliza.
 */
void rgbSine(uint8_t id __unused) {
  static bool up_down = true;
  static int color_value[LEN(RGB)]{0}, nv{0};

  color_value[nv] = up_down ? color_value[nv] + 1 : color_value[nv] - 1;
  if (color_value[nv] >= 127) {
    up_down = false;
  } else if (color_value[nv] <= 0) {
    up_down = true;
    nv++;
    nv %= 3;
  }
  for (size_t c{0}; c < LEN(RGB); c++) {
    analogWrite(RGB[c], 255 - SINE_LUT[color_value[c]]);
  }
  rgb_value = "#";
  for (size_t c{0}; c < LEN(RGB); c++) {
    char tmp[3]{};
    sprintf(tmp, "%02X", SINE_LUT[color_value[c]]);
    rgb_value += tmp;
  }
}

/**
 * @brief Envía al cliente la información del estado del hardware
 *
 * @param cmd el comando completo recibido por el cliente
 * @param client el cliente que envió la solicitud
 */
void getDataCommand(String &cmd, AsyncWebSocketClient *client) {
  // verifico que el comando sea 'dat' y no 'data', u otra cosa inválida
  if (cmd == "dat") {
    String hardware_state{"{\"rgb\":\"" + rgb_value + "\""};
    for (size_t i{0}; i < LEN(BTNS); i++) {
      hardware_state += ",\"btn" + String(i + 1) + "\":" + last_btn_states[i];
    }
    hardware_state += ",\"ldr\":" + String(lrd_value);
    hardware_state +=
        ",\"lcd_connected\":" + String((is_lcd_connected) ? "true" : "false");
    if (is_lcd_connected) {
      hardware_state += ",\"lcd1row\":\"" + lcdrows[0] + "\"";
      hardware_state += ",\"lcd2row\":\"" + lcdrows[1] + "\"";
    }
    hardware_state +=
        ",\"aht_connected\":" + String((is_aht_connected) ? "true" : "false");
    if (is_aht_connected) {
      hardware_state += ",\"tmp\":" + String(tmp);
      hardware_state += ",\"hum\":" + String(hum);
    }
    hardware_state +=
        ",\"bh_connected\":" + String((is_bh_connected) ? "true" : "false");
    if (is_bh_connected) {
      hardware_state += ",\"lx\":" + String(lx);
    }
    hardware_state += '}';
    client->text(hardware_state);
  } else {
    client->text(BADREQ);
  }
}

/**
 * @brief Establece el estado del botón
 *
 * @param cmd el comando completo recibido por el cliente
 * @param client el cliente que envió la solicitud
 */
void setBtnCommand(String &cmd, AsyncWebSocketClient *client) {
  int btn{atoi(cmd.substring(3).c_str())};
  btn--;
  if (btn >= 0 && btn < static_cast<int>(LEN(BTNS))) {
    is_webbtn_pressed[btn] = !last_btn_states[btn];
  }
}

/**
 * @brief Establece el estado del LED RGB
 *
 * @param cmd el comando completo recibido por el cliente
 * @param client el cliente que envió el mensaje
 */
void setRGBCommand(String &cmd, AsyncWebSocketClient *client) {
  // Deja de actualizarse el led rgb con el seno
  pTasker.pause("rgb");
  // En cmd debería haber un 'rgb=#RRGGBB' donde RR,GG,BB son los valores
  // en hexadecimal del color.
  rgb_value = cmd.substring(4, 11); // Se extrae el valor RRGGBB en hexa
  analogWrite(RGB[0], 255 - strtol(cmd.substring(5, 7).c_str(), NULL, 16));
  analogWrite(RGB[1], 255 - strtol(cmd.substring(7, 9).c_str(), NULL, 16));
  analogWrite(RGB[2], 255 - strtol(cmd.substring(9, 11).c_str(), NULL, 16));
}

/**
 * @brief Escribe un texto que viene desde la página web
 *
 * El comando es lcd=?<texto> donde '?' es 0 ó 1 (fila 1 ó 2)
 * y <texto> es lo que se escribe en el display. Deben ser
 * 16 chars codificados en ASCII estándar.
 *
 * @param cmd el comando completo recibido por el cliente
 * @param client el cliente que envió el mensaje
 */
void setLCDCommand(String &cmd, AsyncWebSocketClient *client) {
  String text = cmd.substring(4);
  int row = text[0] - '0';
  lcdrows[row] = text.substring(1);
  if (is_lcd_connected) {
    lcd->setCursor(0, row);
    lcd->print(lcdrows[row]);
  }
}

/**
 * @brief Atiende los eventos del websocket desde los clientes
 *
 * Aquí, por una cuestión de orden y mantener todo en un solo archivo,
 * se encuentran los comandos y el arreglo a funciones que se ejecutan
 * cuando hay un comando válido.
 *
 * @param server servidor corriendo
 * @param client cliente que hace la petición
 * @param type tipo de petición (WS_EVT_...)
 * @param arg argumentos
 * @param data datos
 * @param len tamaño del mensaje
 */
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {

  // Comandos válidos y arreglo de punteros a función a cada uno de ellos
  static const char *CMDS[]{"dat", "btn", "rgb", "lcd"};
  static void (*command[])(String &, AsyncWebSocketClient *){
      getDataCommand, setBtnCommand, setRGBCommand, setLCDCommand};
  //---------------------------------------------------------------------

  if (type == WS_EVT_CONNECT) {
    Serial.println("Cliente conectado: " + client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Cliente desconectado: " + client->id());
  } else if (type == WS_EVT_DATA) {
    // Se almacena el dato en el string msj
    String msj{};
    for (size_t i{0}; i < len; i++) {
      msj += char(data[i]);
    }
    // Se chequea si es un comando válido (de 3 caracteres)
    // si se detecta un código válido, se ejecuta a través
    // del arreglo de funciones 'command'
    bool is_valid_command{false};
    for (size_t i{0}; i < LEN(CMDS); i++) {
      if (msj.substring(0, 3) == CMDS[i]) {
        command[i](msj, client);
        is_valid_command = true;
        break;
      }
      // si no fue un comando válido se envía un mensaje de error
      if (!is_valid_command) {
        client->text(BADREQ);
      }
    }
  }
}

void setup() {
  // Configuración de pines (botones como entrada)
  for (auto &pin : BTNS) {
    pinMode(pin, INPUT);
  }
  // Se fuerza el estado actual de los botones para escribir en el lcd
  for (auto &b : last_btn_states)
    b = true;

  // Monitor serie para debuguear errores
  Serial.begin(BAUD_RATE);

  // Si no se puede montar el filesystem se resetea el ESP
  if (!LittleFS.begin()) {
    Serial.println("Ocurrió un error al inicializar el sistema de archivos...");
    ESP.reset();
  }
  Serial.println("Sistema de archivos montado con éxito.");

  // Se inicializa el I2C
  Wire.begin();
  // Para verificar dónde está el LCD (0x3F//0x27) e inicializarlo
  initLCD(0);
  // Se inicializa el aht10 (temperatura y humedad)
  initAHT10(0);
  // Se inicializa el bh1750 (luxómetro)
  initBH1750(0);
  // Inicialización del WiFi, WebSocket y Servidor Web
  WiFi.softAP(SSID, PSWD);
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.serveStatic("/", LittleFS, "/", "max-age=600")
      .setDefaultFile("index.html");
  server.begin();

  /* Tareas a ejecutar periódicamente */
  // cada 1 segundo chequea si están los dispositivos en el I2C
  pTasker.add(initLCD, "lcd-init", 1000);
  pTasker.add(initAHT10, "aht-init", 1000);
  pTasker.add(initBH1750, "bh-init", 1000);
  // lectura de los botones cada 4ms.
  // 8 lecturas seguidas de un mismo estado da por sentado el estado
  // en la variable last_btn_states (state)
  pTasker.add(readBtns, "btns", 4);
  // Muestra un seno en el led (SINE_LUT) para cada color del alternado
  // los colores (primero el rojo, luego verde y luego azul en ciclo)
  pTasker.add(rgbSine, "rgb", 50);
  // Se lee el ADC cada 125 ms
  pTasker.add(readLDR, "ldr", 125);
  // Se lee temperatura y humedad cada 500 ms
  pTasker.add(readAHT10, "aht", 500);
  // Se lee el luxómetro cada 200 ms (una lectura en alta resolución tarda ~120ms)
  pTasker.add(readBH1750, "bh", 200);
}

void loop() {
  ws.cleanupClients();
  pTasker.refresh();
}
