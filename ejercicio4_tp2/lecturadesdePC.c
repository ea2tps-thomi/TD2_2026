#include <stdio.h>      // printf(), perror()
#include <stdlib.h>     // exit()
#include <fcntl.h>      // open()
#include <unistd.h>     // read(), close()
#include <termios.h>    // configuración UART Linux
#include <signal.h>     // Ctrl+C (SIGINT)

/* programa ejemplo de lectura del puerto serie ttyUSB0 desde PC con Linux
 * compilar gcc lecturadesdePC.c -o lecturaPC
 * ejecutar ./lecturaPC
*/

#define SERIAL_PORT "/dev/ttyUSB1"

int fd;                         // descriptor del puerto serie
struct termios oldtty;         // configuración original

void cerrar_programa(int sig)
{
    tcsetattr(fd, TCSANOW, &oldtty);  // restaurar configuración
    close(fd);                        // cerrar puerto
    printf("\nPuerto cerrado.\n");
    exit(0);
}

int main(void)
{
    struct termios tty;   // nueva configuración UART
    char dato;            // byte recibido

    signal(SIGINT, cerrar_programa); // Ctrl+C -> cerrar_programa()

    fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY); // abrir UART

    if (fd < 0)
    {
        perror("No se pudo abrir el puerto serie");
        return 1;
    }

    tcgetattr(fd, &oldtty); // leer config actual
    tty = oldtty;           // copiar configuración base

    cfmakeraw(&tty);        // modo raw, sin eco ni procesamiento

    cfsetispeed(&tty, B9600); // RX a 9600 baudios
    cfsetospeed(&tty, B9600); // TX a 9600 baudios

    tty.c_cflag |= CLOCAL | CREAD; // habilitar recepción
    tty.c_cflag &= ~PARENB;        // sin paridad
    tty.c_cflag &= ~CSTOPB;        // 1 bit stop
    tty.c_cflag &= ~CSIZE;         // limpiar tamaño previo
    tty.c_cflag |= CS8;            // 8 bits de datos

    tty.c_lflag &= ~ECHO;          // desactivar eco

    tty.c_cc[VMIN]  = 1;           // esperar 1 byte
    tty.c_cc[VTIME] = 0;           // sin timeout

    tcflush(fd, TCIFLUSH);         // limpiar buffer RX
    tcsetattr(fd, TCSANOW, &tty);  // aplicar configuración

    printf("Leyendo desde %s a 9600 8N1\n", SERIAL_PORT);
    printf("Ctrl+C para salir.\n\n");

    while (1)
    {
        if (read(fd, &dato, 1) == 1) // leer 1 byte
        {
            putchar(dato);            // imprimir byte
            fflush(stdout);           // mostrar inmediatamente
        }
    }

    return 0;
}
