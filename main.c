#include "stm32f4xx.h"
#include <stdio.h>

// Global variables to store sensor data
uint16_t rain_data = 0xFFFF;
uint16_t soil_moisture_data = 0xFFFF;
uint8_t dht11_data = 0xFF;

// Function prototypes
int USART_Init(void);
int GPIO_Init(void);
int ADC_Init(void);
int TIM2_Init(void);
void sendData(char *data);
void readSensors(void);

// Global flag for the interrupt
volatile uint8_t dataReady = 0;
volatile uint8_t welcomeSent = 0;  // Flag to send welcome message once

int main(void) {
    // Initialize USART for Bluetooth communication
    if (USART_Init() != 0) {
        while (1);  // Stop execution if USART initialization fails
    }

    // Initialize GPIO and handle errors
    if (GPIO_Init() != 0) {
        sendData("Error: GPIO initialization failed\n");
        while (1);
    }

    // Initialize ADC for soil and rain sensors
    if (ADC_Init() != 0) {
        sendData("Error: ADC initialization failed\n");
        while (1);
    }

    // Initialize Timer 2
    if (TIM2_Init() != 0) {
        sendData("Error: Timer initialization failed\n");
        while (1);
    }

    // Main loop
    while (1) {
        if (!welcomeSent) {
            sendData("Welcome to Weather Station\n");
            welcomeSent = 1;
        }

        if (dataReady) {
            dataReady = 0;
            readSensors();  // Read sensor data and send over Bluetooth
        }
    }
}

// USART initialization
int USART_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // Enable USART1 clock

    USART1->BRR = 0x683;  // Baud rate 9600 for 16 MHz clock
    USART1->CR1 |= USART_CR1_TE | USART_CR1_UE;  // Enable TX and USART

    return 0;
}

// GPIO initialization
int GPIO_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;  // Enable clocks for GPIOA and GPIOB

    // Configure PA0 and PB0 as analog inputs for ADC
    GPIOA->MODER |= GPIO_MODER_MODER0;  // Set PA0 to analog mode
    GPIOB->MODER |= GPIO_MODER_MODER0;  // Set PB0 to analog mode

    // Configure PA9 (USART1 TX) as alternate function
    GPIOA->MODER |= GPIO_MODER_MODER9_1;  // Alternate function mode
    GPIOA->AFR[1] |= (0x7 << (1 * 4));  // AF7 for USART1 TX

    return 0;
}

// ADC initialization
int ADC_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;  // Enable ADC1 clock

    ADC1->SQR3 = 0;  // Configure regular sequence register for single conversion mode

    ADC1->CR2 |= ADC_CR2_ADON;  // Enable ADC

    return 0;
}

// Timer 2 initialization for interrupts
int TIM2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // Enable TIM2 clock
    
    TIM2->PSC = 16000 - 1;  // Prescaler for 1 ms tick (16 MHz / 16000)
    TIM2->ARR = 5000 - 1;  // 5-second interval
    TIM2->DIER |= TIM_DIER_UIE;  // Enable update interrupt
    TIM2->CR1 |= TIM_CR1_CEN;  // Enable counter

    NVIC_EnableIRQ(TIM2_IRQn);  // Enable TIM2 interrupt in NVIC

    return 0;
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

// Function to read ADC value from a given channel
uint16_t readADC(uint8_t channel) {
    ADC1->SQR3 = channel;  // Select the channel
    ADC1->CR2 |= ADC_CR2_SWSTART;  // Start the conversion
    while (!(ADC1->SR & ADC_SR_EOC));  // Wait for conversion to complete
    return ADC1->DR;  // Return the result
}

// Function to read data from sensors
void readSensors(void) {
    // Simulate an error condition for DHT11 sensor
    dht11_data = 0xFF;  // Set invalid data for DHT11 to simulate an error
    
    // Read ADC values
    soil_moisture_data = readADC(0);  // PA0 is ADC channel 0
    rain_data = readADC(8);  // PB0 is ADC channel 8 (on STM32F4 series)

    // Check if DHT11 data is invalid
    if (dht11_data == 0xFF) {
        sendData("Error: DHT11 sensor reading failed\n");
    } else {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "DHT: %d\n", dht11_data);
        sendData(buffer);
    }
    
    // Send ADC data for soil moisture and rain sensors
    char buffer[50];
		if(soil_moisture_data > 5000 | soil_moisture_data<0){
			sendData("Error: Soil Moisture sensor reading failed\n");
		}else if (soil_moisture_data > 3000){
			soil_moisture_data = 1;
			snprintf(buffer, sizeof(buffer), "Soil Moisture: %d\n", soil_moisture_data);
			sendData(buffer);
		}else{
			soil_moisture_data = 0;
			snprintf(buffer, sizeof(buffer), "Soil Moisture: %d\n", soil_moisture_data);
			sendData(buffer);
		}

    
		if(rain_data > 5000 | rain_data < 0){
			sendData("Error: Soil Moisture sensor reading failed\n");

		}else if(rain_data >3000){
			rain_data = 1;
			snprintf(buffer, sizeof(buffer), "Rain: %d\n", rain_data);
			sendData(buffer);
		}else{
			rain_data = 0;
			snprintf(buffer, sizeof(buffer), "Rain: %d\n", rain_data);
			sendData(buffer);
		}	

}
