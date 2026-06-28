// Ejercicio 2 
#include <stdint.h>
#include <stdbool.h>

// REGISTROS BASE
#define RCC_BASE    0x40021000
#define AFIO_BASE   0x40010000
#define GPIOA_BASE  0x40010800
#define GPIOB_BASE  0x40010C00
#define GPIOC_BASE  0x40011000
#define EXTI_REAL   0x40010400
#define NVIC_BASE   0xE000E100
#define ADC1_BASE   0x40012400 

// RCC 
#define RCC_APB2ENR  *(volatile uint32_t *)(RCC_BASE + 0x18)
#define RCC_IOPAEN   (1 << 2)
#define RCC_IOPBEN   (1 << 3)
#define RCC_IOPCEN   (1 << 4)
#define RCC_AFIOEN   (1 << 0)
#define RCC_ADC1EN   (1 << 9)

// GPIO
#define GPIOA_CRL   *(volatile uint32_t *)(GPIOA_BASE + 0x00)
#define GPIOA_ODR   *(volatile uint32_t *)(GPIOA_BASE + 0x0C)
#define GPIOA_IDR   *(volatile uint32_t *)(GPIOA_BASE + 0x08)
#define GPIOB_CRL   *(volatile uint32_t *)(GPIOB_BASE + 0x00)
#define GPIOB_CRH   *(volatile uint32_t *)(GPIOB_BASE + 0x04)
#define GPIOB_ODR   *(volatile uint32_t *)(GPIOB_BASE + 0x0C)
#define GPIOC_CRH   *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR   *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

// AFIO 
#define AFIO_MAPR     *(volatile uint32_t *)(AFIO_BASE + 0x04)
#define AFIO_EXTICR1  *(volatile uint32_t *)(AFIO_BASE + 0x08)

// EXTI
#define EXTI_IMR    *(volatile uint32_t *)(EXTI_REAL + 0x00)
#define EXTI_FTSR   *(volatile uint32_t *)(EXTI_REAL + 0x0C)
#define EXTI_PR     *(volatile uint32_t *)(EXTI_REAL + 0x14)

// NVIC
#define NVIC_ISER0  *(volatile uint32_t *)(NVIC_BASE + 0x000) 
#define NVIC_ICER0  *(volatile uint32_t *)(NVIC_BASE + 0x080)
#define NVIC_IPR1   *(volatile uint32_t *)(NVIC_BASE + 0x304)

// ADC1
#define ADC_CR2     *(volatile uint32_t *)(ADC1_BASE + 0x08)
#define ADC1_SR     *(volatile uint32_t *)(ADC1_BASE + 0x00)
#define ADC1_SQR3   *(volatile uint32_t *)(ADC1_BASE + 0x34)
#define ADC1_DR     *(volatile uint32_t *)(ADC1_BASE + 0x4C)
#define ADC1_SMPR1  *(volatile uint32_t *)(ADC1_BASE + 0x0C)

// SysTick
#define SysTick_BASE 0xE000E010
#define SysTick_CTRL *(volatile uint32_t *)(SysTick_BASE + 0x00)
#define SysTick_LOAD *(volatile uint32_t *)(SysTick_BASE + 0x04)
#define SysTick_VAL  *(volatile uint32_t *)(SysTick_BASE + 0x08)

// Pines Asignados
#define GPIOC13  (1UL << 13)
#define GPIOB0   (1UL << 0)
#define LED_PB1  (1UL << 1)
#define LED_PB5  (1UL << 5)
#define LED_PB6  (1UL << 6)
#define LED_PB7  (1UL << 7)
#define LED_PB8  (1UL << 8)

// Constantes
#define ADC_SWSTART (1UL << 22)
#define ADC_EOC     (1UL << 1)
#define ADC_TSVREFE (1UL << 23)
#define ADC_ADON    (1UL << 0)

#define SysTick_CTRL_ENABLE    (1 << 0)
#define SysTick_CTRL_TICKINT   (1 << 1)

volatile uint32_t tick;
volatile uint32_t captura_tick = 0;
volatile bool estado_pulsador = false;

void SysTick_Handler(void) {
    tick++;
}

void systick_init_ms(void) {
    tick = 0;
    SysTick_CTRL &= ~(1 << 2); // HCLK/8
    SysTick_LOAD = 999;
    SysTick_VAL = 0;
    SysTick_CTRL |= SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}

void EXTI0_IRQHandler(void) {
    NVIC_ICER0 = (1UL << 6); 
    estado_pulsador = true;
    EXTI_PR |= (1UL << 0);
    captura_tick = tick;
}

uint16_t ADC1_lectura_canal(uint8_t canal) {
    ADC1_SQR3 &= ~(0x1FUL << 0);
    ADC1_SQR3 |= (canal << 0);
    ADC_CR2 |= ADC_SWSTART;
    while ((ADC1_SR & ADC_EOC) == 0) {
        // Espera activa de conversión
    }
    return (uint16_t)ADC1_DR;
}

