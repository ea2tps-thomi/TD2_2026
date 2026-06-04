#include <stdint.h>

// register address
#define RCC_BASE 0x40021000
#define GPIOC_BASE 0x40011000

#define RCC_APB2ENR *(volatile uint32_t *)(RCC_BASE + 0x18)
#define GPIOC_CRH *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

// bit fields
#define RCC_IOPCEN (1 << 4)
#define GPIOC13 (1UL << 13)
#define GPIOC14 (1UL << 14) // nuevo led a usar con SysTick

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

void main(void)
{
    RCC_APB2ENR |= RCC_IOPCEN;
    GPIOC_CRH &= 0xF00FFFFF;
    GPIOC_CRH |= 0x02200000;
    GPIOC_ODR |= (GPIOC13 | GPIOC14); //inicio ambos leds a cero.
    systick_init_ms();
 // creación de la variable para almacenar el tiempo de delay del led pc14
	uint32_t delay_pc13 = 250; //250 ms
	uint32_t delay_pc14 = 500; //500 ms
	uint32_t ultimo_tiempo_pc13 = 0;
        uint32_t ultimo_tiempo_pc14 = 0;


    while (1){

	if((tick - ultimo_tiempo_pc13) >= delay_pc13){
          ultimo_tiempo_pc13 = tick;
		GPIOC_ODR ^= GPIOC13; //XOR para no afectar al resto del puerto  
	}
	   if((tick - ultimo_tiempo_pc14) >= delay_pc14){
          ultimo_tiempo_pc14 = tick;
                GPIOC_ODR ^= GPIOC14; //XOR para no afectar al resto del puerto  
        }

    }
}
