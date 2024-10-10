#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>

/*
    Bluetooth BLE Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// https://www.uuidgenerator.net/
#define SERVICE_UUID        "7d4909a8-8895-402b-b05c-cc2e2436d431"
#define CHARACTERISTIC_UUID "39bed6e8-86cb-453c-b986-1186c7f6fe02"

/**
 * SCREEN SETUP
 */

#define TFT_CS        10
#define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
//#define TFT_DC         8
#define TFT_DC         6
#define TFT_MOSI 11  // Data out
#define TFT_SCLK 13  // Clock out

#define VBATPIN A6

#define BTN_1_PIN 10
#define BTN_2_PIN 9

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

/**
 * CLASSES
 */

class MyCallbacks : public BLECharacteristicCallbacks {
  // @todo memory management: reduce, reuse, dispose after use
  void onWrite(BLECharacteristic *pCharacteristic) {
    // local variables destroyed
    String value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.println("*********");
      // @todo match pattern Tdddddd
      if (value.length() == 7 && value.substring(0, 1) == "T") {
        // String currentTime = value.substring(1, 3) + ":" + value.substring(3, 5) + ":" + value.substring(5);
        String currentTime = value.substring(1, 3) + value.substring(3, 5) + value.substring(5);
        Serial.print("New time: ");
        Serial.print(currentTime);

        // set a value that main loop can read
        pCharacteristic->setValue(currentTime);
        //clockTime = currentTime;
      } else if (value.substring(0, 1) == "B") {
        Serial.print("New button: ");
        for (int i = 1; i < value.length(); i++) {
          Serial.print(value[i]);
        }
      }
      Serial.println();
      Serial.println("*********");
    }
  }
};

/**
 * DECLARATIONS
 */

// @todo memory management: reduce, reuse, dispose after use
// @todo instead of multiple vars, use binary e.g. 00, 01, 10, 11
BLECharacteristic *pCharacteristic;
int button1State = 0;
int button2State = 0;
String tString;
String tString2;
float startTime = 0;
String mode = "";
String timeString = "";
int timerClk = 0;
int currTime = 0;
int secOnes = 0;
int secTens = 0;
int minOnes = 0;
int minTens = 0;
String lastTime = "";
String nextTime = "";
int btn1LastState = 0;
int btn2LastState = 0;
String clockTime = "00:00:00";
String lastClockTime = "";

/**
 * SETUP METHOD
 */

void setup() {
  Serial.begin(115200);
  while ( !Serial ) delay(10);
  
  Serial.println(F("Open Source Smart Watch App"));
  Serial.println(F("---------------------------"));

  pinMode(BTN_1_PIN, INPUT);
  pinMode(BTN_2_PIN, INPUT);

  displaySetup();

  bluetoothSetup();

  startTime = millis() / 1000;
}

void displaySetup() {
  display.init(240, 240); 

/*
// ssd1327
  display.begin(0x3D); // i2c address for display
  display.clearDisplay();
  display.display();
*/

display.fillScreen(ST77XX_BLACK);

  tString2 = "x";

  drawText(tString2, 0, 0, ST77XX_WHITE);

  drawText(tString2, 120, 0, ST77XX_WHITE);

  drawText(tString2, 120, 120, ST77XX_WHITE);
  
  drawText(tString2, 0, 120, ST77XX_WHITE);

  drawText("Cmode: " + mode, 30, 40, ST77XX_WHITE);

  tString = "Ready";
  drawText(tString, 30, 90, ST77XX_WHITE);
}

/**
 * MAIN LOOP
 */

