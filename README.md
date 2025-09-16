# _SARP Module Using ESP32's board_

## Getting Started

This project uses the ESP-IDF framework to program the ESP32 microcontroller.

### Prerequisites

- ESP32 development board
- USB cable
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp-idf/get-started/index.html) installed on your system. currently using v.5.2.2 build.

### Building the Project

1. Open a terminal and navigate to the project directory:

2. Configure the project (optional):

  ```sh
  idf.py menuconfig
  ```

4. Build the firmware:

  ```sh
  idf.py build
  ```

### Flashing the Firmware

1. Connect your ESP32 board to your computer via USB.

2. Flash the firmware to the microcontroller:

  ```sh
  idf.py -p <PORT> flash
  ```

  Replace `<PORT>` with your serial port (e.g., `COM3` on Windows or `/dev/ttyUSB0` on Linux).

### Monitoring Output

To view the serial output from the ESP32:

```sh
idf.py -p <PORT> monitor
```

Press `Ctrl+]` to exit the monitor.

### Additional Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp-idf/index.html)
- [Troubleshooting Flashing Issues](https://docs.espressif.com/projects/esp-idf/en/latest/esp-idf/troubleshooting.html)
