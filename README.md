# Flip Clock

A 7-segment flip clock display for the **LilyGO T-Display S3** (ESP32-S3). Shows the time in London (GMT/BST), synced via NTP, with a flip animation on each minute change.

![LilyGO T-Display S3](https://img.shields.io/badge/Hardware-LilyGO%20T--Display%20S3-blue)
![Arduino](https://img.shields.io/badge/IDE-Arduino-teal)
![License](https://img.shields.io/badge/License-MIT-green)

---

## What It Does

- Displays the current time in hours and minutes using large 7-segment style digits
- Automatically handles GMT/BST switching for the UK
- Syncs time from NTP on boot — no battery-backed RTC required
- Flip animation plays each time the minute changes
- Either button rotates the display 180°, so the device can be mounted either way up

---

## Hardware

- **LilyGO T-Display S3** — [LilyGO on AliExpress](https://www.aliexpress.com/item/1005004496543604.html) or the [LilyGO GitHub](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- USB-C cable for programming
- WiFi network (2.4GHz)

---

## Controls

| Button | Action |
|--------|--------|
| Button 14 (right) | Rotate display 180° |
| Button 0 (left) | Rotate display 180° |

Both buttons do the same thing — rotate the display. Useful if you mount the device upside down.

---

## Software Setup

### 1. Arduino IDE

Install [Arduino IDE 2.x](https://www.arduino.cc/en/software).

### 2. ESP32 Board Package

In Arduino IDE go to **File → Preferences** and add this URL to Additional Board Manager URLs:

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Then go to **Tools → Board → Boards Manager**, search for `esp32` and install version **2.0.14**.

> ⚠️ Version 2.0.14 specifically — newer versions have compatibility issues with the T-Display S3.

### 3. TFT_eSPI Library

Download the LilyGO version of TFT_eSPI (not the Library Manager version):

1. Go to [https://github.com/Xinyuan-LilyGO/T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)
2. Download the repo as a ZIP
3. Copy the `TFT_eSPI` folder into your Arduino libraries folder (`~/Documents/Arduino/libraries/`)
4. Delete any existing TFT_eSPI folder first if one exists

### 4. Board Settings

In Arduino IDE select:

- **Board**: ESP32S3 Dev Module
- **Upload Speed**: 921600
- **USB Mode**: Hardware CDC and OTG
- **USB CDC On Boot**: Enabled
- **Flash Mode**: QIO 80MHz
- **Flash Size**: 16MB (128Mb)
- **Partition Scheme**: 16M Flash (3MB APP/9.9MB FATFS)
- **PSRAM**: OPI PSRAM
- **Port**: whichever port appears when you plug in the device

---

## Configuration

Open the sketch and fill in your WiFi credentials at the top:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

That's all that needs changing. The timezone (London, GMT/BST) is hardcoded. If you want a different timezone, adjust the `isDST()` function and the UTC offset in `getLondonTime()`.

---

## Uploading

1. Plug in the T-Display S3 via USB-C
2. Hold the **BOOT** button on the device while clicking Upload in Arduino IDE
3. Release BOOT once the upload starts
4. The device will restart, connect to WiFi, sync NTP, then display the time

If the port doesn't appear, you may need the [CP210x driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).

---

## Troubleshooting

**Stuck on "Connecting..."**  
Check your WiFi credentials. The T-Display S3 only supports 2.4GHz networks.

**Stuck on "Syncing NTP..."**  
WiFi is connected but NTP isn't responding. This usually clears on its own — if it persists, check that your network allows outbound UDP on port 123.

**Time is wrong**  
NTP sync may have failed silently. Power cycle the device. If the time is consistently one hour out, BST/GMT may be switching on the wrong date — the DST calculation uses the last Sunday in March/October rule, which is correct for the UK.

**Display is upside down**  
Press either button to rotate 180°.

---

## How the Timezone Works

The sketch syncs to UTC via NTP then applies a manual offset for London time — 0 minutes in winter (GMT) and 60 minutes in summer (BST). The `isDST()` function determines which applies based on the current date, using the UK rule: clocks go forward on the last Sunday in March and back on the last Sunday in October.

This approach is more reliable on the ESP32 than using `setenv`/`tzset`, which can produce incorrect results when switching between timezones in quick succession.

---

## Serial Monitor

Connect at **115200 baud** to see startup output. Useful for diagnosing WiFi or NTP issues.

---

## Licence

MIT — do what you like with it.

# LIlyGoFlipClock
