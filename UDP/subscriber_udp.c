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
    struct sockaddr_in broker_addr;
    socklen_t tam_broker = sizeof(broker_addr);
    char buffer[TAM_BUFFER];

    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <tema1> [tema2] [tema3] ...\n", argv[0]);
        exit(1);
    }

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
        fprintf(stderr, "Subscriber: No se pudo crear el socket UDP\n");
        return 2;
    }

    printf("Subscriber UDP conectado al broker.\n");

    // Enviar las suscripciones
    for (int i = 1; i < argc; i++)
    {
        snprintf(buffer, sizeof(buffer), "SUB:%s", argv[i]);
        if (sendto(socket_fd, buffer, strlen(buffer), 0, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("Error al enviar suscripción");
        }
        else
        {
            printf("Suscrito al tema: %s\n", argv[i]);
            sleep(1);
        }
    }

    printf("Esperando mensajes del broker...\n");

    // Bucle infinito para recibir mensajes
    while (1)
    {
        int bytes = recvfrom(socket_fd, buffer, TAM_BUFFER - 1, 0,
                             (struct sockaddr *)&broker_addr, &tam_broker);
        if (bytes <= 0)
        {
            perror("Error al recibir mensaje");
            break;
        }

        buffer[bytes] = '\0';
        printf("\n--- NUEVO MENSAJE ---\n");
        printf("%s\n", buffer);
        printf("----------------------\n");
    }

    freeaddrinfo(info);
    close(socket_fd);
    return 0;
}
