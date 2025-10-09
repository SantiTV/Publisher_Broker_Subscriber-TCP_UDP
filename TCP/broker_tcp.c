#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BROKER_PORT 9000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_TOPIC_LEN 50

// Array para mapear el socket (índice) al tema.
// 0 o -1 en el socket indica slot libre.
int client_sockets[MAX_CLIENTS] = {0};
char topic_map[MAX_CLIENTS][MAX_TOPIC_LEN] = {{0}};

// --- Función lógica de redistribución ---
void redistribute_message(const char *buffer)
{
    char topic[MAX_TOPIC_LEN];
    char message_to_send[BUFFER_SIZE];

    // Asumimos el formato es 'PUB:TEMA:MENSAJE'.
    // Usamos sscanf para parsear
    if (sscanf(buffer, "PUB:%49[^:]:%[^\n]", topic, message_to_send) != 2)
    {
        printf("Broker: Formato de publicación inválido.\n");
        return;
    }

    printf("Broker: Publicación recibida - TEMA: %s, MENSAJE: %s\n", topic, message_to_send);

    // Iterar sobre todos los clientes y reenviar si el tema coincide.
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] > 0 && strcmp(topic_map[i], topic) == 0)
        {
            printf("Broker: Reenviando a socket %d\n", client_sockets[i]);
            // Reenvío solo el mensaje (sin PUB:TEMA:)
            send(client_sockets[i], message_to_send, strlen(message_to_send), 0);
        }
    }
}

// --- Función para la lógica de suscripción ---
void handle_subscription(int sender_socket, const char *buffer)
{
    char topic[MAX_TOPIC_LEN];

    // Asumimos el formato es 'SUB:TEMA'.
    if (sscanf(buffer, "SUB:%49[^\n]", topic) != 1)
    {
        printf("Broker: Formato de suscripción inválido.\n");
        return;
    }

    // Encontrar el índice del socket y registrar la suscripción
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == sender_socket)
        {
            strncpy(topic_map[i], topic, MAX_TOPIC_LEN - 1);
            topic_map[i][MAX_TOPIC_LEN - 1] = '\0';
            printf("Broker: Socket %d SUSCRITO al tema: %s\n", sender_socket, topic);
            return;
        }
    }
    printf("Broker: Error, socket %d no encontrado para registrar suscripción.\n", sender_socket);
}

int main()
{
    int master_socket, new_socket, addrlen, activity, i, valread;
    struct sockaddr_in address;

    char buffer[BUFFER_SIZE] = {0};
    fd_set readfds;

    // 1. Crear el socket maestro (listener)
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Configuración de dirección
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(BROKER_PORT);

    // 2. Bind y Listen
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(master_socket, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    printf("Broker iniciado en el puerto %d. Esperando conexiones...\n", BROKER_PORT);

    int fdmax = master_socket; // Descripto de socket máximo

    // 3. Bucle de manejo de conexiones (usando select)
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);

        // Añadir sockets de clientes al conjunto
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_sockets[i] > 0)
            {
                FD_SET(client_sockets[i], &readfds);
                if (client_sockets[i] > fdmax)
                    fdmax = client_sockets[i];
            }
        }

        // Esperar actividad
        activity = select(fdmax + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            perror("Select error");
        }

        // Si hay actividad en el socket maestro (nueva conexión)
        if (FD_ISSET(master_socket, &readfds))
        {
            new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0)
            {
                perror("Accept error");
            }
            else
            {
                // Añadir el nuevo socket al array de clientes
                for (i = 0; i < MAX_CLIENTS; i++)
                {
                    if (client_sockets[i] == 0)
                    {
                        client_sockets[i] = new_socket;
                        topic_map[i][0] = '\0'; // Limpiar suscripción
                        printf("Broker: Nueva conexión en el socket %d, índice %d\n", new_socket, i);
                        break;
                    }
                }
            }
        }

        // Manejar la actividad en los sockets de clientes
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds))
            {
                valread = read(sd, buffer, BUFFER_SIZE);

                if (valread == 0)
                {
                    // Desconexión
                    close(sd);
                    client_sockets[i] = 0;
                    topic_map[i][0] = '\0';
                    printf("Broker: Conexión cerrada en el socket %d\n", sd);
                }
                else
                {
                    buffer[valread] = '\0';

                    if (strncmp(buffer, "SUB:", 4) == 0)
                    {
                        handle_subscription(sd, buffer);
                    }
                    else if (strncmp(buffer, "PUB:", 4) == 0)
                    {
                        redistribute_message(buffer);
                    }
                    else
                    {
                        printf("Broker: Mensaje desconocido de %d: %s\n", sd, buffer);
                    }
                }
            }
        }
    }
    return 0;
}