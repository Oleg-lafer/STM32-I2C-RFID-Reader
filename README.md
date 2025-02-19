RFID Reader using STM32, USB, and I2C

Project Overview

This project implements an RFID reader system using an STM32 microcontroller, an RFID PN532 sensor, and USB communication with a PC terminal. The STM32 acts as an interface between the PC and the RFID sensor, handling commands over USB (CDC Virtual COM Port) and communicating with the sensor using I2C.


---

Features

✅ Reads RFID tags using PN532 via I2C
✅ Receives commands from a PC over USB (CDC Virtual COM Port)
✅ Transmits RFID tag UID back to the PC
✅ Uses FreeRTOS for real-time processing (optional)
✅ Includes UART debugging support


---

System Architecture

+------------+    USB (CDC)   +------------+    I2C    +------------+
|    PC      | <-----------> |   STM32     | <-------> | RFID Sensor |
| (Terminal) |               | (Firmware)  |          |  (PN532)    |
+------------+               +------------+          +------------+

Process Flow

1. PC sends a command (READ_RFID) over USB


2. STM32 receives the command and sends a read request to the PN532 RFID sensor over I2C


3. PN532 responds with the RFID tag UID


4. STM32 transmits the UID back to the PC via USB




---

Hardware Requirements

STM32 Microcontroller (e.g., STM32F103, STM32F4 series)

PN532 RFID Sensor (I2C mode)

USB connection to PC

UART (optional) for debugging



---

Software Requirements

STM32CubeIDE / Keil / PlatformIO for development

STM32 HAL (Hardware Abstraction Layer)

FreeRTOS (optional for multitasking)

A terminal program (e.g., PuTTY, RealTerm) to interact with the USB Virtual COM Port



---

Code Breakdown

1. USB Communication with PC

The PC sends a command via USB, which is handled by:

uint8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
    if (strncmp((char*)Buf, "READ_RFID", *Len) == 0) {
        read_rfid();  // Read RFID when command received
    } else {
        CDC_Transmit_FS((uint8_t*)"Unknown command\r\n", 17);
    }
    return USBD_OK;
}

📌 Functionality:

Listens for incoming USB data

If "READ_RFID" is received, it calls read_rfid()



---

2. I2C Communication with PN532

The STM32 communicates with the RFID sensor using I2C:

HAL_StatusTypeDef pn532_send_command(uint8_t *cmd, uint8_t len) {
    return HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, cmd, len, HAL_MAX_DELAY);
}

HAL_StatusTypeDef pn532_read_data(uint8_t *data, uint8_t len) {
    return HAL_I2C_Master_Receive(&hi2c1, I2C_ADDRESS, data, len, HAL_MAX_DELAY);
}

📌 Functionality:

pn532_send_command(): Sends commands to the RFID sensor

pn532_read_data(): Reads the RFID tag UID from the sensor



---

3. Reading RFID Tags and Sending UID to PC

void read_rfid() {
    uint8_t readCmd[] = {0x02, 0x01};  // Read command for PN532

    if (pn532_send_command(readCmd, sizeof(readCmd)) == HAL_OK) {
        if (pn532_read_data(uid, sizeof(uid)) == HAL_OK) {
            char uidStr[32];
            sprintf(uidStr, "RFID Tag ID: ");
            for (int i = 0; i < sizeof(uid); i++) {
                char temp[4];
                sprintf(temp, "%02X ", uid[i]);
                strcat(uidStr, temp);
            }
            strcat(uidStr, "\r\n");
            CDC_Transmit_FS((uint8_t*)uidStr, strlen(uidStr)); // Send to PC
        } else {
            CDC_Transmit_FS((uint8_t*)"Error reading RFID\r\n", 20);
        }
    } else {
        CDC_Transmit_FS((uint8_t*)"Error sending command\r\n", 23);
    }
}

📌 Functionality:

Sends a command to PN532 to read an RFID tag

Reads the UID from the sensor

Converts it into a hex string and sends it back to the PC via USB



---

How to Use

1. Connect the STM32 to the PC via USB

The STM32 will be recognized as a Virtual COM Port (USB CDC).


2. Open a Serial Terminal (e.g., PuTTY, RealTerm)

Set the Baud Rate: 115200

Select the correct COM port (check in Device Manager on Windows or ls /dev/tty* on Linux)


3. Send the "READ_RFID" Command

Type "READ_RFID" and press Enter

The STM32 will read the RFID tag UID and send it back


4. View the Output

If an RFID tag is detected, the PC terminal will display:

RFID Tag ID: A3 5F 77 89 10

If an error occurs, an error message will be displayed.