void main(void) {
    // 1. Habilitar Clocks
    RCC_APB2ENR |= RCC_AFIOEN | RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN | RCC_ADC1EN;
    
    // LIBERAR JTAG: Desactiva pines JTAG para usar PB3 y PB4 libremente si hiciera falta
    AFIO_MAPR &= ~(7UL << 24);
    AFIO_MAPR |= (2UL << 24); // SWJ-DP Disable (JTAG-DP Disabled, SW-DP Enabled)

    // 2. Configurar PC13 (Salida Push-Pull 2MHz)
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    GPIOC_ODR |= GPIOC13;

    // 3. Inicialización Segura Pin por Pin del Puerto B (Evita solapamientos)
    // Configurar PB0 y PB1 como Salida Push-Pull 2MHz (0010b -> 0x2)
    GPIOB_CRL &= ~(0x000000FFUL);
    GPIOB_CRL |=  (0x00000022UL);

    // Configurar PB5, PB6 y PB7 como Salida Push-Pull 2MHz (0010b -> 0x2)
    GPIOB_CRL &= ~(0xFFF00000UL);
    GPIOB_CRL |=  (0x22200000UL);

    // Configurar PB8 (Registro CRH) como Salida Push-Pull 2MHz
    GPIOB_CRH &= ~(0x0000000FUL);
    GPIOB_CRH |=  (0x00000002UL);

    // Asegurar todos los LEDs apagados al inicio
    GPIOB_ODR &= ~(LED_PB1 | LED_PB5 | LED_PB6 | LED_PB7 | LED_PB8);

    // 4. Configurar Calibración Completa del ADC
    ADC_CR2 |= ADC_TSVREFE;
    ADC_CR2 |= ADC_ADON;
    ADC1_SMPR1 |= (7UL << 18); // 239.5 ciclos para el canal 16
    
    for(volatile int i = 0; i < 2000; i++); // Espera de estabilización tSTAB
    
    ADC_CR2 |= (1UL << 2); // Iniciar calibración (CAL)
    while(ADC_CR2 & (1UL << 2)) {
        // Espera a que termine la calibración
    }

    // 5. Configurar Botón en PA0 (Entrada con Pull-Up)
    GPIOA_CRL &= ~(0xFUL << 0);
    GPIOA_CRL |=  (0x8UL << 0);
    GPIOA_ODR |=  (1UL << 0);

    // 6. Configurar Interrupción EXTI0 en PA0
    AFIO_EXTICR1 &= ~(0xFUL << 0);
    EXTI_FTSR |= (1UL << 0);
    EXTI_IMR  |= (1UL << 0);

    // 7. Configurar NVIC
    NVIC_IPR1 &= ~(0xFFUL << 24);
    NVIC_ISER0 |= (1UL << 6);
    
    // Configurar PA1 como Entrada Analógica (0000b)
    GPIOA_CRL &= ~(0xFUL << 4);

    // 8. Inicializar reloj de sistema
    systick_init_ms();
    
    uint32_t delay_pc13  = 500;
    uint32_t ultimo_pc13 = 0;
    uint32_t ultimo_adc  = 0;
    
    uint16_t valor_pa1 = 0;
    uint16_t valor_temp_crudo = 0;
    float temperatura_celsius = 0.0f;

    while (1) {
        // --- Manejo de Antirrebote del Pulsador ---
        if(estado_pulsador){
            if((tick - captura_tick) >= 20){
                if((GPIOA_IDR & (1UL << 0)) == 0){
                    GPIOB_ODR ^= GPIOB0; 
                }
                estado_pulsador = false;
                NVIC_ISER0 = (1UL << 6); 
            }
        }

        // --- Parpadeo del LED Integrado (PC13) ---
        if ((tick - ultimo_pc13) >= delay_pc13){
            ultimo_pc13 = tick;
            GPIOC_ODR ^= GPIOC13;
        }
                
        // --- Procesamiento de Lecturas del ADC (Cada 200ms) ---
        if ((tick - ultimo_adc) >= 200) {
            ultimo_adc = tick;

            // Muestrear canales
            valor_pa1 = ADC1_lectura_canal(1);
            valor_temp_crudo = ADC1_lectura_canal(16);

            // Conversión del sensor de temperatura a Punto Fijo
            uint32_t vsense_mv = ((uint32_t)valor_temp_crudo * 3300) / 4095;
            if (1430 >= vsense_mv) {
                temperatura_celsius = (float)((1430 - vsense_mv) * 10 / 43) + 25.0f;
            } else {
                temperatura_celsius = 25.0f - (float)((vsense_mv - 1430) * 10 / 43);
            }

            // LIMPIAR BARRA DE LEDS (Línea restaurada)
            GPIOB_ODR &= ~(LED_PB1 | LED_PB5 | LED_PB6 | LED_PB7 | LED_PB8);

            // Actualizar la secuencia de la barra según PA1
            if (valor_pa1 > 100)  { GPIOB_ODR |= LED_PB1; }
            if (valor_pa1 > 1000) { GPIOB_ODR |= LED_PB5; }
            if (valor_pa1 > 2000) { GPIOB_ODR |= LED_PB6; }
            if (valor_pa1 > 3000) { GPIOB_ODR |= LED_PB7; }
            if (valor_pa1 > 4000) { GPIOB_ODR |= LED_PB8; }
        } 
    }
}
