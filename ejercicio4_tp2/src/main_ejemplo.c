#include "stm32f1xx.h"
#include <stdint.h>

/*
 * TP2 - Ejercicio 4 clase UART por zoom.
 * USO = para probar comunicación desde la BluePill ejecutamos este scipt,
 * desde la PC, configuramos la terminal con:
 * sudo stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb -crtscts -echo icanon min 1 time 0
 * luego, en una terminal cat </dev/ttyUSB0 (escuchamos el puerto serie ttyUSB0) y en otra terminal
 * echo "chau"> /dev/ttyUSB0 (enviamos string por el puerto serie). 
 */

int main(void)
{ 
	char *c = "HOLA\r\n"; //string a enviar
	//habilitamos clocks
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

	//configuramos pines TX (PA9)
	GPIOA->CRH &= ~(0xF<<4);        
	GPIOA->CRH |= (0xB<<4);	//alternate_function
	
	//configurar trama 8N1        
	USART1->CR1 &= ~USART_CR1_M;	//M=0 configura 8 bits de datos
	USART1->CR1 &= ~USART_CR1_PCE;	//PCE=0 sin paridad
	USART1->CR2 &= ~USART_CR2_STOP;	//STOP=0 un bit de stop

	//configurar BRR
	USART1->BRR = 0x1D4C;	//9600 baudios

	//habilitar USART, TX
	USART1->CR1 |= USART_CR1_UE;
	USART1->CR1 |= USART_CR1_TE;

	//enviar string
	while(*c)
	{
		while(!(USART1->SR & USART_SR_TXE));	//si DR quedó libre se carga un nuevo dato
		USART1->DR = *c;	//carga dato en DR
		c++;		//se apunta al siguiente caracter del string a enviar
	}

	//-----Recepción y reenvio de caracter recibido-----
	//comentar este bloque para probar solo la transmisión
		
	//configuramos pines RX (PA10) como entrada flotante
	GPIOA->CRH &= ~(0xF<<8);
	GPIOA->CRH |= (0x4<<8);
	
	//habiitar RX
	USART1->CR1 |= USART_CR1_RE;

	//recibir por polling y reenviar caracter recibido
	int fin = 0;
	while(!fin)
	{
		if(USART1->SR & USART_SR_RXNE)	//si el dato se recibió en DR se lee
		{ char r = USART1->DR;
			while(!(USART1->SR & USART_SR_TXE)); //si DR esta disponible se carga con un nuevo dato
			USART1->DR = r;
			if(r=='\n')		//si el dato recibido es '\n' se termina el bucle de recpción
				fin = 1;
		}
	}
}





