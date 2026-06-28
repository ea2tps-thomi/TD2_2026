
//   Ejercicio Practico N.o 3 – Migracion a CMSIS
#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"  // archivo de cabecera necesario para incluir funciones de CMSIS.

static volatile uint32_t g_tick         = 0;   // Contador de milisegundos
static volatile uint32_t g_captura_tick = 0;   // Tick al momento del flanco
static volatile bool     g_pulsador     = false;
static volatile bool     g_modo_adc     = false;

void SysTick_Handler(void)
{
    g_tick++;
}

static void systick_init(void) 
{
    g_tick = 0; 
    SysTick->LOAD = (8000000UL / 1000UL) - 1UL;   // 7999 → tick cada 1 ms 
    SysTick->VAL  = 0;                             // Limpiar valor actual  
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk     // Reloj AHB (no /8)     
                  | SysTick_CTRL_TICKINT_Msk        // Habilitar IRQ          
                  | SysTick_CTRL_ENABLE_Msk;        // Arrancar contador    
}

//   EXTI0 – interrupcion por flanco descendente en PA0
void EXTI0_IRQHandler(void)
{
    // Deshabilitar IRQ mientras se hace el antirrebote
    NVIC_DisableIRQ(EXTI0_IRQn);

    g_pulsador    = true;
    g_captura_tick = g_tick;

    // Limpiar flag de pending en EXTI linea 0
    EXTI->PR |= EXTI_PR_PR0;
}

//   ADC – lectura de un canal por software (polling)
static uint16_t adc_leer_canal(uint8_t canal) 
{
    ADC1->CR2  |= ADC_CR2_ADON;        // encender / arrancar conversion 
    ADC1->SQR3  = canal;               // seleccionar canal              

    // pausa de estabilizacion 
    for (volatile int i = 0; i < 50; i++);

    ADC1->CR2 |= ADC_CR2_SWSTART;      // disparo por software  

    // esperar EOC con timeout para no colgar el SysTick 
    volatile uint32_t timeout = 2000;
    while (!(ADC1->SR & ADC_SR_EOC)) {
        if (--timeout == 0) return 0;
    }

    return (uint16_t)(ADC1->DR & ADC_DR_DATA); // Filtro puro con máscara CMSIS
}

//   Inicializacion de GPIOs

static void gpio_init(void)
{
    // Habilitar relojes: AFIO, GPIOA, GPIOB, GPIOC, ADC1 
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN
                  | RCC_APB2ENR_IOPAEN
                  | RCC_APB2ENR_IOPBEN
                  | RCC_APB2ENR_IOPCEN
                  | RCC_APB2ENR_ADC1EN;

    // Liberar pines JTAG en PB3/PB4 (SWD-only) 
    AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG);
    AFIO->MAPR |=  AFIO_MAPR_SWJ_CFG_JTAGDISABLE; /* SWD habilitado, JTAG libre */

    //PC13: salida push-pull 2 MHz 
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;             // 2MHz Output: MODE=10, CNF=00
    GPIOC->ODR |=  GPIO_ODR_ODR13;                 // Apagado al inicio (nivel alto)

    // PB1, PB5, PB6, PB7: salidas push-pull 2 MHz
    GPIOB->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
    GPIOB->CRL |=  GPIO_CRL_MODE1_1;              // PB1

    GPIOB->CRL &= ~(GPIO_CRL_MODE5 | GPIO_CRL_CNF5);
    GPIOB->CRL |=  GPIO_CRL_MODE5_1;              // PB5

    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
    GPIOB->CRL |=  GPIO_CRL_MODE6_1;              // PB6

    GPIOB->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
    GPIOB->CRL |=  GPIO_CRL_MODE7_1;              // PB7

    // PB8: salida push-pull 2 MHz 
    GPIOB->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8);
    GPIOB->CRH |=  GPIO_CRH_MODE8_1;              // PB8

    // LEDs del Puerto B apagados al inicio 
    GPIOB->ODR &= ~(GPIO_ODR_ODR1 | GPIO_ODR_ODR5 | GPIO_ODR_ODR6 
                  | GPIO_ODR_ODR7 | GPIO_ODR_ODR8);

    // PA0: Entrada digital flotante / Pull-up interna
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    GPIOA->CRL |=  GPIO_CRL_CNF0_1;               // Configura Input con Pull-up/down
    GPIOA->ODR |=  GPIO_ODR_ODR0;                 // Selecciona Pull-UP (escribiendo 1 a ODR)

    // PA1: entrada analogica (ADC Canal 1) 
    GPIOA->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1); // Modo Analógico
}

