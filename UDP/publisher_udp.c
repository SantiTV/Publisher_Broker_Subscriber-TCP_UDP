#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define IP_BROKER "127.0.0.1"
#define PUERTO_BROKER "9000"
#define TAMANIO_BUFFER 1024

int main(void)
{
    int socket_udp;
    struct addrinfo hints, *servinfo, *p;

    // 1. Configuración de la conexión
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // IPv4 o IPv6
    hints.ai_socktype = SOCK_DGRAM; // Socket UDP

    if (getaddrinfo(IP_BROKER, PUERTO_BROKER, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "Error en getaddrinfo\n");
        return 1;
    }

    // 2. Crear el socket UDP
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        socket_udp = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_udp == -1)
            continue; // Intentar con la siguiente dirección

        break; // Socket creado con éxito
    }

    if (p == NULL)
    {
        fprintf(stderr, "Publisher: Falló la creación del socket\n");
        return 2;
    }

    printf("Publisher: Socket UDP creado.\n");

    // 3. Bucle de envío de mensajes
    while (1)
    {
        char tema[50];
        char mensaje[TAMANIO_BUFFER];
        char mensaje_completo[TAMANIO_BUFFER + 50];

        printf("\nIntroduzca TEMA: ");
        if (fgets(tema, sizeof(tema), stdin) == NULL)
            break;
        tema[strcspn(tema, "\n")] = '\0'; // Eliminar salto de línea

        printf("Introduzca MENSAJE: ");
        if (fgets(mensaje, sizeof(mensaje), stdin) == NULL)
            break;
        mensaje[strcspn(mensaje, "\n")] = '\0'; // Eliminar salto de línea

        // Formatear el mensaje
        snprintf(mensaje_completo, sizeof(mensaje_completo), "PUB:%s:%s", tema, mensaje);

        // Enviar el mensaje al broker
        if (sendto(socket_udp, mensaje_completo, strlen(mensaje_completo), 0, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("Error al enviar mensaje");
            break;
        }
        printf("Publisher: Mensaje enviado: %s\n", mensaje_completo);
    }

    freeaddrinfo(servinfo); // Liberar la memoria de las direcciones
    close(socket_udp);      // Cerrar el socket
    return 0;
}