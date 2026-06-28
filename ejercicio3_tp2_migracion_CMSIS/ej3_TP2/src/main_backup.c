// ============================================================
//  Ejercicio Practico N.o 3 – Migracion a CMSIS
//  Porta el ej2-TP2 usando estructuras de stm32f10x.h
//  y reemplaza el retardo por bucle con SysTick de 1 ms
//
//  Placa  : BluePill (STM32F103C8T6)
//  Reloj  : HSI interno – 8 MHz
//  Autor  : Migracion desde ej2-TP2 (bare-metal puro)
// ============================================================

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"   // Estructuras CMSIS del fabricante
/*
// ============================================================
//  Definiciones de pines  (sin cambios respecto al TP2)
// ============================================================
#define PIN_LED_PC13   (1UL << 13)   // LED onboard BluePill (activo bajo)
#define PIN_LED_PB1    (1UL <<  1)
#define PIN_LED_PB5    (1UL <<  5)
#define PIN_LED_PB6    (1UL <<  6)
#define PIN_LED_PB7    (1UL <<  7)
#define PIN_LED_PB8    (1UL <<  8)
#define PIN_BTN_PA0    (1UL <<  0)   // Pulsador en PA0 (EXTI0)
*/
// ============================================================
//  Estado global compartido con las ISR
// ============================================================
static volatile uint32_t g_tick         = 0;   // Contador de milisegundos
static volatile uint32_t g_captura_tick = 0;   // Tick al momento del flanco
static volatile bool     g_pulsador     = false;
static volatile bool     g_modo_adc     = false;

// ============================================================
//  SysTick – base de tiempo de 1 ms
//
//  LOAD = (f_clk / 1000) – 1
//       = (8 000 000 / 1000) – 1  =  7999
//
//  Diferencia TP2 → TP3:
//    TP2:  SysTick_LOAD = 999  (usaba CLKSOURCE=0, reloj AHB/8 = 1 MHz)
//    TP3:  SysTick->LOAD = 7999 (CLKSOURCE=1, reloj AHB = 8 MHz)
//  Ambos dan exactamente 1 ms; aqui se usa el reloj del nucleo
//  para mayor precision segun recomendacion CMSIS.
// ============================================================
void SysTick_Handler(void)
{
    g_tick++;
}

static void systick_init(void) //static para alcance de archivo y no para alcance global.
{
    g_tick = 0; 

    // CMSIS define SysTick como puntero a SysTick_Type
    SysTick->LOAD = (8000000UL / 1000UL) - 1UL;   //7999 → tick cada 1 ms 
    SysTick->VAL  = 0;                              //limpiar valor actual  
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk     // reloj AHB (no /8)     
                  | SysTick_CTRL_TICKINT_Msk        // habilitar IRQ          
                  | SysTick_CTRL_ENABLE_Msk;        //arrancar contador    
}

// ============================================================
//  EXTI0 – interrupcion por flanco descendente en PA0
// ============================================================
void EXTI0_IRQHandler(void)
{
    // Deshabilitar IRQ mientras se hace el antirrebote
    NVIC_DisableIRQ(EXTI0_IRQn);

    g_pulsador    = true;
    g_captura_tick = g_tick;

    // Limpiar flag de pending en EXTI
    EXTI->PR |= (1UL << 0);
}

// ============================================================
//  ADC – lectura de un canal por software (polling)
//
//  Diferencia TP2 → TP3:
//    TP2: ADC_CR2 |= ADC_SWSTART   (macro a puntero raw)
//    TP3: ADC1->CR2 |= ADC_CR2_SWSTART  (campo de estructura CMSIS)
// ============================================================
static uint16_t adc_leer_canal(uint8_t canal)
{
    ADC1->CR2  |= ADC_CR2_ADON;        /* encender / arrancar conversion */
    ADC1->SQR3  = canal;               /* seleccionar canal              */

    /* pausa de estabilizacion */
    for (volatile int i = 0; i < 50; i++);

    ADC1->CR2 |= ADC_CR2_SWSTART;      /* disparo por software           */

    /* esperar EOC con timeout para no colgar el SysTick */
    volatile uint32_t timeout = 2000;
    while (!(ADC1->SR & ADC_SR_EOC)) {
        if (--timeout == 0) return 0;
    }

    return (uint16_t)(ADC1->DR & 0x0FFF);
}

