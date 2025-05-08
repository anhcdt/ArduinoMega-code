
# LED Board Fault Tester using Arduino Mega

## 🧾 Description

This project presents a fault detection device for LED circuit boards, developed using the **Arduino Mega 2560** microcontroller. The system supports testing three different types of LED boards (1 LED, 2 LEDs, and 3 LEDs), with selectable modes via a 5-pin joystick.

Test results are displayed on a 2004 LCD screen with I2C communication. Fault data is stored in `.csv` format on an SD card, and the timestamp of each test is accurately recorded using the DS3231 real-time clock (RTC) module.

## 🛠️ Hardware Components

- 01 unit - Arduino Mega 2560  
- 01 unit - 2004 LCD Display (I2C interface)  
- 01 unit - DS3231 RTC Module  
- 01 unit - 5-pin Joystick (for mode selection)  
- 01 unit - 4-Channel Relay Module  
- 01 unit - HW125 SD Card Reader Module  
- 50 units - 74HC165 Shift Register (for input expansion) 
- 20 units - LR7843 Mosfet modules 
- 200 units - LED Board Fault Detection Module (interfaced via 74HC165)

## 🧰 Required Arduino Libraries

- `Wire.h` – I2C communication  
- `LiquidCrystal_PCF8574.h` – LCD control 
- `RoxMux.h` - 74HC165 operations
- `SD.h` – SD card read/write operations  
- `DS3231.h` – DS3231 RTC operations  
- `SPI.h` – SPI communication for SD card
