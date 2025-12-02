# ESP32 NFC Inventory System

This project is an **ESP32-based inventory system** that uses **NFC/RFID tags** to track items and displays them via a **WebSocket-enabled web interface**.  
It also runs as a **WiFi access point with a captive portal**, so any device connecting to it can immediately access the inventory page.

---

## Features

- Reads **NFC text tags** using the **PN532** module over **SPI**.
- Stores an inventory in **ESP32 flash memory** using `Preferences`.
- Web interface to **view and manage inventory** in real-time via **WebSockets**.
- **Toggle item** functionality: scanning an existing item removes it.
- Captive portal mode: automatically redirects connected clients to the inventory page.
- Inventory is **persistent across power cycles**.

---

## Hardware Requirements

- **ESP32** development board
- **PN532 NFC module** (SPI mode recommended)
- **Wires / breadboard** for connections
- Optional: NFC tags/stickers (13.56 MHz)

---

## PN532 to ESP32 Wiring (SPI)

| ESP32 Pin | PN532 Pin |
|-----------|-----------|
| 23        | MOSI      |
| 19        | MISO      |
| 18        | SCK       |
| 5         | SS (SSEL) |

> Make sure the PN532 is configured to SPI mode (some modules have a jumper for this).

---

## Software Requirements

- Arduino IDE or PlatformIO
- ESP32 board support installed
- Libraries:
  - `WiFi.h`
  - `ESPAsyncWebServer`
  - `WebSocketsServer`
  - `Preferences.h`
  - `SPI.h`
  - `DNSServer.h`
  - `PN532`
  - `NfcAdapter.h`

---

## Installation

1. Install the required libraries via Arduino Library Manager.
2. Connect your ESP32 and PN532 according to the wiring table above.
3. Open the provided sketch (`main.ino`) in Arduino IDE.
4. Change the WiFi SSID and password if desired:
    ```cpp
    const char* ssid = "ᛒᚨᚷ'ᛟᚠ'ᚺᛟᛚᛞᛁX";
    const char* password = "12345678";
    ```
5. Upload the sketch to the ESP32.

---

## Usage

1. Power the ESP32. It will start as a WiFi access point.
2. Connect a device (phone, tablet, PC) to the ESP32 WiFi network.
3. Open a browser and go to `http://192.168.4.1/` (or any URL thanks to the captive portal).
4. Scan NFC tags:
   - If the tag text is **not in inventory**, it will be **added**.
   - If the tag text **already exists**, it will be **removed**.
5. The web page will **update live** using WebSockets.

---

## Web Interface

- Displays a list of items.
- Empty inventory shows: `"No items yet..."`.
- Real-time updates when items are added or removed.

---

## Code Highlights

- **Persistent Storage:** Uses `Preferences` to save items.
- **WebSocket Communication:** Broadcasts inventory updates to all clients.
- **Captive Portal:** Redirects any URL request to the main inventory page.
- **NFC Reading:** Only extracts **text records** from NDEF tags.

---

## License

MIT License — free to use and modify.

---

## Troubleshooting

- **NFC tag not read**: Make sure SPI wiring is correct and PN532 is in SPI mode.
- **Web page not loading**: Check that your device is connected to the ESP32 AP.
- **Items not saving**: Make sure `Preferences.begin()` and `Preferences.putString()` are correctly called in your code.