// ============================================================
//  Inicializacion de GPIO
//
//  Diferencia TP2 → TP3:
//    TP2: GPIOC_CRH &= 0xFF0FFFFF;   (puntero a direccion hardcodeada)
//    TP3: GPIOC->CRH &= ~(0xFUL<<20); (campo de GPIO_TypeDef*)
// ============================================================
static void gpio_init(void)
{
    /* ── Habilitar relojes: AFIO, GPIOA, GPIOB, GPIOC, ADC1 ── */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN
                 |  RCC_APB2ENR_IOPAEN
                 |  RCC_APB2ENR_IOPBEN
                 |  RCC_APB2ENR_IOPCEN
                 |  RCC_APB2ENR_ADC1EN;

    /* ── Liberar pines JTAG en PB3/PB4 (SWD-only) ── */
    AFIO->MAPR &= ~(7UL << 24);
    AFIO->MAPR |=  (2UL << 24);   /* SWJ_CFG = 010 → SWD habilitado, JTAG libre */

    /* ── PC13: salida push-pull 2 MHz (LED onboard, activo bajo) ──
       CRH bits [23:20] → MODE13[1:0]=10, CNF13[1:0]=00             */
    GPIOC->CRH &= ~(0xFUL << 20);
    GPIOC->CRH |=  (0x2UL << 20);
    GPIOC->ODR |=  PIN_LED_PC13;   /* apagado al inicio (activo bajo) */

    /* ── PB1, PB5, PB6, PB7: salidas push-pull 2 MHz (CRL) ──
       CRL bits [7:4]=PB1, [23:20]=PB5, [27:24]=PB6, [31:28]=PB7   */
    GPIOB->CRL &= ~(0xFUL <<  4);   GPIOB->CRL |= (0x2UL <<  4);  /* PB1 */
    GPIOB->CRL &= ~(0xFUL << 20);   GPIOB->CRL |= (0x2UL << 20);  /* PB5 */
    GPIOB->CRL &= ~(0xFUL << 24);   GPIOB->CRL |= (0x2UL << 24);  /* PB6 */
    GPIOB->CRL &= ~(0xFUL << 28);   GPIOB->CRL |= (0x2UL << 28);  /* PB7 */

    /* ── PB8: salida push-pull 2 MHz (CRH) ──
       CRH bits [3:0]                                                */
    GPIOB->CRH &= ~(0xFUL << 0);
    GPIOB->CRH |=  (0x2UL << 0);   /* PB8 */

    /* LEDs PB apagados al inicio */
    GPIOB->ODR &= ~(PIN_LED_PB1 | PIN_LED_PB5 | PIN_LED_PB6
                  | PIN_LED_PB7 | PIN_LED_PB8);

    // PA0 como entrada con pull-up interna
      // CRL bits son MODE=00 CNF=10
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    GPIOA->CRL |=   GPIO_CRL_CNF0_1; // activo la pull-up interna
    GPIOA->ODR |= GPIO_ODR_ODR0; // activo la pull-up interna
    /* ── PA1: entrada analogica ──
       CRL bits [7:4] → MODE=00, CNF=00                             */
    GPIOA->CRL &= ~(0xFUL << 4);   /* todo ceros = entrada analogica */
}

