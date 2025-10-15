#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BROKER_IP "127.0.0.1"
#define BROKER_PORT "9000"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in broker_addr;
    socklen_t addr_len = sizeof(broker_addr);
    char buffer[BUFFER_SIZE];

    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <TEMA1> [TEMA2] ...\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(BROKER_IP, BROKER_PORT, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "Error en getaddrinfo\n");
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd != -1)
            break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "Subscriber: Falló la creación del socket\n");
        return 2;
    }

    printf("Subscriber UDP iniciado.\n");

    // Suscribirse a todos los temas
    for (int i = 1; i < argc; i++)
    {
        snprintf(buffer, sizeof(buffer), "SUB:%s", argv[i]);
        if (sendto(sockfd, buffer, strlen(buffer), 0, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("Error al enviar suscripción");
        }
        else
        {
            printf("Subscriber: Suscrito al tema: %s\n", argv[i]);
            sleep(1);
        }
    }

    printf("Esperando mensajes...\n");

    // Recibir mensajes de cualquier tema
    while (1)
    {
        int bytes = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&broker_addr, &addr_len);
        if (bytes <= 0)
        {
            perror("Error al recibir mensaje");
            break;
        }

        buffer[bytes] = '\0';
        printf("\n--- NUEVA ACTUALIZACIÓN ---\n");
        printf("%s\n", buffer);
        printf("---------------------------\n");
    }

    freeaddrinfo(servinfo);
    close(sockfd);
    return 0;
}
