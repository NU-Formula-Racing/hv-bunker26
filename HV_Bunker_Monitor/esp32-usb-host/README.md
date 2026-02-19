# ESP32-S3 USB Host for BMS Communication

ESP32-S3 configured as USB CDC host to receive data from STM32F405 BMS.

## Hardware

- ESP32-S3 development board
- STM32F405 on BMS mainboard
- Cut USB cable with Dupont connectors

## Wiring

ESP32-S3 Dev Board → BMS Mainboard
- GPIO 20 (D+) via Green wire → STM32 PA12 (D+)
- GPIO 19 (D-) via White wire → STM32 PA11 (D-)
- GND via Black wire → GND
- Red wire (VBUS) NOT CONNECTED

## Features

- USB CDC host mode on GPIO 19/20
- Receives BMS data from STM32
- WiFi connectivity
- Google Sheets integration
- Automatic reconnection
