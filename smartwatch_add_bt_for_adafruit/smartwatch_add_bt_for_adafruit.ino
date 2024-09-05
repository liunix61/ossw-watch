#include <Adafruit_SSD1327.h>

#include <bluefruit.h>
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>

/**
 * SCREEN SETUP
 */

// Used for software SPI
#define OLED_CLK 13
#define OLED_MOSI 11

// Used for software or hardware SPI
#define OLED_CS 10
#define OLED_DC 8

// Used for I2C or SPI
#define OLED_RESET -1

#define VBATPIN A6

#define BTN_1_PIN 10
#define BTN_2_PIN 9

int button1State = 0;
int button2State = 0;

// I2C
Adafruit_SSD1327 display(128, 128, &Wire, OLED_RESET, 1000000);

// OTA DFU service
BLEDfu bledfu;

// Uart over BLE service
BLEUart bleuart;

// Function prototypes for packetparser.cpp
uint8_t readPacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);

// Packet buffer
extern uint8_t packetbuffer[];

String tString;
String tString2;

/**
 * DECLARATIONS
 */

float startTime = 0;
String mode = "";
String timeString = "";

/**
 * SETUP METHOD
 */

void setup() {
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb
  
  Serial.println(F("Adafruit Bluefruit52 Controller App Example"));
  Serial.println(F("-------------------------------------------"));

  pinMode(BTN_1_PIN, INPUT);
  pinMode(BTN_2_PIN, INPUT);

  displaySetup();

  bluetoothSetup();

  startTime = millis() / 1000;
}

void displaySetup() {
  display.begin(0x3D); // i2c address for display
  display.clearDisplay();
  display.display();


  tString2 = "x";

  drawText(tString2, 0, 0, SSD1327_WHITE);

  drawText(tString2, 120, 0, SSD1327_WHITE);

  drawText(tString2, 120, 120, SSD1327_WHITE);
  
  drawText(tString2, 0, 120, SSD1327_WHITE);

  drawText("Cmode: " + mode, 30, 40, SSD1327_WHITE);

  tString = "Ready";
  drawText(tString, 30, 90, SSD1327_WHITE);
}

void bluetoothSetup() {
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();
}

String battPercent = "";
int timerBatt = 0;
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

/**
 * MAIN LOOP
 */

void loop() {
  
  // Wait for new data to arrive
  uint8_t len = readPacket(&bleuart, 10);
    // bluefruit app Buttons
  if (len > 0 && packetbuffer[1] == 'B') {
    char buttnum = packetbuffer[2]; // 49 50 51 52
    uint8_t pressed = packetbuffer[3]; // 48 49 i.e. 0 and 1 // uint8_t

    tString = buttnum;
    if (pressed == 49) {
      drawText(tString, 30, 20, SSD1327_WHITE);
    } else if (pressed == 48) {
      drawText(tString, 30, 20, SSD1327_BLACK);
    } 
  }
  /*
   * BUTTONS
   */

  button1State = digitalRead(BTN_1_PIN);
  button2State = digitalRead(BTN_2_PIN);
  
  if (button1State == HIGH) {
    btn1LastState = 1;
    drawText("btn 1", 50, 50, SSD1327_BLACK);
  } else {
    if (btn1LastState == 1) {
      // trigger btn a
      drawText("btn 1", 50, 50, SSD1327_WHITE);
    } else {
      drawText("btn 1", 50, 50, SSD1327_BLACK);
    }
    btn1LastState = 0;
  }

  if (button2State == HIGH) {
    btn2LastState = 1;
    drawText("btn 2", 50, 50, SSD1327_BLACK);
  } else {
    if (btn2LastState == 1) {
      // trigger btn b
      drawText("btn 2", 50, 50, SSD1327_WHITE);
    } else {
      drawText("btn 2", 50, 50, SSD1327_BLACK);
    }
    btn2LastState = 0;
  }
  

  /*
   * BATTERY
   */


  if (timerBatt == 200) {
    drawText("Batt: " + battPercent, 30, 0, SSD1327_BLACK);
    battPercent = getBattPercent(battPercent);
    drawText("Batt: " + battPercent, 30, 0, SSD1327_WHITE);
    timerBatt = 0;
  } else {
    timerBatt = timerBatt + 1;
  }

  /*
   * TIMER
   */
  if (timerClk % 20 == 0) {
    currTime = millis() / 1000 - startTime;

    // @todo replace only the digit(s) that have changed

    secOnes = currTime % 10;
    secTens = (currTime % 60) / 10;
    minOnes = (currTime / 60) % 10;
    minTens = ((currTime / 60) % 60) / 10;
    nextTime = String(minTens) + String(minOnes) + ":" + String(secTens) + String(secOnes);
    if (nextTime != lastTime) {
      drawText("Timer: " + lastTime, 20, 70, SSD1327_BLACK);
      lastTime = nextTime;
      drawText("Timer: " + lastTime, 20, 70, SSD1327_WHITE);
    }
  } else {
    timerClk = timerClk + 1;
  }

  delay(5);
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
  display.display();
}

void drawTextWrap(String text, int x, int y, int color) {
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setTextColor(color);
  display.setCursor(x,y);
  display.print(text);
  display.display();
}

void actionAPress(boolean pressed, String mode) {
  String xyz = "action A press";
  if (pressed) {
    drawText(xyz, 30, 60, SSD1327_WHITE);
  } else {
    // for now, only act on release

    if (mode == "Timer") {
        // timer start/stop/reset
    } else if (mode == "Compass") {
        // zero at current setting
    } else if (mode == "Notes") {
        // ???
    }

    drawText(xyz, 30, 60, SSD1327_BLACK);
  }
  //display.display();
}

void modePress(boolean pressed, String mode) {
  String xyz = "mode press";
  if (pressed) {
    drawText(xyz, 30, 60, SSD1327_WHITE);
  } else {
    drawText(xyz, 30, 60, SSD1327_BLACK);
  }
  //display.display();
}

String switchMode(String mode) {
    if (mode == "Timer") {
        return "Compass";
    } else if (mode == "Compass") {
        return "Notes";
    }

    return "Timer";
}

String getBattPercent(String prevPercent) {
  float measuredvbat;
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.6;  // Multiply by 3.6V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  measuredvbat = (measuredvbat - 3.7) / .005;

  return String(measuredvbat) + "%";
}


void startAdv(void) {
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}
