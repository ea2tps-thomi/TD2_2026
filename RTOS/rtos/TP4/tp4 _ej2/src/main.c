#include "stm32f1xx.h"          /* Cabecera CMSIS del proyecto */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <stdio.h>

#define QUEUE_LENGTH    5
#define ITEM_SIZE       sizeof(uint16_t)

/* Manejadores de FreeRTOS */
SemaphoreHandle_t xAdcSemaphore = NULL;
QueueHandle_t xAdcQueue = NULL;

TaskHandle_t xTaskProductoraHandle = NULL;
TaskHandle_t xTaskConsumidoraHandle = NULL;

volatile uint16_t g_adc_raw_value = 0;

/* Prototipos */
void Hardware_Init(void);
void UART1_Init(void);
void UART1_SendString(const char *str);
void vTaskProductoraADC(void *pvParameters);
void vTaskConsumidoraUART(void *pvParameters);

int main(void)
{
    Hardware_Init();

    xAdcSemaphore = xSemaphoreCreateBinary();
    xAdcQueue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);

    if (xAdcSemaphore != NULL && xAdcQueue != NULL)
    {
        /* Tarea Productora: Lee el ADC cada 100 ms */
        xTaskCreate(vTaskProductoraADC, 
                    "Task_ADC_Producer", 
                    configMINIMAL_STACK_SIZE + 128, 
                    NULL, 
                    2, 
                    &xTaskProductoraHandle);

        /* Tarea Consumidora: Recibe el dato y lo manda por UART */
        xTaskCreate(vTaskConsumidoraUART, 
                    "Task_UART_Consumer", 
                    configMINIMAL_STACK_SIZE + 128, 
                    NULL, 
                    2, 
                    &xTaskConsumidoraHandle);

        vTaskStartScheduler();
    }

    while (1);
}

/* -------------------------------------------------------------------------- */
/* Tarea Productora (ADC)                                                     */
/* -------------------------------------------------------------------------- */
void vTaskProductoraADC(void *pvParameters)
{
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        /* Iniciar conversión por software */
        ADC1->CR2 |= ADC_CR2_SWSTART;

        /* Bloqueado esperando que la ISR otorgue el semáforo binario */
        if (xSemaphoreTake(xAdcSemaphore, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            uint16_t adc_val = g_adc_raw_value;
            xQueueSend(xAdcQueue, &adc_val, portMAX_DELAY);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Tarea Consumidora (UART1)                                                  */
/* -------------------------------------------------------------------------- */
void vTaskConsumidoraUART(void *pvParameters)
{
    (void) pvParameters;
    uint16_t received_val = 0;
    char buffer[64];

    for (;;)
    {
        /* Aguarda en estado Bloqueado hasta recibir un dato por la cola */
        if (xQueueReceive(xAdcQueue, &received_val, portMAX_DELAY) == pdTRUE)
        {
            snprintf(buffer, sizeof(buffer), "Valor de ADC0: %u\r\n", received_val);
            UART1_SendString(buffer);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* ISR del ADC                                                                */
/* -------------------------------------------------------------------------- */
void ADC1_2_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (ADC1->SR & ADC_SR_EOC)
    {
        /* Leer DR guarda la conversión y limpia automáticamente el flag EOC */
        g_adc_raw_value = (uint16_t)(ADC1->DR & 0xFFFF);

        xSemaphoreGiveFromISR(xAdcSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* -------------------------------------------------------------------------- */
/* Configuración de Periféricos e Inicialización Hardware                      */
/* -------------------------------------------------------------------------- */
void Hardware_Init(void)
{
    SystemCoreClockUpdate();

    /* 1. Habilitar reloj para GPIOA, USART1, ADC1 y AFIO */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN | 
                    RCC_APB2ENR_ADC1EN | RCC_APB2ENR_AFIOEN;

    /* 2. Configurar Divisor de Clock para ADC (PCLK2 / 6 = 12 MHz max a 72 MHz) */
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV6;

    /* 3. Configurar PA0 como Entrada Analógica (ADC1_IN0) */
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);

    /* 4. Inicializar UART1 */
    UART1_Init();

    /* 5. Configurar ADC1 */
    ADC1->CR1 |= ADC_CR1_EOCIE;              /* Habilitar interrupción EOC */
    ADC1->SQR3 = 0;                          /* Canal 0 como 1ra conversión */

    ADC1->CR2 |= ADC_CR2_ADON;               /* Encender periférico ADC */
    for (volatile int i = 0; i < 1000; i++);  /* Tiempo de estabilización */

    /* Calibración recomendada del ADC */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL);
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL);

    /* 6. Configurar Interrupción en NVIC */
    NVIC_SetPriority(ADC1_2_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(ADC1_2_IRQn);
}

void UART1_Init(void)
{
    /* Configurar PA9 (TX) como Salida Alternada Push-Pull 10MHz */
    GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOA->CRH |= (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_0);

    /* Configurar PA10 (RX) como Entrada flotante / Pull-up */
    GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOA->CRH |= GPIO_CRH_CNF10_0;

    /* Baudrate: 9600 bps @ 72 MHz (BRR = 0x1D4C) */
    USART1->BRR = 0x1D4C;

    /* Habilitar Transmisor, Receptor y periférico USART1 */
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART1_SendString(const char *str)
{
    while (*str) {
        while (!(USART1->SR & USART_SR_TXE)); /* Esperar puerto disponible */
        USART1->DR = (*str++ & 0xFF);
    }
}

/* -------------------------------------------------------------------------- */
/* Idle Hook de FreeRTOS                                                      */
/* -------------------------------------------------------------------------- */
void vApplicationIdleHook(void)
{
}
