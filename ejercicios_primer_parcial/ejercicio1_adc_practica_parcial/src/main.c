#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"  // archivo de cabecera CMSIS

// Variable global para almacenar el resultado del ADC (Debe ser volatile)
volatile uint16_t adc_value = 0;

// Prototipos de funciones
void ADC1_Init_Timer_Trigger(void);
void TIM2_Init(void);

int main(void) {
    // 1. Inicializar el sistema y los relojes principales (usualmente provisto por el startup)
    SystemInit(); 

    // 2. Configurar el ADC para que espere el disparo del Timer
    ADC1_Init_Timer_Trigger();
    
    // 3. Configurar e iniciar el Timer 2 (Esto arranca el ciclo automático)
    TIM2_Init();

    // 4. Bucle principal de la aplicación (100% libre de bloqueos)
    while(1) {
    
      __WFI(); // Instrucción "Wait For Interrupt": duerme la CPU para ahorrar energía
        // Tu código principal corre libremente aquí.
        // No necesitas usar retardos (delays).
        // Cada 100ms, 'adc_value' se actualizará automáticamente en segundo plano.
    }
}

/**
 * @brief Configura el ADC1 para leer el canal 0 (PA0) activado por el Timer 2.
 */
void ADC1_Init_Timer_Trigger(void) {
    // 1. Habilitar relojes para GPIOA y el periférico ADC1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Reloj GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;   // Reloj ADC1

    // 2. Configurar el Prescaler del ADC (PCLK2 dividido por 6)
    // Si el sistema corre a 72MHz, el ADC funcionará a 12MHz (máximo permitido < 14MHz)
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;        
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;    

    // 3. Configurar PA0 como Entrada Analógica
    // CNF0[1:0] = 00 (Analog mode), MODE0[1:0] = 00 (Input mode)
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0); 

    // 4. Configurar la secuencia del grupo regular del ADC
    ADC1->SQR1 &= ~ADC_SQR1_L;           // L[3:0] = 0000 -> 1 sola conversión en la secuencia
    ADC1->SQR3 &= ~ADC_SQR3_SQ1;         // SQ1[4:0] = 00000 -> El primer canal a medir es el Canal 0

    // 5. Tiempo de muestreo (Sample Time) para el Canal 0
    // Configurado a 55.5 ciclos para asegurar una lectura estable del potenciómetro
    ADC1->SMPR2 |= (ADC_SMPR2_SMP0_1 | ADC_SMPR2_SMP0_0); 

    // 6. Configurar el disparador externo por Hardware (Timer 2)
    ADC1->CR2 |= ADC_CR2_EXTTRIG;         // Habilitar la conversión por eventos externos
    ADC1->CR2 &= ~ADC_CR2_EXTSEL;         // Limpiar bits de selección
    ADC1->CR2 |= ADC_CR2_EXTSEL_1;        // Asignar el valor binario 010 (Evento TIM2 CC2)

    // 7. Habilitar la Interrupción por Fin de Conversión (EOC)
    ADC1->CR1 |= ADC_CR1_EOCIE;           

    // 8. Configurar el controlador de interrupciones del núcleo (NVIC)
    NVIC_EnableIRQ(ADC1_2_IRQn);          // Habilitar la línea compartida de interrupción ADC1 y ADC2
    NVIC_SetPriority(ADC1_2_IRQn, 1);     // Prioridad opcional

    // 9. Encendido del ADC y Rutina de Calibración obligatoria
    ADC1->CR2 |= ADC_CR2_ADON;            // Primer ADON: Despierta al ADC de su estado de bajo consumo
    
    ADC1->CR2 |= ADC_CR2_RSTCAL;          // Iniciar reset de calibración
    while(ADC1->CR2 & ADC_CR2_RSTCAL);    // Esperar a que el hardware limpie el bit
    
    ADC1->CR2 |= ADC_CR2_CAL;             // Iniciar calibración propiamente dicha
    while(ADC1->CR2 & ADC_CR2_CAL);       // Esperar a que finalice la calibración por hardware
}

/**
 * @brief Configura el Timer 2 para enviar un pulso de disparo (Trigger) cada 100ms.
 */
void TIM2_Init(void) {
    // 1. Habilitar el reloj del módulo Timer 2
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 2. Configurar la base de tiempo (Asumiendo reloj interno a 72MHz)
    // Frecuencia deseada = 10Hz (Cada 0.1s). Reloj = 72MHz / (7200 * 1000) = 10Hz
    TIM2->PSC = 7200 - 1;                 // Prescaler: El temporizador cuenta a una velocidad de 10kHz
    TIM2->ARR = 1000 - 1;                 // Periodo (Auto-reload): Cuenta hasta 1000 pasos (100ms)

    // 3. Configurar el Canal 2 en modo "Output Compare - Toggle" (Conmutación)
    // Al alcanzar la igualdad en la cuenta, enviará el flanco interno que el ADC está esperando
    TIM2->CCMR1 &= ~TIM_CCMR1_OC2M;
    TIM2->CCMR1 |= (TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_0); // Modo Toggle en Match

    // 4. Habilitar la salida física interna del canal 2 hacia el ADC
    TIM2->CCER |= TIM_CCER_CC2E;

    // 5. Arrancar el conteo del temporizador
    TIM2->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Manejador de la Interrupción (ISR) para el ADC1 y ADC2.
 */
void ADC1_2_IRQHandler(void) {
    // Verificar si el bit de Fin de Conversión (EOC) está activo en el registro de estado
    if (ADC1->SR & ADC_SR_EOC) {
        // Almacenar el valor digital (0 a 4095) en la variable global.
        // Importante: Leer el registro ADC1->DR limpia automáticamente el flag EOC en el hardware.
        adc_value = (uint16_t)(ADC1->DR);
    }
}
                 
 
