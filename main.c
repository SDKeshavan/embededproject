#include "stm32f4xx.h"
#include <stdio.h>

// Global variables to store sensor data
uint8_t rain_data = 0xFF;
uint8_t dht11_data = 0xFF;
uint8_t soil_moisture_data =0xFF;

// Function prototypes
int USART_Init(void);
int GPIO_Init(void);
int TIM2_Init(void);
void sendData(char *data);
void readSensors(void);

// Global flag for the interrupt
volatile uint8_t dataReady = 0;
volatile uint8_t welcomeSent = 0;  // Flag to send welcome message once

int main(void) {
    // First, initialize USART for Bluetooth communication
    if (USART_Init() != 0) {
        // If USART initialization fails, we cannot communicate via Bluetooth
        while (1);  // Stop execution
    }

    // Proceed with other initializations and handle errors
    if (GPIO_Init() != 0) {
        sendData("Error: GPIO initialization failed\n");
        while (1);  // Stop execution
    }
    
    if (TIM2_Init() != 0) {
        sendData("Error: Timer initialization failed\n");
        while (1);  // Stop execution
    }

    // Main loop
    while (1) {
        if (!welcomeSent) {
            sendData("Welcome to Weather Station\n");
            welcomeSent = 1;  // Set flag to indicate welcome message is sent
        }

        if (dataReady) {
            dataReady = 0;  // Reset the flag
            readSensors();  // Read sensor data and send it over Bluetooth
        }
    }
}

// USART initialization (this must run first to enable Bluetooth output for errors)
int USART_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // Enable USART1 clock
    if (!(RCC->APB2ENR & RCC_APB2ENR_USART1EN)) {
        return -1;  // Return error if USART clock is not enabled
    }
    
    USART1->BRR = 0x683;  // Baud rate 9600 for 16 MHz clock
    USART1->CR1 |= USART_CR1_TE | USART_CR1_UE;  // Enable TX and USART
    
    if (!(USART1->CR1 & (USART_CR1_TE | USART_CR1_UE))) {
        return -2;  // Return error if USART configuration fails
    }

    return 0;  // Return success
}

// GPIO initialization
int GPIO_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;  // Enable clocks for GPIOA and GPIOB

    if (!(RCC->AHB1ENR & (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN))) {
        return -1;  // Return error if clock is not enabled
    }

    // Configure PA4 (DHT11) and PA0 (Soil moisture) as input
    GPIOA->MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER0);  // Input mode
    GPIOA->PUPDR |= (GPIO_PUPDR_PUPDR4_0 | GPIO_PUPDR_PUPDR0_0);  // Enable pull-up resistors

    // Configure PB0 (Rain sensor) as input
    GPIOB->MODER &= ~GPIO_MODER_MODER0;  // Input mode
    GPIOB->PUPDR |= GPIO_PUPDR_PUPDR0_0;  // Enable pull-up resistor

    // Configure PA9 (USART1 TX) as alternate function
    GPIOA->MODER |= GPIO_MODER_MODER9_1;  // Alternate function mode
    GPIOA->AFR[1] |= (0x7 << (1 * 4));  // AF7 for USART1 TX

    return 0;  // Return success
}

// Timer 2 initialization for interrupts
int TIM2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // Enable TIM2 clock
    if (!(RCC->APB1ENR & RCC_APB1ENR_TIM2EN)) {
        return -1;  // Return error if TIM2 clock is not enabled
    }
    
    TIM2->PSC = 16000 - 1;  // Prescaler for 1 ms tick (16 MHz / 16000)
    TIM2->ARR = 5000 - 1;  // 5-second interval
    TIM2->DIER |= TIM_DIER_UIE;  // Enable update interrupt
    TIM2->CR1 |= TIM_CR1_CEN;  // Enable counter

    NVIC_EnableIRQ(TIM2_IRQn);  // Enable TIM2 interrupt in NVIC

    return 0;  // Return success
}

// TIM2 Interrupt Service Routine
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;  // Clear update interrupt flag
        dataReady = 1;  // Set flag to read data
    }
}

// Function to send data over Bluetooth
void sendData(char *data) {
    while (*data) {
        while (!(USART1->SR & USART_SR_TXE));  // Wait until TX is empty
        USART1->DR = *data++;
    }
}

// Function to read data from sensors, with simulated error only for DHT11
void readSensors(void) {
    // Simulate an error condition for DHT11 sensor
    dht11_data = 0xFF;  // Set invalid data for DHT11 to simulate an error
    soil_moisture_data = GPIOA->IDR;
    rain_data = GPIOB->IDR;

    // Check specifically if DHT11 data is invalid
    if (dht11_data == 0xFF) {
        sendData("Error: DHT11 sensor reading failed\n");  // Send error message for DHT11
    } else {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "DHT: %d\n", dht11_data);
        sendData(buffer);  // Send data over Bluetooth for soil moisture and rain
    }
		
		  if (soil_moisture_data == 0xFF) {
        sendData("Error: Soil sensor reading failed\n");  // Send error message for DHT11
    } else {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Soil: %d\n", soil_moisture_data);
        sendData(buffer);  // Send data over Bluetooth for soil moisture and rain
    }
		
				  if (rain_data == 0xFF) {
        sendData("Error: Rain sensor reading failed\n");  // Send error message for DHT11
    } else {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Rain: %d\n", rain_data);
        sendData(buffer);  // Send data over Bluetooth for soil moisture and rain
    }
}