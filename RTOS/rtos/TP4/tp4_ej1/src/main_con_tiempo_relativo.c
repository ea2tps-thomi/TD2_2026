#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* Variable global compartida para contar ciclos de CPU libres */
volatile uint32_t idle_counter = 0;

/* Prototipos de funciones */
void vTaskA_Blinky(void *pvParameters);
void vTaskB_Telemetry(void *pvParameters);
void UART1_Init(void);
void UART1_SendString(const char *str);

/*-----------------------------------------------------------*/

int main(void) {

    SystemCoreClockUpdate();

    // 1. Habilitar reloj para GPIOC, GPIOA y USART1
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    // 2. Configurar PC13 como salida push-pull a 10 MHz (LED integrado)
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0;

    // 3. Inicializar periférico UART1 para telemetría
    UART1_Init();

    // 4. Crear Tarea A (Alta prioridad = 2)
    BaseType_t resA = xTaskCreate(vTaskA_Blinky,
                                 "Blinky",
                                 256,
                                 NULL,
                                 2,
                                 NULL);

    // 5. Crear Tarea B (Prioridad menor = 1)
    BaseType_t resB = xTaskCreate(vTaskB_Telemetry,
                                 "Telemetry",
                                 256,
                                 NULL,
                                 1,
                                 NULL);

    if (resA != pdPASS || resB != pdPASS) {
        while(1); // Si falla la creación de alguna tarea, detener
    }

    // 6. Arrancar scheduler
    vTaskStartScheduler();

    // Nunca debería llegar aquí
    for (;;);
}

/*-----------------------------------------------------------*/

/**
 * Tarea A (Blinky): Parpadeo a 5 Hz (Período de 200 ms -> 100 ms ON / 100 ms OFF)
 */
void vTaskA_Blinky(void *pvParameters) {
    (void) pvParameters;

    while(1) {
        GPIOC->BSRR = GPIO_BSRR_BR13; // LED ON (Activo en bajo)
        vTaskDelay(pdMS_TO_TICKS(100));
        
        GPIOC->BSRR = GPIO_BSRR_BS13; // LED OFF
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * Tarea B (Telemetría por UART): Transmite el estado del sistema cada 2 segundos.
 */
void vTaskB_Telemetry(void *pvParameters) {
    (void) pvParameters;
    char buffer[64];

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(2000));

        snprintf(buffer, sizeof(buffer), "Sistema Operativo Ejecutando idle:%04lu\r\n", (unsigned long)idle_counter);
        UART1_SendString(buffer);
    }
}

/**
 * Tarea C (Idle Hook / Monitor de Sistema):
 * Se ejecuta automáticamente desde el Idle Task de FreeRTOS.
 */
void vApplicationIdleHook(void) {
    idle_counter++;
}

/*-----------------------------------------------------------*/
/* Funciones auxiliares para UART1 */

void UART1_Init(void) {
    // Configurar PA9 (TX) como Salida Alternada Push-Pull 10MHz
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOA->CRH |= (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_0);

    // Configurar PA10 (RX) como Entrada flotante / Pull-up
    GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOA->CRH |= GPIO_CRH_CNF10_0;

    // Baudrate: 9600 bps @ 72 MHz (USARTDIV = 72000000 / (16 * 9600) = 468.75)
    // DIV_Mantissa = 468 (0x1D4), DIV_Fraction = 0.75 * 16 = 12 (0x0C) -> BRR = 0x1D4C
    USART1->BRR = 0x1D4C;

    // Habilitar Transmisor, Receptor y periférico USART1
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART1_SendString(const char *str) {
    while (*str) {
        while (!(USART1->SR & USART_SR_TXE)); // Esperar puerto disponible
        USART1->DR = (*str++ & 0xFF);
    }
}