void loop() {
  // Wait for new data to arrive
  // uint8_t len = readPacket(&bleuart, 10);

  // @todo new way to read bluetooth data
  // @todo read value that is modified by bluetooth write

  // bluefruit app Buttons
  /*
  if (len > 0 && packetbuffer[1] == 'B') {
    char buttnum = packetbuffer[2]; // 49 50 51 52
    uint8_t pressed = packetbuffer[3]; // 48 49 i.e. 0 and 1 // uint8_t

    tString = buttnum;
    if (pressed == 49) {
      drawText(tString, 30, 20, ST77XX_WHITE);
    } else if (pressed == 48) {
      drawText(tString, 30, 20, ST77XX_BLACK);
    } 
  }
  */

  /*
   * PHYSICAL BUTTONS
   */
  
  // @todo need debounce
  // @todo same code for both buttons
  /*
  button1State = digitalRead(BTN_1_PIN);
  if (button1State == HIGH) {
    // button 1 pressed, dont trigger yet
    btn1LastState = 1;
    drawText("btn 1", 50, 50, ST77XX_BLACK);
  } else {
    // button 1 released
    if (btn1LastState == 1) {
      // trigger button 1 action
      drawText("btn 1", 50, 50, ST77XX_WHITE);
    } else {
      drawText("btn 1", 50, 50, ST77XX_BLACK);
    }
    btn1LastState = 0;
  }

  button2State = digitalRead(BTN_2_PIN);
  if (button2State == HIGH) {
    // button 2 pressed, dont trigger yet
    btn2LastState = 1;
    drawText("btn 2", 50, 50, ST77XX_BLACK);
  } else {
    // button 2 released
    if (btn2LastState == 1) {
      // trigger button 2 action
      drawText("btn 2", 50, 50, ST77XX_WHITE);
    } else {
      drawText("btn 2", 50, 50, ST77XX_BLACK);
    }
    btn2LastState = 0;
  }
  */

  /*
   * TIMER AND CLOCK
   */
  if (timerClk % 20 == 0) {
    clockTime = readValue(pCharacteristic);
    if (lastClockTime != clockTime) {
      drawText("C Time: " + lastClockTime, 20, 50, ST77XX_BLACK);
      lastClockTime = clockTime;
      drawText("C Time: " + clockTime, 20, 50, ST77XX_WHITE);
    }

    currTime = millis() / 1000 - startTime;

    // @todo replace only the digit(s) that have changed

    secOnes = currTime % 10;
    secTens = (currTime % 60) / 10;
    minOnes = (currTime / 60) % 10;
    minTens = ((currTime / 60) % 60) / 10;
    nextTime = String(minTens) + String(minOnes) + ":" + String(secTens) + String(secOnes);
    if (nextTime != lastTime) {
      drawText("Timer: " + lastTime, 20, 70, ST77XX_BLACK);
      lastTime = nextTime;
      drawText("Timer: " + lastTime, 20, 70, ST77XX_WHITE);
    }
  } else {
    timerClk = timerClk + 1;
  }

  delay(10);
}

/**
 * SUPPORTING METHODS
 */

void drawText(String text, int x, int y, int color) {
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(color);
  display.setCursor(x,y);
  display.print(text);
  // display.display(); // ssd1327
}

/*
void actionAPress(boolean pressed, String mode) {
  String xyz = "action A press";
  if (pressed) {
    drawText(xyz, 30, 60, ST77XX_WHITE);
  } else {
    // for now, only act on release

    if (mode == "Timer") {
        // timer start/stop/reset
    } else if (mode == "Compass") {
        // zero at current setting
    } else if (mode == "Notes") {
        // ???
    }

    drawText(xyz, 30, 60, ST77XX_BLACK);
  }
  //display.display();
}
*/

/*
void modePress(boolean pressed, String mode) {
  // local variables destroyed
  String xyz = "mode press";
  if (pressed) {
    drawText(xyz, 30, 60, ST77XX_WHITE);
  } else {
    drawText(xyz, 30, 60, ST77XX_BLACK);
  }
  //display.display();
}
*/

// @todo memory management: modify value, dont make a new one
/*
String switchMode(String mode) {
    if (mode == "Timer") {
        return "Compass";
    } else if (mode == "Compass") {
        return "Notes";
    }

    return "Timer";
}
*/

void bluetoothSetup() {
  BLEDevice::init("MyESP32");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic =
    pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  // callback call 
  pCharacteristic->setCallbacks(new MyCallbacks());

  pCharacteristic->setValue("00:00:00");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

int readValue(BLECharacteristic *characteristic) {
  if (characteristic == NULL) {
    return -1;
  }
  std::string value = std::string((characteristic->getValue()).c_str());

  try{
    return stoi(value);
  } catch(...) {
    return -1;
  }
}
