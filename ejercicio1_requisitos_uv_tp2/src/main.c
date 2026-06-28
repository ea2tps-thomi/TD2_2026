/EJERCICIO 1
#include <stdint.h>
#include <stdbool.h> // archivo cabecera para trabajar con variables booleanas 

// REGISTROS BASE
#define RCC_BASE    0x40021000
#define AFIO_BASE   0x40010000
#define GPIOA_BASE  0x40010800
#define GPIOB_BASE  0x40010C00
#define GPIOC_BASE  0x40011000
#define EXTI_REAL   0x40010400
#define NVIC_BASE   0xE000E100

// RCC 
#define RCC_APB2ENR  *(volatile uint32_t *)(RCC_BASE + 0x18)
#define RCC_IOPAEN   (1 << 2)
#define RCC_IOPBEN   (1 << 3)
#define RCC_IOPCEN   (1 << 4)
#define RCC_AFIOEN   (1 << 0)

// GPIO
#define GPIOA_CRL  *(volatile uint32_t *)(GPIOA_BASE + 0x00)
#define GPIOA_ODR  *(volatile uint32_t *)(GPIOA_BASE + 0x0C)
#define GPIOA_IDR  *(volatile uint32_t *)(GPIOA_BASE + 0x08)
#define GPIOB_CRL  *(volatile uint32_t *)(GPIOB_BASE + 0x00)
#define GPIOB_ODR  *(volatile uint32_t *)(GPIOB_BASE + 0x0C)
#define GPIOC_CRH  *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR  *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

// AFIO 
#define AFIO_EXTICR1  *(volatile uint32_t *)(AFIO_BASE + 0x08)

// EXTI
#define EXTI_IMR   *(volatile uint32_t *)(EXTI_REAL + 0x00)
#define EXTI_FTSR  *(volatile uint32_t *)(EXTI_REAL + 0x0C)
#define EXTI_PR    *(volatile uint32_t *)(EXTI_REAL + 0x14)

// NVIC
#define NVIC_ISER0  *(volatile uint32_t *)(NVIC_BASE + 0x000)
#define NVIC_ICER0  *(volatile uint32_t *)(NVIC_BASE + 0x080)
#define NVIC_IPR1   *(volatile uint32_t *)(NVIC_BASE + 0x304) // Cubre EXTI0 (IRQ 6)
#define NVIC_IPR2   *(volatile uint32_t *)(NVIC_BASE + 0x308) // Cubre EXTI3 (IRQ 9)

// SysTick
#define SysTick_BASE 0xE000E010
#define SysTick_CTRL *(volatile uint32_t *)(SysTick_BASE + 0x00)
#define SysTick_LOAD *(volatile uint32_t *)(SysTick_BASE + 0x04)
#define SysTick_VAL  *(volatile uint32_t *)(SysTick_BASE + 0x08)

#define SysTick_CTRL_ENABLE    (1 << 0)
#define SysTick_CTRL_TICKINT   (1 << 1)
#define SysTick_CTRL_CLKSOURCE (1 << 2)

// PINES DE LEDS
#define GPIOC13  (1UL << 13) // LED integrado de la placa
#define GPIOB0   (1UL << 0)  // LED testigo para PA0

// Variables globales para control de tiempos y estados
volatile uint32_t tick;
volatile uint32_t captura_tick = 0;
volatile uint32_t captura_tick_pa3 = 0;
volatile bool estado_pulsador = false;
volatile bool estado_pulsador_pa3 = false;

void SysTick_Handler(void)
{
    tick++;
}

void systick_init_ms(void)
{
    tick = 0;
    SysTick_CTRL &= ~SysTick_CTRL_CLKSOURCE; // HCLK/8
    SysTick_LOAD = 999;                      // 1ms
    SysTick_VAL = 0;
    SysTick_CTRL |= SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}
// ISR para PA0 (IRQ 6) - Prioridad Alta
void EXTI0_IRQHandler(void)
{
    // Si ya estamos procesando un disparo, ignoramos los rebotes
    if (!estado_pulsador) {
        estado_pulsador = true;
        captura_tick = tick;
    }
    EXTI_PR |= (1UL << 0);    // Limpiar SIEMPRE el flag de interrupción al salir
}

