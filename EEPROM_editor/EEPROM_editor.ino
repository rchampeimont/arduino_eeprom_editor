#include <PS2KeyAdvanced.h>
#include <PS2KeyMap.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// PS/2 keyboard connections
const int PS2_KEYBOARD_CLOCK_PIN = 2; // pin must support interrupts
const int PS2_KEYBOARD_DATA_PIN = 3; // digital input pin

// Connections to the circuit: LCD screen
const int LCD_RS_PIN = 12;
const int LCD_ENABLE_PIN = 11;
const int LCD_DATA_PIN_4 = 7;
const int LCD_DATA_PIN_5 = 8;
const int LCD_DATA_PIN_6 = 9;
const int LCD_DATA_PIN_7 = 10;

// Declare here how many LCD rows and cols the LCD screen has:
const int LCD_ROWS = 2;
const int LCD_COLS = 16;

// First EEPROM address displayed on the LCD screen
int displayedEEPROMOffset = 0;

// EEPROM address where the editor cursor is pointing
int cursorEEPROMOffset = 0;

PS2KeyAdvanced keyboard;
PS2KeyMap keymap;
LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_DATA_PIN_4, LCD_DATA_PIN_5, LCD_DATA_PIN_6, LCD_DATA_PIN_7);

void setup() {
  keyboard.begin(PS2_KEYBOARD_DATA_PIN, PS2_KEYBOARD_CLOCK_PIN);
  Serial.begin(9600);
  lcd.begin(LCD_COLS, LCD_ROWS);

  displayEEPROM();

  Serial.println("Ready");
}

void displayEEPROM() {
  lcd.clear();
  lcd.write("cur addr: ");
  lcd.print(cursorEEPROMOffset);

  lcd.setCursor(0, 1);
  for (int i = 0; i < LCD_COLS; i++) {
    lcd.write(EEPROM[i + displayedEEPROMOffset]);
  }

  // display cursor at the position were we are writing
  lcd.cursor();
  lcd.setCursor(cursorEEPROMOffset - displayedEEPROMOffset, 1);
}

void writeChar(char key) {
  Serial.write("Writing 0x");
  Serial.print(key, HEX);
  Serial.write(" at address ");
  Serial.println(cursorEEPROMOffset);
  EEPROM.update(cursorEEPROMOffset, key);
  displayEEPROM();
}

void insertChar(char key) {
  writeChar(key);
  moveCursorEEPROMOffset(1);
}

// Move the displayed window if necessary to include cursor
void maybeScroll() {
  while (cursorEEPROMOffset >= displayedEEPROMOffset + LCD_COLS) {
    displayedEEPROMOffset++;
  }
  while (cursorEEPROMOffset < displayedEEPROMOffset) {
    displayedEEPROMOffset--;
  }
}

// Move cursor relative to current position
void moveCursorEEPROMOffset(int delta) {
  cursorEEPROMOffset = (cursorEEPROMOffset + delta) % EEPROM.length();
  maybeScroll();
  displayEEPROM();
}

// Set cursor at absolute position
void setCursorEEPROMOffset(int newPosition) {
  cursorEEPROMOffset = newPosition % EEPROM.length();
  maybeScroll();
  displayEEPROM();
}

void loop() {
  unsigned int key = keyboard.read();
  if (key) {
    Serial.write("Key read: 0x");
    Serial.print(key, HEX);
    Serial.write(" (");
    Serial.write(key);
    Serial.println(")");
    if (key == 0x115) {
      // left arrow
      moveCursorEEPROMOffset(-1);
    } else if (key == 0x116) {
      // right arrow
      moveCursorEEPROMOffset(1);
    } else if (key == 0x113) {
      // page up
      moveCursorEEPROMOffset(-LCD_COLS);
    } else if (key == 0x114) {
      // page down
      moveCursorEEPROMOffset(LCD_COLS);
    } else if (key == 0x111) {
      // home key
      setCursorEEPROMOffset(0);
    } else if (key == 0x112) {
      // end key
      setCursorEEPROMOffset(EEPROM.length() - 1);
    } else if (key == 0x11C) {
      // backspace
      moveCursorEEPROMOffset(-1);
      writeChar(' ');
    } else if ((key & 0x8000) == 0 && keymap.remapKey(key)) {
      // printable character pressed
      insertChar(keymap.remapKey(key));
    }
  }
}