//   Inicializacion de EXTI0 (PA0, flanco descendente)
static void exti_init(void)
{
    // Mapear EXTI0 a PA0 (Limpiar los bits 3:0 del registro EXTICR1 pone a PA0 por defecto) 
    AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI0);

    EXTI->FTSR |= EXTI_FTSR_TR0;   // Sensible a flanco descendente linea 0
    EXTI->IMR  |= EXTI_IMR_MR0;     // Desenmascarar linea 0

    // Prioridad y habilitacion en NVIC 
    NVIC_SetPriority(EXTI0_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

//   Inicializacion del ADC1
static void adc_init(void)
{
    // Habilitar sensor de temperatura interno y Vrefint 
    ADC1->CR2 |= ADC_CR2_TSVREFE;

    // Tiempo de muestreo canal 16 (sensor temp): 239.5 ciclos 
    ADC1->SMPR1 |= ADC_SMPR1_SMP16;

    // Encender ADC por primera vez 
    ADC1->CR2 |= ADC_CR2_ADON;

    // Retardo de estabilizacion (tSTAB >= 1 us) 
    for (volatile int i = 0; i < 2000; i++);

    // Segunda escritura de ADON arranca la primera conversion 
    ADC1->CR2 |= ADC_CR2_ADON;
}

void main(void)
{
    gpio_init();
    exti_init();
    adc_init();
    systick_init();

    uint32_t ultimo_pc13 = 0;
    const uint32_t delay_pc13 = 500; // parpadeo a 1 Hz 

    while (1)
    {
        // Parpadeo LED PC13 cada 500 ms 
        if ((g_tick - ultimo_pc13) >= delay_pc13) {
            ultimo_pc13 = g_tick;
            GPIOC->ODR ^= GPIO_ODR_ODR13; // Cambia estado usando macro CMSIS del pin 13
        }

        //Antirrebote del pulsador 
        if (g_pulsador) {
            if ((g_tick - g_captura_tick) >= 20) {
                // confirmar nivel bajo en el registro de entrada IDR del pin 0 
                if (!(GPIOA->IDR & GPIO_IDR_IDR0)) {
                    g_modo_adc = !g_modo_adc;
                }
                g_pulsador = false;
                NVIC_EnableIRQ(EXTI0_IRQn);   // CMSIS: rehabilitar IRQ 
            }
        }

        // Apagar todos los LEDs de PB antes de actualizar 
        GPIOB->ODR &= ~(GPIO_ODR_ODR1 | GPIO_ODR_ODR5 | GPIO_ODR_ODR6 
                      | GPIO_ODR_ODR7 | GPIO_ODR_ODR8);

        if (g_modo_adc)
        {
            // Modo sensor de temperatura interno (canal 16) 
            uint16_t raw        = adc_leer_canal(16);
            uint32_t mv         = ((uint32_t)raw * 3300UL) / 4095UL;

            // Formula del Reference Manual (enteros, sin float) 
            int32_t temp;
            if (1430 >= (int32_t)mv)
                temp = (int32_t)(((1430 - (int32_t)mv) * 10) / 43) + 25;
            else
                temp = 25 - (int32_t)((((int32_t)mv - 1430) * 10) / 43);

            // Barra de LEDs usando registros ODR puros de CMSIS 
            if (temp > 20) GPIOB->ODR |= GPIO_ODR_ODR1;
            if (temp > 24) GPIOB->ODR |= GPIO_ODR_ODR5;
            if (temp > 28) GPIOB->ODR |= GPIO_ODR_ODR6;
            if (temp > 32) GPIOB->ODR |= GPIO_ODR_ODR7;
            if (temp > 36) GPIOB->ODR |= GPIO_ODR_ODR8;
        }
        else
        {
            // Modo ADC externo (canal 1, PA1)
            uint16_t val = adc_leer_canal(1);

            if (val >  100) GPIOB->ODR |= GPIO_ODR_ODR1;
            if (val > 1000) GPIOB->ODR |= GPIO_ODR_ODR5;
            if (val > 2000) GPIOB->ODR |= GPIO_ODR_ODR6;
            if (val > 3000) GPIOB->ODR |= GPIO_ODR_ODR7;
            if (val > 4000) GPIOB->ODR |= GPIO_ODR_ODR8;
        }
    }
}
