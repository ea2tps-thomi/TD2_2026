#include <stdint.h>

//Dirección de registros bases de: periféricos, NVIC, AFIO, y RCC
#define RCC_BASE 0x40021000
#define GPIOC_BASE 0x40011000
#define AFIO_BASE 0x40010000
#define GPIOA_BASE 0x40010800
#define GPIOB_BASE 0x40010C00
#define EXTI_BASE 0x40010400
#define NVIC_BASE 0xE000E100

//Dirección de registros base de: ADC1
#define ADC1_BASE 0x40012400

//Diracción de registros de configuración de: RCC_APB2, control de puertos A, B, y C
#define RCC_APB2ENR *(volatile uint32_t *)(RCC_BASE + 0x18)
//#define 

#define GPIOC_CRH *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

// Campos de bit para configuración de RCC_APB2 y periféricos
#define RCC_ADC1EN (1 << 9) //habilitación del clk en ADC1 
#define RCC_IOPCEN (1 << 4) //habilitación del clk en puerto C
#define RCC_IOPBEN (1 << 3) //habilitación del clk en puerto b
#define RCC_IOPAEN (1 << 2) //habilitación del clk en puerto a
#define RCC_AFIOEN (1 << 0) //habilitación de funciones externas

#define GPIOC13 (1UL << 13) //led para usar con SysTick


// --- Registros del SysTick (Cortex-M Core) ---
#define SysTick_BASE 0xE000E010
#define SysTick_CTRL *(volatile uint32_t *)(SysTick_BASE + 0x00)
#define SysTick_LOAD *(volatile uint32_t *)(SysTick_BASE + 0x04)
#define SysTick_VAL  *(volatile uint32_t *)(SysTick_BASE + 0x08)

// Bits de control de SysTick
#define SysTick_CTRL_ENABLE    (1 << 0) // Habilita el contador
#define SysTick_CTRL_TICKINT   (1 << 1) // Habilita la interrupción de SysTick
#define SysTick_CTRL_CLKSOURCE (1 << 2) // Fuente de reloj
#define SysTick_CTRL_COUNTFLAG (1 << 16) // Bandera de desborde
					 
volatile uint32_t tick;

// función que atiende la interrupción
void SysTick_Handler(void)
{
    tick++;
}

// inicialización del systick
void systick_init_ms(void)
{
    tick = 0;
    SysTick_CTRL &= ~SysTick_CTRL_CLKSOURCE; // Reloj de SysTick a HCLK/8 (8MHz / 8 = 1MHz -> 1 tick = 1µs)
    SysTick_LOAD = 999; // Poner valor de RECARGA para 1000 ticks (1ms) (1000 - 1) = 999
    SysTick_VAL = 0; // reset del contador
    SysTick_CTRL |= SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE; // Habilitar la interrupción de SysTick (TICKINT) Y el contador (ENABLE)
}

void main(void){

    RCC_APB2ENR |= RCC_IOPCEN;
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    GPIOC_ODR |= (GPIOC13); //inicio ambos leds a cero.
    systick_init_ms();
 // creación de la variable para almacenar el tiempo de delay del led pc14
	uint32_t delay_pc13 = 500; //500 ms
	uint32_t ultimo_tiempo_pc13 = 0;


    while (1){

	if((tick - ultimo_tiempo_pc13) >= delay_pc13){
          ultimo_tiempo_pc13 = tick;
		GPIOC_ODR ^= GPIOC13; //XOR para no afectar al resto del puerto  
	}
    }
}
