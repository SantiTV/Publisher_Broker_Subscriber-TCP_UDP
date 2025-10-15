#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
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
    int socketfd;
    struct addrinfo hints, *servinfo, *p;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in broker_addr;
    socklen_t addr_len = sizeof(broker_addr);

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <TEMA_A_SUSCRIBIRSE>\n", argv[0]);
        exit(1);
    }
    const char *topic = argv[1];

    // 1. Configuración de la conexión
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // IPv4 o IPv6
    hints.ai_socktype = SOCK_DGRAM; // Socket UDP

    if (getaddrinfo(BROKER_IP, BROKER_PORT, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "Error en getaddrinfo\n");
        return 1;
    }

    // 2. Crear el socket UDP
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socketfd == -1)
            continue; // Intentar con la siguiente dirección

        break; // Socket creado con éxito
    }

    if (p == NULL)
    {
        fprintf(stderr, "Subscriber: Falló la creación del socket\n");
        return 2;
    }

    printf("Subscriber: Socket UDP creado.\n");

    // 3. Enviar mensaje de suscripción: SUB:TEMA
    snprintf(buffer, sizeof(buffer), "SUB:%s", topic);
    if (sendto(socketfd, buffer, strlen(buffer), 0, p->ai_addr, p->ai_addrlen) == -1)
    {
        perror("Error al enviar mensaje de suscripción");
        freeaddrinfo(servinfo);
        close(socketfd);
        return 3;
    }
    printf("Subscriber: Suscrito al tema: %s\n", topic);

    freeaddrinfo(servinfo); // Liberar la memoria de las direcciones

    // 4. Bucle de recepción de mensajes
    while (1)
    {
        int bytes_received = recvfrom(socketfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&broker_addr, &addr_len);
        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
                printf("Subscriber: Conexión cerrada por el Broker.\n");
            else
                perror("Error al recibir mensaje");
            break;
        }
        buffer[bytes_received] = '\0'; // Asegurar terminación nula
        printf("\n--- ACTUALIZACIÓN (%s) ---\n", topic);
        printf("%s\n", buffer);
        printf("---------------------------\n");
    }

    close(socketfd);
    return 0;
}