// ISR para PA3 (IRQ 9) - Prioridad Baja
void EXTI3_IRQHandler(void)
{
    // Si ya estamos procesando un disparo, ignoramos los rebotes
    if (!estado_pulsador_pa3) {
        estado_pulsador_pa3 = true;
        captura_tick_pa3 = tick;
        for(volatile int i = 0; i < 300000; i++);
    }
    EXTI_PR |= (1UL << 3);    // Limpiar SIEMPRE el flag de interrupción al salir
}

void main(void)
{
    //  Habilita Clocks (Puertos A, B, C y AFIO)
    RCC_APB2ENR |= RCC_AFIOEN | RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN;
    
    // Configura PC13 como Salida Digital (LED integrado)
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    GPIOC_ODR |= GPIOC13; // Apagado inicial (al ser activo bajo en muchas placas)

    // Configura PB0 como Salida Digital (LED testigo)
    GPIOB_CRL &= ~(0xFUL << 0);
    GPIOB_CRL |=  (0x2UL << 0);
    GPIOB_ODR &= ~GPIOB0; // Inicia apagado

    // Configura PA0 como Entrada con Pull-Up
    GPIOA_CRL &= ~(0xFUL << 0);
    GPIOA_CRL |=  (0x8UL << 0);   
    GPIOA_ODR |=  (1UL << 0);   
    
    // Configura PA3 como Entrada con Pull-Up
    GPIOA_CRL &= ~(0xFUL << 12);
    GPIOA_CRL |=  (0x8UL << 12);  
    GPIOA_ODR |=  (1UL << 3);   

    //  En ruteador AFIO: Conectar PA0->EXTI0 y PA3->EXTI3 
    AFIO_EXTICR1 &= ~((0xFUL << 0) | (0xFUL << 12)); // 0000 en ambos campos asigna Puerto A

    //  EXTI: Configurar flanco de bajada y desenmascarar líneas 0 y 3
    EXTI_FTSR |= (1UL << 0) | (1UL << 3);   
    EXTI_IMR  |= (1UL << 0) | (1UL << 3);   

    // NVIC: Configuración de Prioridades
    // EXTI0 (IRQ 6) -> Prioridad 0 (Más alta)
    NVIC_IPR1 &= ~(0xFFUL << 24);    
    
    // EXTI3 (IRQ 9) -> Prioridad 0x10 (Menor prioridad que 0)
    NVIC_IPR2 &= ~(0xFFUL << 8);    
    NVIC_IPR2 |=  (0x10UL << 8); //  
    
    //  NVIC: Habilitar IRQ6 e IRQ9
    NVIC_ISER0 |= (1UL << 6) | (1UL << 9);   
    
    //  Inicializar SysTick
    systick_init_ms();
    
    uint32_t delay_pc13   = 500;
    uint32_t ultimo_pc13  = 0;
    while (1) 
    {
        // Antirebote por software para PA0
        if (estado_pulsador) {
            if ((tick - captura_tick) >= 20) {
                if ((GPIOA_IDR & (1UL << 0)) == 0) {
                    GPIOB_ODR ^= GPIOB0; // Conmuta el LED testigo en PB0
                }
                // Limpiamos cualquier rebote
                EXTI_PR |= (1UL << 0); 
                estado_pulsador = false;
            }
        }

        //Antirebote por software para PA3
        if (estado_pulsador_pa3) {
            if ((tick - captura_tick_pa3) >= 20) {
                if ((GPIOA_IDR & (1UL << 3)) == 0) {
                    GPIOB_ODR ^= GPIOB0; // Conmuta el LED en PB0  
                }
                // Limpiamos cualquier rebote 
                EXTI_PR |= (1UL << 3); 
                estado_pulsador_pa3 = false; //
            }
        }

        //Parpadeo constante del LED testigo de placa (PC13)
        if ((tick - ultimo_pc13) >= delay_pc13) {
            ultimo_pc13 = tick;
            GPIOC_ODR ^= GPIOC13;
        }
    }
}
