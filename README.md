# Watchy Zephyr Application

A Zephyr RTOS application for the Watchy smartwatch platform, featuring LVGL GUI support, WiFi connectivity, and sensor integration.

**Tested on [Watchy](https://watchy.sqfmi.com/) device.**

## Features

- LVGL-based user interface with custom image support
- WiFi connectivity with credential management
- BMA4XX accelerometer support
- Button input handling
- Multiple app framework (counter app, image viewer)
- ESP32 hardware support

## Getting Started

### Prerequisites

Before you begin, ensure you have the Zephyr development environment set up. Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) to install required dependencies.

### Installation

1. **Initialize the workspace with west:**

```bash
west init -m https://github.com/letanphuc/watchy_zephyr_app --mr main watchy_zephyr_app
```

2. **Update Zephyr modules:**

```bash
cd watchy_zephyr_app
west update
```

3. **Apply Zephyr patch for Watchy display:**

```bash
cd zephyr
git apply ../app/patch/0001-Fix-for-display-of-watchy.patch
cd ..
```

This patch fixes:
- Display pixel format (MONO01 instead of MONO10)
- Reset pin logic for the SSD16xx e-paper display

4. **Install Python dependencies:**

```bash
pip install -r zephyr/scripts/requirements.txt
```

5. **Set up the environment:**

Source the Zephyr environment and activate your Python virtual environment:

```bash
source zephyr/zephyr-env.sh
source /path/to/your/.venv/bin/activate
```

### Building

Build the application for Watchy:

```bash
west build -b watchy/esp32/procpu app -- -DBOARD_ROOT=$PWD/app
```

**Clean build (if needed):**

```bash
west build -t clean
```

### Flashing and Monitoring

Flash the application and start the serial monitor:

```bash
west flash && west espressif monitor
```

## Project Structure

```
app/
├── CMakeLists.txt          # Main CMake configuration
├── prj.conf                # Project configuration
├── west.yml                # West manifest
├── boards/                 # Board-specific configurations
│   └── sqfmi/
├── cmake/                  # Custom CMake modules
│   └── lvgl_image.cmake    # LVGL image conversion
├── imgs/                   # Image assets
├── script/                 # Utility scripts
│   └── img_convert.py      # Image conversion tool
└── src/                    # Source code
    ├── main.c              # Application entry point
    ├── buttons.c/h         # Button handling
    ├── sensors.c           # Sensor integration
    └── app/                # Application modules
        ├── app_manager.c
        ├── counter/
        ├── images/
        └── gpio_event.c
```

## Configuration

Key configuration options in `prj.conf`:

- **WiFi**: Enabled with credentials storage in NVS
- **Display**: LVGL with 1-bit color depth for e-paper
- **Sensors**: BMA4XX accelerometer support
- **Networking**: IPv4, DHCP, DNS resolver
- **Logging**: Enabled with immediate mode

## Development

### Adding Images

Place PNG images in `src/app/images/res/`. The build system automatically converts them to LVGL-compatible format using the `lvgl_image.cmake` module.

### Creating New Apps

Follow the pattern in `src/app/counter/` to create new application modules. Register your app in `app_manager.c`.

## Resources

- [Zephyr Project Documentation](https://docs.zephyrproject.org/latest/)
- [Example Application Template](https://github.com/zephyrproject-rtos/example-application)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Watchy Hardware](https://watchy.sqfmi.com/)

## License

SPDX-License-Identifier: Apache-2.0
