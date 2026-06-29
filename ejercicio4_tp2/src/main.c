//  Ejercicio Practico N.o 4 Transmisión y recepción de datos desde PC a BLuepill y viceversa
#include <stdint.h>
#include <stdbool.h>
#include <string.h>     // Incluido para usar strcmp()
#include "stm32f1xx.h"  // archivo de cabecera CMSIS

static volatile uint32_t g_tick         = 0;   // Contador de milisegundos
static volatile uint32_t g_captura_tick = 0;   // Tick al momento del flanco
static volatile bool     g_pulsador     = false;
static volatile bool     g_modo_adc     = false;

// --- NUEVO: Variables globales para el buffer de recepción RX ---
#define RX_BUFFER_SIZE 32
static volatile char g_rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t g_rx_index = 0;
static volatile bool g_comando_recibido = false;

void SysTick_Handler(void)
{
    g_tick++;
}

static void systick_init(void) 
{
    g_tick = 0; 
    SysTick_Config(72000000UL / 1000UL);          
}

//  EXTI0 – interrupcion por flanco descendente en PA0
void EXTI0_IRQHandler(void)
{
    // Deshabilitar IRQ mientras se hace el antirrebote en el main
    NVIC_DisableIRQ(EXTI0_IRQn);

    g_pulsador = true;
    g_captura_tick = g_tick; // Registramos el milisegundo exacto de la interrupción

    // Limpiar flag de pending en EXTI linea 0
    EXTI->PR |= EXTI_PR_PR0;
}

// --- NUEVO: ISR para manejo de interrupción de la USART1 ---
void USART1_IRQHandler(void)
{
    // Verificar si la interrupción fue causada por recepción (RXNE)
    if (USART1->SR & USART_SR_RXNE)
    {
        // Leer el registro de datos limpia automáticamente el flag RXNE
        char c = (char)(USART1->DR & 0xFF); 

        // Si el comando anterior no fue procesado en el main, ignoramos entradas nuevas
        if (!g_comando_recibido)
        {
            // Detectar fin de cadena enviado desde la PC (\n o \r)
            if (c == '\n' || c == '\r')
            {
                if (g_rx_index > 0) // Procesar solo si hay caracteres en el buffer
                {
                    g_rx_buffer[g_rx_index] = '\0'; // Terminación nula obligatoria
                    g_comando_recibido = true;      // Marcar comando listo para evaluar
                }
            }
            // Guardar carácter común si queda espacio libre en el buffer
            else if (g_rx_index < (RX_BUFFER_SIZE - 1))
            {
                g_rx_buffer[g_rx_index++] = c;
            }
        }
    }
}

//  Inicializacion de GPIOs
static void gpio_init(void)
{
    // Habilitar relojes: AFIO, GPIOA, GPIOB, GPIOC y USART 
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_USART1EN;
    
    // PB1 como salida push-pull 2 MHz utilizado para ON/OFF desde PC
    GPIOB->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1); 
    GPIOB->CRL |=  GPIO_CRL_MODE1_1;              
    GPIOB->ODR |= ~GPIO_ODR_ODR1;  // LED en PB1 apagado al inicio 

    // PA0 como entrada digital Pull-up interna utilizado desde BL
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0); 
    GPIOA->CRL |=  GPIO_CRL_CNF0_1;               // Input con Pull-up/down
    GPIOA->ODR |=  GPIO_ODR_ODR0;                 // Selecciona Pull-UP (escribiendo 1 a ODR)
    
    // Configuración de pines USART1 
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9); 
    GPIOA->CRH |= (GPIO_CRH_MODE9_1 | GPIO_CRH_MODE9_0 | GPIO_CRH_CNF9_1); // PA9 alternate function output PP 50MHz   
    GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10); 
    GPIOA->CRH |= GPIO_CRH_CNF10_0; // PA10 entrada flotante
}

