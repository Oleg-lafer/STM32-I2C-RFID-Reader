// Include necessary header files
#include "main.h"          // Main project definitions
#include "cmsis_os.h"      // FreeRTOS support
#include "i2c.h"           // I2C peripheral support
#include "usart.h"         // UART support for debugging
#include "usb_device.h"    // USB CDC (Communication Device Class) support
#include "gpio.h"          // GPIO support

// Define I2C address for PN532 RFID sensor
#define I2C_ADDRESS 0x48    // Example I2C address for the PN532 RFID module

// Declare FreeRTOS task handle for RFID reading
osThreadId_t readRFIDTaskHandle;

// Buffer to store RFID UID (Unique Identifier)
uint8_t uid[7];

// Function to send a command to the PN532 RFID sensor over I2C
HAL_StatusTypeDef pn532_send_command(uint8_t *cmd, uint8_t len) {
    // Send I2C command using HAL library
    return HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, cmd, len, HAL_MAX_DELAY);
}

// Function to read data from the PN532 RFID sensor over I2C
HAL_StatusTypeDef pn532_read_data(uint8_t *data, uint8_t len) {
    // Receive data over I2C using HAL library
    return HAL_I2C_Master_Receive(&hi2c1, I2C_ADDRESS, data, len, HAL_MAX_DELAY);
}

// Function to read RFID data and send it to the PC via USB
void read_rfid() {
    uint8_t readCmd[] = {0x02, 0x01};  // Example read command for the PN532 sensor

    // Send the read command to the RFID sensor
    if (pn532_send_command(readCmd, sizeof(readCmd)) == HAL_OK) {
        // Read UID data from the RFID sensor
        if (pn532_read_data(uid, sizeof(uid)) == HAL_OK) {
            // Buffer to format the UID data as a string
            char uidStr[32];
            sprintf(uidStr, "RFID Tag ID: ");
            
            // Convert UID bytes to hex string
            for (int i = 0; i < sizeof(uid); i++) {
                char temp[4];
                sprintf(temp, "%02X ", uid[i]);
                strcat(uidStr, temp);
            }
            strcat(uidStr, "\r\n");

            // Send the UID string to the PC via USB
            CDC_Transmit_FS((uint8_t*)uidStr, strlen(uidStr));
        } else {
            // If reading RFID data fails, send an error message via USB
            char errorMsg[] = "Error reading RFID data\r\n";
            CDC_Transmit_FS((uint8_t*)errorMsg, strlen(errorMsg));
        }
    } else {
        // If sending the command fails, send an error message via USB
        char errorMsg[] = "Error sending command to RFID\r\n";
        CDC_Transmit_FS((uint8_t*)errorMsg, strlen(errorMsg));
    }
}

// FreeRTOS task to handle RFID reading in a loop (not used in this simplified version)
void StartReadRFID(void *argument) {
    while (1) {
        read_rfid();        // Read RFID data
        osDelay(500);       // Delay for 500ms before the next read
    }
}

// This function handles incoming data from the PC over USB (Virtual COM Port)
uint8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
    // Check if the command received is "READ_RFID"
    if (strncmp((char*)Buf, "READ_RFID", *Len) == 0) {
        read_rfid();  // If the command matches, call the RFID read function
    } else {
        // Send an error message if the command is unrecognized
        char errorMsg[] = "Unknown command received\r\n";
        CDC_Transmit_FS((uint8_t*)errorMsg, strlen(errorMsg));
    }
    return USBD_OK;  // Return success status
}

// Main function where the MCU setup begins
int main(void) {
    HAL_Init();                 // Initialize the Hardware Abstraction Layer (HAL)
    SystemClock_Config();       // Configure the system clock
    
    // Initialize all peripherals: GPIO, I2C, UART, USB
    MX_GPIO_Init();             // Initialize General Purpose Input/Output (GPIO)
    MX_I2C1_Init();             // Initialize I2C1 (for RFID communication)
    MX_USART2_UART_Init();      // Initialize UART2 (for debugging purposes)
    MX_USB_DEVICE_Init();       // Initialize USB as a Virtual COM Port

    // Initialize FreeRTOS kernel (optional in this project)
    osKernelInitialize();
    
    // Create a FreeRTOS task for continuous RFID reading (not mandatory here)
    readRFIDTaskHandle = osThreadNew(StartReadRFID, NULL, NULL);

    // Start the FreeRTOS scheduler
    osKernelStart();

    // Infinite loop in case FreeRTOS scheduler is not used
    while (1) {
        // MCU stays here if FreeRTOS is not running
    }
}

// I2C Initialization for communication with RFID sensor
void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;                  // Use I2C1 peripheral
    hi2c1.Init.ClockSpeed = 100000;         // Set clock speed to 100kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2; // Standard I2C duty cycle
    hi2c1.Init.OwnAddress1 = 0;             // Device address (not used here)
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; // 7-bit addressing mode
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; // Disable dual addressing
    hi2c1.Init.OwnAddress2 = 0;             // Secondary address (not used)
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; // Disable general call
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;     // Enable clock stretching

    // Check if I2C initialization was successful
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();  // Call error handler on failure
    }
}

// UART Initialization for debugging (optional)
void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;               // Use USART2 peripheral
    huart2.Init.BaudRate = 115200;          // Set baud rate for communication
    huart2.Init.WordLength = UART_WORDLENGTH_8B; // 8-bit word length
    huart2.Init.StopBits = UART_STOPBITS_1; // 1 stop bit
    huart2.Init.Parity = UART_PARITY_NONE;  // No parity bit
    huart2.Init.Mode = UART_MODE_TX_RX;     // Enable both transmit and receive
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE; // No hardware flow control
    huart2.Init.OverSampling = UART_OVERSAMPLING_16; // Standard oversampling

    // Check if UART initialization was successful
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();  // Call error handler on failure
    }
}

// Error handler function for unexpected situations
void Error_Handler(void) {
    __disable_irq();      // Disable all interrupts
    while (1) {
        // Stay in an infinite loop for debugging
    }
}
