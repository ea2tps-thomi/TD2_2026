//EJERCICIO 2 CON ADC
.syntax unified
.cpu cortex-m3
.thumb

.global _esstack

// ─── Tabla de vectores ────────────────────────────────────────────────────────
.word _esstack          // 0: Stack pointer inicial
.word _reset            // 1: Reset handler
.word 0                 // 2: NMI
.word 0                 // 3: HardFault
.word 0                 // 4: MemManage
.word 0                 // 5: BusFault
.word 0                 // 6: UsageFault
.word 0                 // 7: reservado
.word 0                 // 8: reservado
.word 0                 // 9: reservado
.word 0                 // 10: reservado
.word 0                 // 11: SVCall
.word 0                 // 12: Debug Monitor
.word 0                 // 13: reservado
.word 0                 // 14: PendSV
.word SysTick_Handler   // 15: El handler del SysTick 
// ─── IRQs del STM32F103 ───────────────────────────────────────────────────────
.word 0                 // IRQ0: WWDG
.word 0                 // IRQ1: PVD
.word 0                 // IRQ2: TAMPER
.word 0                 // IRQ3: RTC
.word 0                 // IRQ4: FLASH
.word 0                 // IRQ5: RCC
.word EXTI0_IRQHandler  // IRQ6: EXTI0  ← botón PA0

.thumb_func
_reset:
    bl main
    b .
