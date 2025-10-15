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

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;

    // 1. Configuración de la conexión
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(BROKER_IP, BROKER_PORT, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "Error getaddrinfo\n");
        return 1;
    }

    // 2. Conectar al Broker
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
        fprintf(stderr, "Publisher: Falló la conexión con el Broker\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    printf("Publisher: Conectado al Broker.\n");

    // 3. Bucle de envío de mensajes
    while (1)
    {
        char topic[50];
        char message[BUFFER_SIZE];
        char full_msg[BUFFER_SIZE + 50];

        printf("\nIntroduzca TEMA: ");
        if (fgets(topic, sizeof(topic), stdin) == NULL)
            break;
        topic[strcspn(topic, "\n")] = '\0';

        printf("Introduzca MENSAJE: ");
        if (fgets(message, sizeof(message), stdin) == NULL)
            break;
        message[strcspn(message, "\n")] = '\0';

        // Formato de Publicación: PUB:TEMA:MENSAJE
        snprintf(full_msg, sizeof(full_msg), "PUB:%s:%s", topic, message);

        if (send(sockfd, full_msg, strlen(full_msg), 0) == -1)
        {
            perror("Send error");
            break;
        }
        printf("Publisher: Mensaje enviado: \"%s\"\n", full_msg);
        }

    close(sockfd);
    return 0;
}