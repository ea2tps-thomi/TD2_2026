.syntax unified
.cpu cortex-m3
.thumb

.global _esstack

.word _esstack
.word _reset

.thumb_func
_reset:
    bl main
    b .
