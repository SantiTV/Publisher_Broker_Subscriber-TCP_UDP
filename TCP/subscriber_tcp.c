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
    char buffer[BUFFER_SIZE];
    char subscription_msg[60];

    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <TEMA1> [TEMA2] [TEMA3] ...\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(BROKER_IP, BROKER_PORT, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "Error getaddrinfo\n");
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1)
            break;
        close(sockfd);
    }

    if (p == NULL)
    {
        fprintf(stderr, "Subscriber: Falló la conexión con el Broker\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    printf("Subscriber: Conectado al Broker.\n");

    // Suscribirse a todos los temas dados
    for (int i = 1; i < argc; i++)
    {
        snprintf(subscription_msg, sizeof(subscription_msg), "SUB:%s", argv[i]);
        if (send(sockfd, subscription_msg, strlen(subscription_msg), 0) == -1)
        {
            perror("Send suscripción error");
            close(sockfd);
            return 3;
        }
        printf("Subscriber: Suscrito al tema: %s\n", argv[i]);
        sleep(1);
    }

    printf("Esperando mensajes de los temas suscritos...\n");

    while (1)
    {
        int numbytes;
        if ((numbytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) <= 0)
        {
            if (numbytes == 0)
                printf("Subscriber: Conexión cerrada por el Broker.\n");
            else
                perror("Recv error");
            break;
        }

        buffer[numbytes] = '\0';
        printf("\n--- NUEVA ACTUALIZACIÓN ---\n");
        printf("%s\n", buffer);
        printf("---------------------------\n");
    }

    close(sockfd);
    return 0;
}
