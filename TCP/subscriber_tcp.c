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

int main(int argc, char *argv[])
{
    int socket_fd;
    struct addrinfo hints, *info, *p;
    char buffer[TAM_BUFFER];
    char mensaje_subs[60];

    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <tema1> [tema2] [tema3] ...\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(IP_BROKER, PUERTO_BROKER, &hints, &info) != 0)
    {
        fprintf(stderr, "Error en getaddrinfo\n");
        return 1;
    }

    for (p = info; p != NULL; p = p->ai_next)
    {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd == -1)
            continue;
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) != -1)
            break;
        close(socket_fd);
    }

    if (p == NULL)
    {
        fprintf(stderr, "Subscriber: No se pudo conectar con el broker\n");
        return 2;
    }

    freeaddrinfo(info);
    printf("Subscriber conectado al broker.\n");

    // Enviar todas las suscripciones
    for (int i = 1; i < argc; i++)
    {
        snprintf(mensaje_subs, sizeof(mensaje_subs), "SUB:%s", argv[i]);
        if (send(socket_fd, mensaje_subs, strlen(mensaje_subs), 0) == -1)
        {
            perror("Error al enviar suscripción");
            close(socket_fd);
            return 3;
        }
        printf("Suscrito al tema: %s\n", argv[i]);
        sleep(1);
    }

    printf("Esperando mensajes...\n");

    // Escucha permanente
    while (1)
    {
        int bytes_recibidos = recv(socket_fd, buffer, TAM_BUFFER - 1, 0);
        if (bytes_recibidos <= 0)
        {
            if (bytes_recibidos == 0)
                printf("Conexión cerrada por el broker.\n");
            else
                perror("Error al recibir mensaje");
            break;
        }

        buffer[bytes_recibidos] = '\0';
        printf("\n--- NUEVO MENSAJE ---\n");
        printf("%s\n", buffer);
        printf("----------------------\n");
    }

    close(socket_fd);
    return 0;
}
