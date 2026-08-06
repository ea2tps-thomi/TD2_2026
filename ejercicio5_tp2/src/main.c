#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"  // archivo de cabecera CMSIS


#define VALOR_PASO      2 //valor del paso para controlar la velocidad del efecto "fade"
volatile int16_t duty = 0; //variable para definir el ciclo de trabajo
volatile int8_t paso = VALOR_PASO; //carga el valor del paso 
//  Inicializacion de GPIOs
static void gpio_init(void)
{
    // Habilitar relojes: AFIO, GPIOB, y TIMER 4  
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;  
           
   // PB6 (CANAL 1 DEL TIMER 4)
   //como pin alternate function push-pull  CNF[1:0] = 10 y MODE[1:0] = 11 (MÁX VEL 50 MHZ)         
    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6); //limpio los bits 
    GPIOB->CRL |=  GPIO_CRL_CNF6_1; //configuro como alternate function output
    GPIOB->CRL |= (GPIO_CRL_MODE6 | GPIO_CRL_MODE6_1); //salida a máxima velocidad de conmutación           
    GPIOB->ODR |= ~GPIO_ODR_ODR6;  // LED en PB6 apagado al inicio (en realidad tiene un valor don't care en datasheet)
   
}

void PWM_PB6_Interrupt_Init(void){
//configuracion del timer
// Aunque el bus APB1 corre a un máximo de 36MHz, el multiplicador interno 
    // del STM32F1 entrega los 72MHz completos al TIM4 si el pre-escaler de APB1 es > 1.
TIM4->PSC = 7200-1;//acá va el valor del divisor de frecuencia de APB1 (36MHz) y se puede dividir entre 1-65536, si se pone 0 divide por 1
TIM4->ARR = 150-1; //acá va el valor del auto-reload register y define el periodo (cuenta de 0 a 999)
TIM4->CCR1 = 0;//acá se carga el valor del duty cycle (se inicia en 0, con el led apagado)

//configuracion del PWM
//configura OC1M en 110 (salida PWM en modo 1)
TIM4->CCMR1 &= ~(TIM_CCMR1_OC1M);
TIM4->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);
//habilita la precagra del registro de comparacion (OC1PE)
TIM4->CCMR1 |= TIM_CCMR1_OC1PE;
//habilita la salida del canal 1 en PB6 
TIM4->CCER |= TIM_CCER_CC1E;
// habilita la precarga del regitro de auto recarga
TIM4->CR1 |= TIM_CR1_ARPE;

//configuracion de generacion de interrupcion 
TIM4->DIER |= TIM_DIER_UIE; // Habilitar interrupción por Update (desborde) en el Timer
NVIC_EnableIRQ(TIM4_IRQn); //Activar el vector de TIM4 en el NVIC del núcleo ARM
NVIC_SetPriority(TIM4_IRQn, 2); //Asignar una prioridad intermedia
// habilita el timer e inicia (activa el bit counter)
TIM4->CR1 |= TIM_CR1_CEN;
}
//funcion handler del pwm 
void TIM4_IRQHandler(void) {
    // Verificar si la interrupción fue causada por el evento de actualización (Update Flag)
    if (TIM4->SR & TIM_SR_UIF) {
        
        // ¡OBLIGATORIO!: Limpiar la bandera de interrupción por software.
        // Si no se limpia, el microprocesador se congela reingresando a la ISR continuamente.
        TIM4->SR &= ~TIM_SR_UIF;

        // --- Lógica del Fading del LED ---
        duty += paso;

        // Limitar superiormente usando el valor máximo de ARR (149)
        if (duty >= 149) {      
            duty = 149;
            paso = -VALOR_PASO; // Cambiar dirección: empezar a disminuir el brillo
        } 
        // Limitar inferiormente
        else if (duty <= 0) {
            duty = 0;
            paso = VALOR_PASO;  // Cambiar dirección: empezar a aumentar el brillo
        }

        // Escribir el nuevo ciclo de trabajo en el registro de captura/comparación
        TIM4->CCR1 = duty;
    }
}

int main(void){

    gpio_init();
    PWM_PB6_Interrupt_Init();
    while (1)
    {
     // ¡CPU COMPLETAMENTE LIBRE!
        // Aquí puedes ejecutar lógica compleja, procesar pantallas, leer sensores, etc.
        // El fading del LED en PB6 funciona de forma autónoma en segundo plano.
        
        __WFI(); // Instrucción "Wait For Interrupt": duerme la CPU para ahorrar energía
                 // hasta que ocurra la próxima interrupción de los 15ms.
    } // cierre del bucle while(1)
} // cierre del bucle main 
