// Ejercicio 1 y 2 
#include <stdint.h>

//REGISTROS BASE
#define RCC_BASE    0x40021000
#define AFIO_BASE   0x40010000
#define GPIOA_BASE  0x40010800
#define GPIOB_BASE  0x40010C00
#define GPIOC_BASE  0x40011000
#define EXTI_REAL   0x40010400
#define NVIC_BASE   0xE000E100
#define ADC1_BASE   0x40012400 

//RCC 
#define RCC_APB2ENR  *(volatile uint32_t *)(RCC_BASE + 0x18)
#define RCC_IOPAEN   (1 << 2)
#define RCC_IOPBEN   (1 << 3)
#define RCC_IOPCEN   (1 << 4)
#define RCC_AFIOEN   (1 << 0)   // clock para AFIO
#define RCC_ADC1EN   (1 << 9) //habilitación del clk en ADC1
#define ADC_ADON     (1 << 0)  //habilitación del ADC1

//GPIO
#define GPIOA_CRL  *(volatile uint32_t *)(GPIOA_BASE + 0x00)
#define GPIOA_ODR  *(volatile uint32_t *)(GPIOA_BASE + 0x0C)
#define GPIOB_CRL  *(volatile uint32_t *)(GPIOB_BASE + 0x00)
#define GPIOB_CRH  *(volatile uint32_t *)(GPIOB_BASE + 0x04)
#define GPIOB_ODR  *(volatile uint32_t *)(GPIOB_BASE + 0x0C)
#define GPIOC_CRH  *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR  *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

//AFIO 
// EXTICR1: selecciona qué puerto mapea EXTI0 hasta EXTI3, PA0 está mapeado acá
#define AFIO_EXTICR1  *(volatile uint32_t *)(AFIO_BASE + 0x08)

//EXTI tabla 64 del manual de periféricos
#define EXTI_IMR   *(volatile uint32_t *)(EXTI_REAL + 0x00)  // Interrupt Mask
#define EXTI_FTSR  *(volatile uint32_t *)(EXTI_REAL + 0x0C)  // Falling Trigger
#define EXTI_PR    *(volatile uint32_t *)(EXTI_REAL + 0x14)  // Pending Register
#define EXTI_RTSR *(volatile uint32_t *)(EXTI_REAL + 0x08)   //Rising trigger 


//NVIC
// ISER0 habilita IRQs 0-31. EXTI0 = IRQ #6
#define NVIC_ISER0  *(volatile uint32_t *)(NVIC_BASE + 0x000)  // 0x000 es el offset del ISER0, entonces 0x00 + NVIC_BASE = 0xE000E100 
// IPR1: prioridad de IRQ #6 (byte 2 del registro IPR1)
#define NVIC_IPR1   *(volatile uint32_t *)(NVIC_BASE + 0x304) //  // 0x304 es el offset del IPR1 (van avanzando los bits LSB en múltiplos de 4, es decir  0, 4, C), entonces 0x304 + NVIC_BASE = 0xE000E404

//ADC 
#define ADC_CR2 *(volatile uint32_t *)(ADC1_BASE + 0x08)


//Registros del SysTick
#define SysTick_BASE 0xE000E010
#define SysTick_CTRL *(volatile uint32_t *)(SysTick_BASE + 0x00)
#define SysTick_LOAD *(volatile uint32_t *)(SysTick_BASE + 0x04)
#define SysTick_VAL  *(volatile uint32_t *)(SysTick_BASE + 0x08)

//Pin del led incorporado en PC13
#define GPIOC13  (1UL << 13) // led incorporado tp1
//Pin para el led del pulsador en PA0
#define GPIOB0   (1UL << 0)
// Pines para los 5 LEDs desde PB1-PB4-PB5-PB6-PB7
#define LED_PB1  (1UL << 1)
#define LED_PB5  (1UL << 5)
#define LED_PB6  (1UL << 6)
#define LED_PB7  (1UL << 7)
#define LED_PB8  (1UL << 8) // se desplaza 8 porque es el inicio de la parte alta del puerto B (para el ODR que es de 0 a 31)
// FALTA CONGIGURACION PARA EL PIN CORRESPONDIENTE AL ADC1 PA1
//#define ADC1_IN1 

