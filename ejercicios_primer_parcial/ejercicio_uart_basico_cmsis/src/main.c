#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f1xx.h"  // Archivo de cabecera CMSIS

#define READY_SIGNAL 'R' // Carácter de control recibido 
#define BUFFER_SIZE 128  // Tamaño del buffer 

volatile char tx_buffer[BUFFER_SIZE];
volatile uint8_t tx_head = 0; 
volatile uint8_t tx_tail = 0; 

void USART1_Init(uint32_t baudrate){
    // 1. Habilitar reloj para Puerto A y USART1 (APB2)
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    // 2. Limpieza de los 4 bits de PA9 y PA10 mediante macros CMSIS
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9 | GPIO_CRH_MODE10 | GPIO_CRH_CNF10);

    // 3. Configuración de PA9 y PA10 usando macros de bits CMSIS (Sin números directos)
    // PA9  (TX): Output 50MHz (MODE=11) + AF Push-Pull (CNF=10)
    // PA10 (RX): Input (MODE=00) + Floating Input (CNF=01)
    GPIOA->CRH |= (GPIO_CRH_MODE9_1 | GPIO_CRH_MODE9_0 | GPIO_CRH_CNF9_1) | (GPIO_CRH_CNF10_0);

    // 4. Formato 8N1: 8 bits de datos, sin paridad, 1 bit de parada
    USART1->CR1 &= ~(USART_CR1_M | USART_CR1_PCE);
    USART1->CR2 &= ~USART_CR2_STOP;
    
    // 5. Configurar Baud Rate
    USART1->BRR = SystemCoreClock / baudrate;

    // 6. Habilitar TX, RX, interrupción por RXNE y habilitar el periférico USART
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    // 7. Habilitar interrupción en el NVIC
    NVIC_EnableIRQ(USART1_IRQn);
}

uint8_t USART1_RequestTransmit(const char *data, uint8_t len){
    for(uint8_t i = 0; i < len; i++){
        uint8_t next_head = (tx_head + 1) % BUFFER_SIZE;
        if(next_head == tx_tail){
            return 0; // Buffer lleno
        }
        tx_buffer[tx_head] = data[i];
        tx_head = next_head;
    }
    return 1;
}

void USART1_IRQHandler(void){
    // Recepción: Espera el carácter 'R'
    if(USART1->SR & USART_SR_RXNE){
        char rx_data = (char)(USART1->DR & 0xFF); 
        if(rx_data == READY_SIGNAL){
            if(tx_head != tx_tail){
                USART1->CR1 |= USART_CR1_TXEIE; // Inicia transmisión en 2do plano
            }
        }
    }

    // Transmisión: Ocurre en segundo plano al estar el registro de transmisión vacío
    if((USART1->SR & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)){
        if(tx_head != tx_tail){
            USART1->DR = tx_buffer[tx_tail];
            tx_tail = (tx_tail + 1) % BUFFER_SIZE;
        } else {
            USART1->CR1 &= ~USART_CR1_TXEIE; // Deshabilita TXEIE cuando finaliza el buffer
        }
    }
}

int main(void) {
    SystemInit(); 
    USART1_Init(9600);

    // Definido como arreglo de caracteres (sizeof calcula correctamente la longitud)
    const char dato_prueba[] = "hola Mundo 9600 8N1!\r\n";
    USART1_RequestTransmit(dato_prueba, sizeof(dato_prueba) - 1);

    while(1) {
        __WFI(); // CPU entra en modo de bajo consumo hasta recibir la siguiente interrupción
    }
}