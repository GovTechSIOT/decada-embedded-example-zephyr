<a href="https://govtechsiot.github.io/decada-embedded-example-zephyr/"><img src="https://codedocs.xyz/doxygen/doxygen.svg"/></a>

## Introduction
`decada-embedded-example-zephyr` is a functional real-time operating system example using [Zephyr](https://www.zephyrproject.org/) for embedded software developers to reference and build upon. This example showcases how one can publish sensor measure points to the government's IoT infrastructure via DECADA Cloud.

This repository is a lightweight RTOS variant of [decada-embedded-example-mbedos](https://github.com/GovTechSIOT/decada-embedded-example-mbedos).
For the most feature-rich example, please visit the example-mbedos repository.


## Development Team

* Lau Lee Hong (lau\_lee\_hong@tech.gov.sg)
* Lee Tze Han



## Requirements for Development
* Development Machine running either Windows, MacOS or Linux
* A Zephyr supported target (see https://docs.zephyrproject.org/2.5.0/boards/index.html)
  * Source code is tested using a STM32F767ZI
  * Most ARM targets running an ARM Cortex-M4 and ARM Cortex-M7 should work as well
  * Non-ARM targets with similar specifications should work as well
* ESP32 WiFi Module
* External MCU Programmer (*optional*; for devices that do not have an onboard programmer, like a STLINK-v3)



## Quick Start
 * Clone the repository onto local disk: 
    `git clone --recurse-submodules https://github.com/GovTechSIOT/decada-embedded-example-zephyr.git`
 * Set up the development environment (see [/docs/setup_guide.pdf](/docs/setup_guide.pdf)) 
 * Open project with VSCode-PlatformIO 
 * Edit Wifi Details and DECADA Credentials in `/src/user_config.h`
 * In the IDE under PlatformIO tab, perform the following sequence
   * Clean --> Build --> Upload



### Hardware Setup

This repository is tested on the STM32F767ZI DK with peripherals configured as:
* **ESP32**
  * Pins configurable in 
    * `/zephyr/boards/arm/manuca_dk_revb/manuca_dk_revb.dts` (`xbee-serial`)
    * `/zephyr/boards/shields/esp_32_xbee/esp_32_xbee.overlay` (`reset-gpios`)

Other hardware targets supported by Zephyr can utilize this repository out of the box if platformio.ini is configured properly. 
The following steps are only required for boards that are not supported by PIO/Zephyr.
* Adding your board's json in `/boards` for PlatformIO usage
* Adding your board's Devicetree in `/zephyr/boards` for Zephyr usage



## Variants
Besides Zephyr, we provide embedded source code example(s) to connect to DECADA Cloud using other RTOS(es) as well.
* [MbedOS](https://os.mbed.com/): `decada-embedded-example-mbedos` (https://github.com/GovTechSIOT/decada-embedded-example-mbedos)



## Documentation and References
* Singapore Government Tech Stack - Sensors & IoT: https://www.developer.tech.gov.sg/products/categories/sensor-platforms-and-internet-of-things/decada-iot-tech-stack/
* Technical Instruction Manual for GovTech's SIOT Starter Kit: https://docs.developer.tech.gov.sg/docs/decada-starter-kit-guide/
* GovTech's Custom STM32F767ZI Development Kit (MANUCA DK) User Manual: https://docs.developer.tech.gov.sg/docs/decada-starter-kit-guide/assets/intro/MANUCA_User_Manual_V1.pdf



## License and Contributions
The software is provided under the Apache-2.0 license (see https://docs.developer.tech.gov.sg/docs/decada-starter-kit-guide/#/terms-and-conditions). Contributions to this project repository are accepted under the same license. Please see [CONTRIBUTING.md](CONTRIBUTING.md) for more information.