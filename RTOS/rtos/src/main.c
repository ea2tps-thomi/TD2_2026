
#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"





void vBlinkTask(void *pvParameters) {
    (void) pvParameters;
    while(1) {
//        GPIOC->ODR ^= (1 << 13); // Toggle PC13
//        vTaskDelay(pdMS_TO_TICKS(500));
        
        GPIOC->BSRR = GPIO_BSRR_BR13; // LED ON
        vTaskDelay(pdMS_TO_TICKS(500));        
        GPIOC->BSRR = GPIO_BSRR_BS13; // LED OFF
        vTaskDelay(pdMS_TO_TICKS(500));        
        
    }
}



int main(void) {

    SystemCoreClockUpdate();

//    if (SystemCoreClock != 72000000UL) {
//        while(1); // si está mal, detener aquí
//    }

    // Habilitar reloj GPIOC
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // Configurar PC13 como salida push-pull a 10 MHz
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0;

    // Crear tarea Blink
    // Stack: 256 palabras (1024 bytes), prioridad 1
    BaseType_t res = xTaskCreate(vBlinkTask,	//funcion de la tarea
    				 "Blink",	//nombre
    				  256,		//tamaño dl stack en palabras
    				  NULL, 	//parametro pasado a la tarea
    				  1, 		//prioridad
    				  NULL);	//manejador de tarea
    if (res != pdPASS) {
        while(1); // si falla creación de tarea, detener
    }

    // Arrancar scheduler
    vTaskStartScheduler();

    // Nunca debería llegar aquí
    for (;;);

    while(1);
}