// Bits de control de SysTick
#define SysTick_CTRL_ENABLE    (1 << 0) // Habilita el contador
#define SysTick_CTRL_TICKINT   (1 << 1) // Habilita la interrupción de SysTick
#define SysTick_CTRL_CLKSOURCE (1 << 2) // Fuente de reloj
#define SysTick_CTRL_COUNTFLAG (1 << 16) // Bandera de desborde

// Variables y Función del SysTick
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

// Rutina del servicio de interrupción ISR: mapeada en EXTI0
// El nombre debe coincidir con la entrada en la tabla de vectores (crt.s)
void EXTI0_IRQHandler(void)
{
  //NVIC_ISER0 &= ~(1UL << 6);      // deshabilitar IRQ #6 en ISER0 para evitar rebotes temporalmente
    // Toggle del LED en PB0
    GPIOB_ODR ^= GPIOB0;

    // Limpiar el flag de pending (escribir 1 al bit correspondiente)
    EXTI_PR |= (1UL << 0);
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
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    GPIOC_ODR |= GPIOC13; //inicio ambos leds a cero.

    // PB0 → LED botón, PB1, PB4..PB7 → LEDs secuencia (todos push-pull 2MHz)
    GPIOB_CRL &= ~(0xFFF000FFUL << 0);           // limpiar PB0, PB1, quedan igual PB2,3 y 4, limpia PB5, 6 y 7
    GPIOB_CRL |=  (0x22UL << 0);           // PB0 y PB1 salida 2MHz
    GPIOB_CRL |=  (0x222UL << 20);        // PB5..PB7 salida 2MHz
    GPIOB_CRH &= 0xFFFFFFF0UL;              //limpio el pin 8 de la parte alta del puerto B 
    GPIOB_CRH |= 0x00000002UL;              // pin 8 como salida del puerto B

    // Inicializar todos apagados
    static const uint32_t leds[] = {LED_PB1, LED_PB5, LED_PB6, LED_PB7, LED_PB8};
    
    GPIOB_ODR &= ~(LED_PB1 | LED_PB5 | LED_PB6 | LED_PB7 | LED_PB8);
    
    
    // PA0 → entrada con pull-up (botón, activo en bajo)
    GPIOA_CRL &= ~(0xFUL << 0);
    GPIOA_CRL |=  (0x8UL << 0);   // MODE=00 (input), CNF=10 (pull-up/down)
    GPIOA_ODR |=  (1UL << 0);   // ODR=1 → pull-up activado tabla 20 
    
    // AFIO: mapear a PA0 → EXTI0
    AFIO_EXTICR1 &= ~(0xFUL << 0);  // bits [3:0] = 0000 → PA figura 21 del manual de perifericos

    // EXTI: habilitar las interrpuciones por PA0 y configurarlo por flanco de bajada
    EXTI_FTSR |= (1UL << 0);   //disparo en flanco descendente por tener pullup interna y estar en 1, y cuando se pulsa pasa a cero y etonces se genera el flanco de bajada
    EXTI_IMR  |= (1UL << 0);   // desenmascarar línea EXTI0, es decir habilitarla 

    // NVIC: prioridad e habilitación de EXTI0 (IRQ #6) (es el lugar de esa interrupción en el crt.s)
    // IPR1 cubre IRQs 4-7; IRQ6 está en los bits [31:24] de IPR1
    NVIC_IPR1 &= ~(0xFFUL << 24);    // prioridad 0 (la más alta)
    NVIC_ISER0 |= (1UL << 6);      // habilitar IRQ #6 en ISER0
    
    //GPIO DEL ADC1 -------- PA1 
    GPIOA_CRL &= ~((0xFUL << 4)); //CONFIGURDO COMO ENTRADA ADC1 DEL PA1
    
    // Systick:
    systick_init_ms();
    
  uint32_t delay_pc13   = 500;
  uint32_t ultimo_pc13  = 0;


while (1){

        GPIOB_ODR |= LED_PB1 | LED_PB5 | LED_PB6 | LED_PB7 | LED_PB8;
        if ((tick - ultimo_pc13) >= delay_pc13){
                ultimo_pc13 = tick;
                GPIOC_ODR ^= GPIOC13;
                }
        }
}
