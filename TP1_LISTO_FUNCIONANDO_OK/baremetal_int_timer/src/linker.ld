MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 20K
}

/* 2. Definición de Símbolos Globales */
ENTRY(_reset) /* El punto de entrada, definido en tu crt.s */

/* Símbolo para el final de la pila, usado en tu .isr_vector */
_esstack = ORIGIN(RAM) + LENGTH(RAM); /* 0x20005000 (el final de los 20K de RAM) */

/* 3. Definición de las Secciones de Salida */
SECTIONS
{
  /* SECCIONES EN FLASH */
  /* La tabla de vectores debe ir primero en la Flash */
  .isr_vector :
  {
    KEEP(*(.isr_vector))  /* Mantiene la sección .isr_vector de tu crt.s */
  } > FLASH

  /* Código del programa y datos de solo lectura */
  .text :
  {
    *(.text)             /* Todo el código C y ensamblador */
    *(.text*)            /* Variantes de .text */
    *(.rodata)           /* Datos de solo lectura (constantes) */
    *(.rodata*)
  } > FLASH
  /* SECCIONES EN RAM */
  /* .data: Variables inicializadas con un valor distinto de cero (ej: int x = 10;) */
  /* El linker necesita símbolos para que el startup mueva los datos */
  _sidata = LOADADDR(.data); /* Dirección de los valores en FLASH */

  .data :
  {
    _sdata = .;           /* Inicio de .data en RAM */
    *(.data)
    *(.data*)
    _edata = .;           /* Fin de .data en RAM */
  } > RAM AT> FLASH     /* Ubicada en RAM, pero cargada desde FLASH */
  /* .bss: Variables inicializadas a cero o sin inicializar */
  /* (ej: static int y; o int z = 0;) */
  .bss :
  {
    _sbss = .;            /* Símbolo de inicio de .bss */
    *(.bss)
    *(.bss*)
    *(COMMON)
    _ebss = .;            /* Símbolo de fin de .bss */
  } > RAM               /* Solo ocupa espacio en RAM */
}