// ============================================================
//  Inicializacion de EXTI0 (PA0, flanco descendente)
// ============================================================
static void exti_init(void)
{
    // Mapear EXTI0 a PA0 (valor 0000 en EXTICR1[3:0]) 
    AFIO->EXTICR[0] &= ~(0xFUL << 0);

    EXTI->FTSR |= (1UL << 0);   // sensible a flanco descendente 
    EXTI->IMR  |= (1UL << 0);   // desenmascarar linea 0

    // Prioridad y habilitacion en NVIC 
    NVIC_SetPriority(EXTI0_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

// ============================================================
//  Inicializacion del ADC1
// ============================================================
static void adc_init(void)
{
    /* Habilitar sensor de temperatura interno y Vrefint */
    ADC1->CR2 |= ADC_CR2_TSVREFE;

    /* Tiempo de muestreo canal 16 (sensor temp): 239.5 ciclos */
    ADC1->SMPR1 |= (7UL << 18);

    /* Encender ADC por primera vez */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* Retardo de estabilizacion (tSTAB >= 1 us) */
    for (volatile int i = 0; i < 2000; i++);

    /* Segunda escritura de ADON arranca la primera conversion */
    ADC1->CR2 |= ADC_CR2_ADON;
}

// ============================================================
//  Main
// ============================================================
void main(void)
{
    gpio_init();
    exti_init();
    adc_init();
    systick_init();

    uint32_t ultimo_pc13 = 0;
    const uint32_t delay_pc13 = 500;   /* parpadeo a 1 Hz */

    while (1)
    {
        /* ── Parpadeo LED PC13 cada 500 ms ── */
        if ((g_tick - ultimo_pc13) >= delay_pc13) {
            ultimo_pc13 = g_tick;
            GPIOC->ODR ^= PIN_LED_PC13;
        }

        /* ── Antirrebote del pulsador (20 ms) ── */
        if (g_pulsador) {
            if ((g_tick - g_captura_tick) >= 20) {
                /* confirmar nivel bajo (flanco verdadero, no rebote) */
                if (!(GPIOA->IDR & PIN_BTN_PA0)) {
                    g_modo_adc = !g_modo_adc;
                }
                g_pulsador = false;
                NVIC_EnableIRQ(EXTI0_IRQn);   // CMSIS: rehabilitar IRQ 
            }
        }

        /* ── Apagar todos los LEDs de PB antes de actualizar ── */
        GPIOB->ODR &= ~(PIN_LED_PB1 | PIN_LED_PB5 | PIN_LED_PB6
                      | PIN_LED_PB7 | PIN_LED_PB8);

        if (g_modo_adc)
        {
            /* ── Modo sensor de temperatura interno (canal 16) ── */
            uint16_t raw        = adc_leer_canal(16);
            uint32_t mv         = ((uint32_t)raw * 3300UL) / 4095UL;

            /* Formula del Reference Manual (enteros, sin float) */
            int32_t temp;
            if (1430 >= (int32_t)mv)
                temp = (int32_t)(((1430 - (int32_t)mv) * 10) / 43) + 25;
            else
                temp = 25 - (int32_t)((((int32_t)mv - 1430) * 10) / 43);

            /* Barra de LEDs: escalones de 4 grados desde 20 °C */
            if (temp > 20) GPIOB->ODR |= PIN_LED_PB1;
            if (temp > 24) GPIOB->ODR |= PIN_LED_PB5;
            if (temp > 28) GPIOB->ODR |= PIN_LED_PB6;
            if (temp > 32) GPIOB->ODR |= PIN_LED_PB7;
            if (temp > 36) GPIOB->ODR |= PIN_LED_PB8;
        }
        else
        {
            /* ── Modo ADC externo (canal 1, PA1) ── */
            uint16_t val = adc_leer_canal(1);

            if (val >  100) GPIOB->ODR |= PIN_LED_PB1;
            if (val > 1000) GPIOB->ODR |= PIN_LED_PB5;
            if (val > 2000) GPIOB->ODR |= PIN_LED_PB6;
            if (val > 3000) GPIOB->ODR |= PIN_LED_PB7;
            if (val > 4000) GPIOB->ODR |= PIN_LED_PB8;
        }
    }
}
