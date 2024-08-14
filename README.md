# MIL_Chamber

**Integrating IoT for Non-IoT Devices to Monitor and Control Data**

This repository provides a solution for integrating IoT capabilities into non-IoT devices, specifically for monitoring and controlling data such as working time, setpoint values, and current operational points in a MIL Chamber.

## Overview

The MIL Chamber is traditionally a non-IoT device used for various testing and environmental simulations. This project aims to enhance the functionality of the MIL Chamber by integrating IoT capabilities, allowing for real-time monitoring, data logging, and remote control.

### Key Features

- **Data Monitoring**: Continuously monitor important parameters such as working time, setpoint values, and current points within the MIL Chamber.
- **Setpoint Adjustment**: Remotely adjust the setpoint values to control the chamber's environment as needed.
- **Real-Time Alerts**: Receive notifications if any parameter goes beyond the desired threshold, ensuring quick responses to potential issues.
- **Data Logging**: Automatically record and store data over time for analysis and reporting.

## Getting Started

### Prerequisites

Ensure you have the following installed:
- ESP-IDF
- Python 3.x
- MQTT Broker (e.g., Mosquitto)
- ESP32 or other IoT-compatible hardware
- Necessary libraries: `PubSubClient.h`, `UART.h`, etc.

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/yourusername/MIL_Chamber.git
   cd MIL_Chamber
   ...
