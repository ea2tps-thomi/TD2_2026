//------------------------------------------EJERCICIO 2 CON ADC------------------------------------------


#include <stdint.h>

// ─── Bases ────────────────────────────────────────────────────────────────────
#define RCC_BASE    0x40021000
#define AFIO_BASE   0x40010000
#define GPIOA_BASE  0x40010800
#define GPIOB_BASE  0x40010C00
#define GPIOC_BASE  0x40011000
//#define EXTI_BASE   0xE000E000  // (EXTI está en 0x40010400)
#define EXTI_REAL   0x40010400
#define NVIC_BASE   0xE000E000
#define ADC1_BASE   0x40012400 //ADC1
#define ADC_ADON (1 << 0)    

// ─── RCC ──────────────────────────────────────────────────────────────────────
#define RCC_APB2ENR  *(volatile uint32_t *)(RCC_BASE + 0x18)
#define RCC_IOPAEN   (1 << 2)
#define RCC_IOPBEN   (1 << 3)
#define RCC_IOPCEN   (1 << 4)
#define RCC_AFIOEN   (1 << 0)   // clock para AFIO
#define RCC_ADC1EN (1 << 9) //habilitación del clk en ADC1

// ─── GPIO ─────────────────────────────────────────────────────────────────────
#define GPIOA_CRL  *(volatile uint32_t *)(GPIOA_BASE + 0x00)
#define GPIOA_ODR  *(volatile uint32_t *)(GPIOA_BASE + 0x0C)
#define GPIOB_CRL  *(volatile uint32_t *)(GPIOB_BASE + 0x00)
#define GPIOB_ODR  *(volatile uint32_t *)(GPIOB_BASE + 0x0C)
#define GPIOC_CRH  *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR  *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

// ─── AFIO ─────────────────────────────────────────────────────────────────────
// EXTICR1: selecciona qué puerto mapea EXTI0..3
#define AFIO_EXTICR1  *(volatile uint32_t *)(AFIO_BASE + 0x08)

// ─── EXTI ─────────────────────────────────────────────────────────────────────
#define EXTI_IMR   *(volatile uint32_t *)(EXTI_REAL + 0x00)  // Interrupt Mask
#define EXTI_FTSR  *(volatile uint32_t *)(EXTI_REAL + 0x0C)  // Falling Trigger
#define EXTI_PR    *(volatile uint32_t *)(EXTI_REAL + 0x14)  // Pending Register
#define EXTI_RTSR *(volatile uint32_t *)(EXTI_REAL + 0x08)


// ─── NVIC ─────────────────────────────────────────────────────────────────────
// ISER0 habilita IRQs 0-31. EXTI0 = IRQ #6
#define NVIC_ISER0  *(volatile uint32_t *)(0xE000E100)
// IPR1: prioridad de IRQ #6 (byte 2 del registro IPR1)
#define NVIC_IPR1   *(volatile uint32_t *)(0xE000E404)

//───────────────── ADC ─────────────────
#define ADC_CR2 * (volatile uint32_t *)(ADC1_BASE + 0x08)


// --- Registros del SysTick (Cortex-M Core) ---
#define SysTick_BASE 0xE000E010
#define SysTick_CTRL *(volatile uint32_t *)(SysTick_BASE + 0x00)
#define SysTick_LOAD *(volatile uint32_t *)(SysTick_BASE + 0x04)
#define SysTick_VAL  *(volatile uint32_t *)(SysTick_BASE + 0x08)

// ─── Pines ────────────────────────────────────────────────────────────────────
#define GPIOB0   (1UL << 0) // led del boton
// Defines para los 5 LEDs
#define LED_PB1  (1UL << 1)
#define LED_PB4  (1UL << 4)
#define LED_PB5  (1UL << 5)
#define LED_PB6  (1UL << 6)
#define LED_PB7  (1UL << 7)
#define ADC1_IN1 

static const uint32_t leds[] = {LED_PB1, LED_PB4, LED_PB5, LED_PB6, LED_PB7};


#define GPIOC13  (1UL << 13) // led incorporado tp1

// Bits de control de SysTick ────────────────────────────────────────────────────────────────────
#define SysTick_CTRL_ENABLE    (1 << 0) // Habilita el contador
#define SysTick_CTRL_TICKINT   (1 << 1) // Habilita la interrupción de SysTick
#define SysTick_CTRL_CLKSOURCE (1 << 2) // Fuente de reloj
#define SysTick_CTRL_COUNTFLAG (1 << 16) // Bandera de desborde

//Función del Sistyc 
volatile uint32_t tick;

// función que atiende la interrupción
void SysTick_Handler(void)
{
    tick++;
}

