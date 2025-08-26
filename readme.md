# ESP32 Dual Sketch System

This workspace contains two coordinated ESP32 sketches that work together to create a simple remote camera system over WiFi.

## Overview

- **sketch_aug23a**: Runs on the main ESP32 (controller). Hosts a web interface where users can trigger photo capture. When a photo is requested, it sends a request to the second ESP32 (camera) and displays the returned image.
- **sketch_aug23b**: Runs on the ESP32-CAM. Handles camera initialization and image capture. Hosts a web server with endpoints for status, readiness, and image capture. When the controller requests an image, it captures a photo and returns it as a JPEG.

## How It Works

1. **ESP32-CAM (sketch_aug23b)** connects to WiFi and starts a web server. It exposes `/capture` for image capture and `/status` for diagnostics.
2. **Main ESP32 (sketch_aug23a)** also connects to WiFi and starts a web server with a user-friendly web page. When the user clicks "Capture Photo," it requests `/capture` from the ESP32-CAM and displays the image.
3. The two ESP32s communicate over the local network using HTTP requests.

## Usage

1. Flash `sketch_aug23b` to your ESP32-CAM. Note its IP address from the serial monitor.
2. Update the `espc_ip` variable in `sketch_aug23a` with the ESP32-CAM's IP address.
3. Flash `sketch_aug23a` to your main ESP32.
4. Connect both devices to the same WiFi network.
5. Open the main ESP32's IP address in your browser. Use the web interface to capture and view photos from the ESP32-CAM.

## Features

- Simple web UI for photo capture
- Real-time image transfer between two ESP32 devices
- Status and readiness checks for camera health

## Folder Structure

- `sketch_aug23a/` — Main ESP32 controller sketch
- `sketch_aug23b/` — ESP32-CAM sketch

## Notes

- The `libraries/` folder is ignored and not part of this project.
- Make sure to update WiFi credentials and IP addresses as needed.

---

**Title for GitHub:** ESP32 Dual Sketch Camera System
**Description for GitHub:** Two ESP32 sketches for a remote camera system: one hosts a web UI, the other captures and serves images over WiFi.
