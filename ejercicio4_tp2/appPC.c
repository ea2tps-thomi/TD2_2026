#include <stdio.h>      // printf(), perror(), scanf()
#include <stdlib.h>     // exit()
#include <fcntl.h>      // open()
#include <unistd.h>     // read(), write(), close()
#include <termios.h>    // configuración UART Linux
#include <signal.h>     // Gestión de señales (Ctrl+C)
#include <string.h>     // strlen()

#define SERIAL_PORT "/dev/ttyUSB0"
#define SERIAL_PORT2 "/dev/ttyUSB1"

int fd;                         // descriptor del puerto serie
struct termios oldtty;          // configuración original
volatile sig_atomic_t salir_escucha = 0; // Bandera para controlar el bucle de escucha

// Manejador de señales para capturar el Ctrl+C
void controlador_sigint(int sig)
{
    // Si estamos en modo escucha, esta bandera romperá el bucle
    salir_escucha = 1; 
}

void restaurar_y_cerrar(void)
{
    tcsetattr(fd, TCSANOW, &oldtty);  // restaurar configuración original
    close(fd);                        // cerrar puerto
    printf("\nPuerto serial cerrado correctamente. ¡Adiós!\n");
}

void modo_escucha(void)
{
    char dato;
    salir_escucha = 0;
    
    // Asignamos temporalmente el Ctrl+C para salir del modo escucha, no del programa
    signal(SIGINT, controlador_sigint);

    printf("\n--- MODO ESCUCHA ACTIVO ---\n");
    printf("Esperando datos... (Presiona Ctrl+C para volver al menú)\n\n");

    // Limpiamos buffers antes de empezar a escuchar
    tcflush(fd, TCIFLUSH);

    while (!salir_escucha)
    {
        // Con VMIN=0 y VTIME=1, read() no se congela indefinidamente.
        // Si pasan 100ms sin datos, retorna 0, permitiendo evaluar la bandera salir_escucha.
        if (read(fd, &dato, 1) > 0) 
        {
            putchar(dato);            // Imprime el carácter recibido
            fflush(stdout);           // Muestra en pantalla inmediatamente
        }
    }

    // Reestablecemos la señal por defecto para el resto del programa
    signal(SIGINT, SIG_DFL);
    printf("\n--- Saliendo del Modo Escucha ---\n");
}

void modo_envio(void)
{
    int opcion_envio;
    
    while (1)
    {
        printf("\n--- MODO ENVÍO ---\n");
        printf("1. Enviar comando OFF\n");
        printf("2. Enviar comando ON\n");
        printf("3. Volver al menú principal\n");
        printf("Selecciona una opción: ");
        
        if (scanf("%d", &opcion_envio) != 1) {
            // Limpiar el buffer de entrada en caso de que pongan una letra
            while (getchar() != '\n');
            continue;
        }

        // Restaurado a tu lógica original exacta
        if (opcion_envio == 1)
        {
            write(fd, "ON\n", 3); 
            printf("[Enviado] -> OFF\n");
        }
        else if (opcion_envio == 2)
        {
            write(fd, "OFF\n", 4);
            printf("[Enviado] -> ON\n");
        }
        else if (opcion_envio == 3)
        {
            break; // Rompe el ciclo y vuelve al menú principal
        }
        else
        {
            printf("Opción no válida.\n");
        }
    }
}

int main(void)
{
    struct termios tty;   // nueva configuración UART
    int opcion_menu;

    fd = open("/dev/ttyUSB1", O_RDWR | O_NOCTTY); 
    if (fd >= 0) {
        printf("[OK] Conectado exitosamente a /dev/ttyUSB1\n");
    } else {
        // Intento 2: Si falló el 1, probar con ttyUSB0
        fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
        if (fd >= 0) {
            printf("[OK] Conectado exitosamente a /dev/ttyUSB0\n");
        } else {
            // Ambos fallaron
            perror("Error: No se encontró un dispositivo en /dev/ttyUSB0 ni en /dev/ttyUSB1");
            return 1;
        }
    }

    tcgetattr(fd, &oldtty); // Guardar config actual para restaurarla al final
    tty = oldtty;           

    cfmakeraw(&tty);        // modo raw, sin eco ni procesamiento

    cfsetispeed(&tty, B9600); // RX a 9600 baudios
    cfsetospeed(&tty, B9600); // TX a 9600 baudios

    tty.c_cflag |= CLOCAL | CREAD; // habilitar recepción y modo local
    tty.c_cflag &= ~PARENB;        // sin paridad
    tty.c_cflag &= ~CSTOPB;        // 1 bit stop
    tty.c_cflag &= ~CSIZE;         // limpiar tamaño previo
    tty.c_cflag |= CS8;            // 8 bits de datos

    tty.c_lflag &= ~ECHO;          // desactivar eco en el puerto

    // --- Mantiene la respuesta instantánea a Ctrl+C ---
    tty.c_cc[VMIN]  = 0;           // No bloquear exigiendo bytes mínimos
    tty.c_cc[VTIME] = 1;           // Timeout de 1 décima de segundo (100 ms)

    tcsetattr(fd, TCSANOW, &tty);  // aplicar configuración de termios

    // Bucle del Menú Principal
    while (1)
    {
        printf("\n====================================\n");
        printf("       MENÚ CONTROL SERIAL      \n");
        printf("====================================\n");
        printf("1. Entrar en Modo Escucha (Recibir)\n");
        printf("2. Entrar en Modo Envío (Mandar ON/OFF)\n");
        printf("3. Salir del programa\n");
        printf("Selecciona una opción: ");

        if (scanf("%d", &opcion_menu) != 1) {
            while (getchar() != '\n'); // Limpiar entrada errónea
            printf("Por favor, ingresa un número válido.\n");
            continue;
        }

        switch (opcion_menu)
        {
            case 1:
                modo_escucha();
                break;
            case 2:
                modo_envio();
                break;
            case 3:
                restaurar_y_cerrar();
                return 0;
            default:
                printf("Opción incorrecta, intenta de nuevo.\n");
        }
    }

    return 0;
}