// Inicialización del systick
void systick_init_ms(void)
{
    tick = 0;
    SysTick_CTRL &= ~SysTick_CTRL_CLKSOURCE; // Reloj de SysTick a HCLK/8 (8MHz / 8 = 1MHz -> 1 tick = 1µs)
    SysTick_LOAD = 999; // Poner valor de RECARGA para 1000 ticks (1ms) (1000 - 1) = 999
    SysTick_VAL = 0; // reset del contador
    SysTick_CTRL |= SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE; // Habilitar la interrupción de SysTick (TICKINT) Y el contador (ENABLE)
}

// ─── ISR: EXTI0 ───────────────────────────────────────────────────────────────
// El nombre debe coincidir con la entrada en la tabla de vectores (crt.s)
void EXTI0_IRQHandler(void)
{
    // Toggle del LED en PB0
    GPIOB_ODR ^= GPIOB0;

    // Limpiar el flag de pending (escribir 1 al bit correspondiente)
    EXTI_PR = (1UL << 0);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
void main(void)
{
    // 1. Habilitar clocks: AFIO, GPIOA, GPIOB, GPIOC, ADC1EN, 
    RCC_APB2ENR |= RCC_AFIOEN | RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN | RCC_ADC1EN;
    
    //ENCENDIDO DEL CANAL ADC1
    ADC_CR2 &= ~ ADC_ADON ;
    ADC_CR2 |=  ADC_ADON ;
    

    // 2. PC13 → salida push-pull 2MHz (LED blink TP1)
    //GPIOC_CRH &= ~(0xF << 20);
    //GPIOC_CRH |=  (0x2 << 20);
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    GPIOC_ODR |= (GPIOC13); //inicio ambos leds a cero.
    
    
    

    // 3. PB0 → salida push-pull 2MHz (LED controlado por botón) MAS 5 LEDS MAS
    //GPIOB_CRL &= ~(0xF << 0);
    //GPIOB_CRL |=  (0x2 << 0);
    
    
    //PB1 AL PB7:
 
    
    // PB0 → LED botón, PB1, PB4..PB7 → LEDs secuencia (todos push-pull 2MHz)
    GPIOB_CRL &= ~(0xFFUL << 0);           // limpiar PB0 y PB1
    GPIOB_CRL |=  (0x22UL << 0);           // PB0 y PB1 salida 2MHz

    GPIOB_CRL &= ~(0xFFFFUL << 16);        // limpiar PB4..PB7
    GPIOB_CRL |=  (0x2222UL << 16);        // PB4..PB7 salida 2MHz

    // Inicializar todos apagados
    GPIOB_ODR &= ~(LED_PB1 | LED_PB4 | LED_PB5 | LED_PB6 | LED_PB7);
    
    
    
    //GPIOB_CRL &= 0x00000FF2;// los grupos de bit mas signifativo en 0 y no se pisa el primer grupo menos signifacativo
    
    //GPIOB_CRL |= (0x22222000); 
    
    
    
    GPIOB_ODR &= ~(0x1FUL << 3);   // limpia bits 3..7
    
    
    // 4. PA0 → entrada con pull-up (botón, activo en bajo)
    GPIOA_CRL &= ~(0xF << 0);
    GPIOA_CRL |=  (0x8 << 0);   // MODE=00 (input), CNF=10 (pull-up/down)
    GPIOA_ODR |=  (1UL << 0);   // ODR=1 → pull-up activado
    
    // 5. AFIO: mapear PA0 → EXTI0 (valor 0x0 = puerto A, ya es el default)
    AFIO_EXTICR1 &= ~(0xF << 0);  // bits [3:0] = 0000 → PA

    // 6. EXTI: habilitar línea 0 y configurar flanco de bajada
    EXTI_RTSR |= (1UL << 0);   // 0x40010408, disparo en flanco ascendente
    //EXTI_FTSR |= (1UL << 0);   // disparo en flanco descendente
    EXTI_IMR  |= (1UL << 0);   // desenmascarar línea EXTI0

    // 7. NVIC: prioridad e habilitación de EXTI0 (IRQ #6)
    // IPR1 cubre IRQs 4-7; IRQ6 está en los bits [31:24] de IPR1
    NVIC_IPR1 &= ~(0xFF << 24);    // prioridad 0 (la más alta)
    NVIC_ISER0 |= (1UL << 6);      // habilitar IRQ #6 en ISER0
    
    //GPIO DEL ADC1 -------- PA1 
    GPIOA_CRL &= ~((0xF << 4)); //CONFIGURDO COMO ENTRADA ADC1 DEL PA1
    
    //8. Systick:
    systick_init_ms();
    
    
    uint32_t delay_pc13   = 500;
    uint32_t ultimo_pc13  = 0;
   

while (1)
{
    // Blink PC13 cada 500ms
    if ((tick - ultimo_pc13) >= delay_pc13)
    {
        ultimo_pc13 = tick;
        GPIOC_ODR ^= GPIOC13;
    }
   
}
}
