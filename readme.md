# ESP32 Camera System

This workspace contains two coordinated ESP32 sketches that work together to create a simple remote camera system over WiFi.

## Overview

- **esp32**: Runs on the main ESP32 (controller). Hosts a web interface where users can trigger photo capture. When a photo is requested, it sends a request to the second ESP32 (camera) and displays the returned image.
- **esp32-cam**: Runs on the ESP32-CAM. Handles camera initialization and image capture. Hosts a web server with endpoints for status, readiness, and image capture. When the controller requests an image, it captures a photo and returns it as a JPEG.

## How It Works

1. **ESP32-CAM (esp32-cam)** connects to WiFi and starts a web server. It exposes `/capture` for image capture and `/status` for diagnostics.
2. **Main ESP32 (esp32)** also connects to WiFi and starts a web server with a user-friendly web page. When the user clicks "Capture Photo," it requests `/capture` from the ESP32-CAM and displays the image.
3. The two ESP32s communicate over the local network using HTTP requests.

## Usage

1. Flash `esp32-cam` to your ESP32-CAM. Note its IP address from the serial monitor.
2. Update the `espc_ip` variable in `esp32` with the ESP32-CAM's IP address.
3. Flash `esp32` to your main ESP32.
4. Connect both devices to the same WiFi network.
5. Open the main ESP32's IP address in your browser. Use the web interface to capture and view photos from the ESP32-CAM.

## Features

- Simple web UI for photo capture
- Real-time image transfer between two ESP32 devices
- Status and readiness checks for camera health

## Folder Structure

- `esp32/` — Main ESP32 controller sketch
- `esp32-cam/` — ESP32-CAM sketch

## Notes

- Make sure to update WiFi credentials and IP addresses as needed.

---

**Title for GitHub:** ESP32 Dual Sketch Camera System
**Description for GitHub:** Two ESP32 sketches for a remote camera system: one hosts a web UI, the other captures and serves images over WiFi.
