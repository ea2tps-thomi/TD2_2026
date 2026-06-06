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

// Pines destinados para leds
#define GPIOC13  (1UL << 13)
//#define GPIOB0   (1UL << 0)
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
volatile bool modo_adc = false;

void SysTick_Handler(void) {
    tick++;
}

void systick_init_ms(void) {
    tick = 0;
    SysTick_CTRL &= ~(1 << 2); 
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
         ADC_CR2 |= ADC_ADON; // Enciende el ADC
         ADC1_SQR3 = canal;   // Selecciona el canal 1 o 16
    
         // Demora de estabilización
         for(volatile int i = 0; i < 50; i++);

         ADC_CR2 |= ADC_SWSTART;
    
         //si el ADC no responde, sale por timeout sin colgar el Systick
         volatile uint32_t timeout = 2000; 
         while ((ADC1_SR & ADC_EOC) == 0) {
         timeout--;
         if(timeout == 0)
         return 0; //devuelve un cero sino realiza correctamente la conversión
         }
         return (uint16_t)ADC1_DR; //devuelve el valor convertido con un entero sin signo de 16 bits 
}

void main(void) {
    //  Habilita los clocks
    RCC_APB2ENR |= RCC_AFIOEN | RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN | RCC_ADC1EN;
    
    // Liberar pines del puerto JTAG en puerto B
    AFIO_MAPR &= ~(7UL << 24);
    AFIO_MAPR |= (2UL << 24);

    // Configurar PC13 (Salida Push-Pull a 2MHz)
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    GPIOC_ODR |= GPIOC13;

    // Inicializacion de pines en Puerto B
    GPIOB_CRL &= ~(0x000000FFUL);
    GPIOB_CRL |=  (0x00000022UL);

    GPIOB_CRL &= ~(0xFFF00000UL);
    GPIOB_CRL |=  (0x22200000UL);

    GPIOB_CRH &= ~(0x0000000FUL);
    GPIOB_CRH |=  (0x00000002UL);

// Iniciamos los leds apagados
    GPIOB_ODR &= ~(LED_PB1 | LED_PB5 | LED_PB6 | LED_PB7 | LED_PB8);

    // Configura el ADC
    ADC_CR2 |= ADC_TSVREFE;
    ADC_CR2 |= ADC_ADON; // 1ra vez 
    ADC1_SMPR1 |= (7UL << 18); 
    
    for(volatile int i = 0; i < 2000; i++); // delay para estabilizar
        ADC_CR2 |= ADC_ADON; // 2da vez para asegurar
    // Configurar Boton en PA0
    GPIOA_CRL &= ~(0xFUL << 0);
    GPIOA_CRL |=  (0x8UL << 0);
    GPIOA_ODR |=  (1UL << 0);

    // Configurar Interrupcion EXTI0
    AFIO_EXTICR1 &= ~(0xFUL << 0);
    EXTI_FTSR |= (1UL << 0);
    EXTI_IMR  |= (1UL << 0);

    // Configurar NVIC
    NVIC_IPR1 &= ~(0xFFUL << 24);
    NVIC_ISER0 |= (1UL << 6);
    
    // Configurar PA1 como Entrada Analógica
    GPIOA_CRL &= ~(0xFUL << 4);

    // Inicializar Systick
    systick_init_ms();
    
    uint32_t delay_pc13  = 500;
    uint32_t ultimo_pc13 = 0;
    uint32_t ultimo_adc  = 0;
    
    uint16_t valor_pa1 = 0;
    uint16_t valor_sensor_int = 0;
    uint32_t vsensado = 0;

    while (1) {
    
      //Parpadeo LED en PC13
        if ((tick - ultimo_pc13) >= delay_pc13){
            ultimo_pc13 = tick;
            GPIOC_ODR ^= GPIOC13;
        }
        
        //Antirrebote Pulsador
        if(estado_pulsador){
            if((tick - captura_tick) >= 20){
                if((GPIOA_IDR & (1UL << 0)) == 0){
                  //  GPIOB_ODR ^= GPIOB0; 
                  modo_adc = !modo_adc; //invertimos el estado cada vez que presionamos
                }
                estado_pulsador = false;
                NVIC_ISER0 = (1UL << 6); 
            }
        }
                GPIOB_ODR &= ~(LED_PB1 | LED_PB5 | LED_PB6 | LED_PB7 | LED_PB8);
                if(modo_adc){
                 valor_sensor_int = ADC1_lectura_canal(16);
                 // Convierte el valor el valor de 12 bits a Milivoltios
                 vsensado = ((uint32_t)valor_sensor_int * 3300)/4095;
                 // Aplicamos la fórmula de la hoja de datos de ST en enteros
                 // Aplicamos la fórmula del Reference Manual usando enteros
                int32_t temp_celsius = 0;
                if (1430 >= vsensado) {
                    temp_celsius = (int32_t)(((1430 - vsensado) * 10) / 43) + 25;
                } else {
                    temp_celsius = 25 - (int32_t)(((vsensado - 1430) * 10) / 43);
                }
                //Rango de 20°C a 40°C en pasos de 4°C porque medimos la temp interna del core, y aumenta en esos valores una vez encendido
                  if (temp_celsius > 20){ 
                  GPIOB_ODR |= LED_PB1;
                  }
                  if (temp_celsius > 24){ 
                  GPIOB_ODR |= LED_PB5;
                  }
                  if (temp_celsius > 28){ 
                  GPIOB_ODR |= LED_PB6; 
                  }
                  if (temp_celsius> 32){ 
                  GPIOB_ODR |= LED_PB7; 
                  }
                  if (temp_celsius > 36){ 
                  GPIOB_ODR |= LED_PB8; 
                  }
                
                } else {
                 valor_pa1 = ADC1_lectura_canal(1);
                  if (valor_pa1 > 100){ 
                  GPIOB_ODR |= LED_PB1; 
                  }
                  if (valor_pa1 > 1000){ 
                  GPIOB_ODR |= LED_PB5; 
                  }
                  if (valor_pa1 > 2000){ 
                  GPIOB_ODR |= LED_PB6; 
                  }
                  if (valor_pa1 > 3000){ 
                  GPIOB_ODR |= LED_PB7; 
                  }
                  if (valor_pa1 > 4000){ 
                  GPIOB_ODR |= LED_PB8; 
                  }
                }
             }
           }
