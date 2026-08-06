#include <stdint.h>

//register adress base
#define RCC_BASE 0x40021000
#define GPIOC_BASE 0x40011000
#define SysTick_BASE 0xE000E010

//register adress base + offset 
#define RCC_APB2ENR *(volatile uint32_t *)(RCC_BASE + 0x18)
#define GPIOC_CRH   *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR   *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

//register bit fields
#define RCC_IOPCEN         (1 << 4)	// campo de bit para habilitar clk en puerto C.
#define GPIOC13            (1UL << 13)	// campo de bit para el registro ODR
#define GPIOC_CRH_CNF13_1  (1 << 23)	//campo de bit para CNF[1]
#define GPIOC_CRH_CNF13_0  (1 << 22)	//campo de bit para CNF[0]
#define GPIOC_CRH_MODE13_1 (1 << 21)	//campo de bit para MODE[1]
#define GPIOC_CRH_MODE13_0 (1 << 20)	//campo de bit para MODE[0]

//register SysTick
#define SysTick_CTRL *(volatile uint32_t *)(SysTick_BASE + 0x00)
#define SysTick_LOAD *(volatile uint32_t *)(SysTick_BASE + 0x04)
#define SysTick_VAL  *(volatile uint32_t *)(SysTick_BASE + 0x08)

//SysTick register bit fields
#define SysTick_CTRL_ENABLE (1 << 0)
#define SysTick_CTRL_TICKINT (1 << 1)
#define SysTick_CTRL_CLKSOURCE (1 << 2)

//variable para almacenar los ticks 

#define DELAY_TOGGLE_MS 500	// definimos un delay para comparar con el contador del SysTick
volatile uint32_t ticks = 0;
//realizamos el Handler del SysTick
void SysTick_Handler(void)
{

	ticks++;

}

// realizo la funcion para inicializar y configurar el SysTick
// configuro el systick para generar una interrupcion cada 1mS
//(f_CLK/N)*(t_CLK)= cantidad de ms, ms = 1, despejo N
void SysTick_Init(void)
{

	SysTick_LOAD = 1000 - 1;	// contador a 999
	SysTick_VAL = 0;	// Reseteo el contador
	SysTick_CTRL &= ~SysTick_CTRL_CLKSOURCE;	// Reloj de SysTick a HCLK/8 (8MHz / 8 = 1MHz -> 1 tick = 1µs)
	SysTick_CTRL |= SysTick_CTRL_TICKINT;	// Habilito pedido de  interrpucion cuando el contador llega a cero
	SysTick_CTRL |= SysTick_CTRL_ENABLE;	// Por ùltimo pongo a marchar el contador
}

// Configuro pin 13 como salida y habilitacion de clock 

void gpio_Init(void)
{
	RCC_APB2ENR |= RCC_IOPCEN;	// habilito el reloj en el puerto C. 
	GPIOC_CRH &= ~(GPIOC_CRH_CNF13_0 | GPIOC_CRH_CNF13_1);	//asi formo la condicion para salida a 2MHz push pull 
	GPIOC_CRH |= GPIOC_CRH_MODE13_1;
}

void toggle_led(uint32_t ms)
{

	static uint32_t tiempo_offset = 0;

//evalua si ya pasaron tantos "ms"
	if ((ticks - tiempo_offset) >= ms) {
		GPIOC_ODR ^= GPIOC13;	// Conmuta de estado si llego a cero 
		tiempo_offset = ticks;
	}

}

int main(void)
{

//inicio de microcontrolador 
	gpio_Init();
	SysTick_Init();

	while (1) {
		toggle_led(DELAY_TOGGLE_MS);
	}
	return 0;
}