static void rxtx_init(void){
    // Configuración de trama 8N1 a 9600 baudios
    USART1->CR1 &= ~USART_CR1_PCE; // sin paridad
    USART1->CR1 &= ~USART_CR1_M;   // 8 bits de datos
    USART1->CR2 &= ~USART_CR2_STOP; // 1 bit de stop
    USART1->BRR = 0x1D4C;          // 9600 baudios con clock de 72MHz
    
    USART1->CR1 |= USART_CR1_RE;   // habilitación de recepción
    USART1->CR1 |= USART_CR1_TE;   // habilitación de transmisión
    
    // --- NUEVO: Habilitar interrupción por registro de recepción lleno (RXNEIE) ---
    USART1->CR1 |= USART_CR1_RXNEIE; 
    
    USART1->CR1 |= USART_CR1_UE;   // habilitación de USART1

    // --- NUEVO: Habilitación y prioridad de USART1 en el controlador Cortex-M3 (NVIC) ---
    NVIC_SetPriority(USART1_IRQn, 0); // Prioridad alta para no perder bytes críticos de la PC
    NVIC_EnableIRQ(USART1_IRQn);
}

//  Inicializacion de EXTI0 (PA0, flanco descendente)
static void exti_init(void)
{
    AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI0_PA); // Mapear EXTI0 a PA0

    EXTI->FTSR |= EXTI_FTSR_TR0;   // Sensible a flanco descendente linea 0
    EXTI->IMR  |= EXTI_IMR_MR0;    // Desenmascarar linea 0

    // Prioridad y habilitacion en NVIC 
    NVIC_SetPriority(EXTI0_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

int main(void){

    gpio_init();
    exti_init();
    systick_init();
    rxtx_init();

    char *dato = "Pulsador presionado\r\n"; 
    char buffer[64] = "";
    
    // Copiamos la cadena fija al buffer una sola vez antes de entrar al bucle
    uint8_t i = 0;
    while (dato[i] != '\0' && i < (sizeof(buffer) - 1)) 
    {
        buffer[i] = dato[i];
        i++;
    }
    buffer[i] = '\0';

    while (1)
    {
        // 1. EVALUAR SI SE PRESIONÓ EL PULSADOR
        if (g_pulsador) 
        {
            g_pulsador = false; // Bajamos el flag inmediatamente

            // TRANSMISIÓN DE LA CADENA POR USART1
            uint8_t buffer_index = 0;
            while (buffer[buffer_index] != '\0')
            {
                while (!(USART1->SR & USART_SR_TXE)); 
                USART1->DR = buffer[buffer_index]; 
                buffer_index++;
            }
        }

        // --- NUEVO: 2. EVALUAR COMANDOS ENTRANTES DESDE LA PC ---
        if (g_comando_recibido)
        {
            // Comparar buffer con comandos esperados sin incluir saltos de línea
            if (strcmp((char*)g_rx_buffer, "ON") == 0)
            {
                GPIOB->ODR &= ~GPIO_ODR_ODR1; // Encender LED en PB1 (0 lógico enciende por pull-up)
            }
            else if (strcmp((char*)g_rx_buffer, "OFF") == 0)
            {
                GPIOB->ODR |= GPIO_ODR_ODR1;  // Apagar LED en PB1 (1 lógico apaga)
            }

            // Resetear el buffer de lectura para el siguiente mensaje
            g_rx_index = 0;
            g_comando_recibido = false;
        }

        // 3. MÁQUINA DE ESTADO PARA EL ANTIRREBOTE (Debounce no bloqueante)
        if ((NVIC->ISER[0] & (1UL << EXTI0_IRQn)) == 0)
        {
            if ((g_tick - g_captura_tick) >= 50) 
            {
                EXTI->PR |= EXTI_PR_PR0;
                NVIC_EnableIRQ(EXTI0_IRQn);
            }
        }
        
    } // cierre del bucle while(1)
}
