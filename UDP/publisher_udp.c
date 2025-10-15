#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define IP_BROKER "127.0.0.1"
#define PUERTO_BROKER "9000"
#define TAM_BUFFER 1024

int main(void)
{
    int socket_fd;
    struct addrinfo hints, *info, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(IP_BROKER, PUERTO_BROKER, &hints, &info) != 0)
    {
        fprintf(stderr, "Error en getaddrinfo\n");
        return 1;
    }

    for (p = info; p != NULL; p = p->ai_next)
    {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd != -1)
            break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "Publisher: No se pudo crear el socket UDP\n");
        return 2;
    }

    printf("Publisher UDP listo para enviar mensajes.\n");

    while (1)
    {
        char tema[50];
        char mensaje[TAM_BUFFER];
        char mensaje_completo[TAM_BUFFER + 50];

        printf("\nTema: ");
        if (fgets(tema, sizeof(tema), stdin) == NULL)
            break;
        tema[strcspn(tema, "\n")] = '\0';

        printf("Mensaje: ");
        if (fgets(mensaje, sizeof(mensaje), stdin) == NULL)
            break;
        mensaje[strcspn(mensaje, "\n")] = '\0';

        snprintf(mensaje_completo, sizeof(mensaje_completo), "PUB:%s:%s", tema, mensaje);

        if (sendto(socket_fd, mensaje_completo, strlen(mensaje_completo), 0,
                   p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("Error al enviar mensaje");
            break;
        }

        printf("Mensaje enviado al broker: %s\n", mensaje_completo);
    }

    freeaddrinfo(info);
    close(socket_fd);
    return 0;
}
