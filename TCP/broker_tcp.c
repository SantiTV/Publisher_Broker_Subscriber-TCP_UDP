#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PUERTO_BROKER 9000
#define MAX_CLIENTES 10
#define TAM_BUFFER 1024
#define TAM_TEMA 50
#define MAX_TEMAS_POR_CLIENTE 10

int sockets_clientes[MAX_CLIENTES] = {0};
char mapa_temas[MAX_CLIENTES][MAX_TEMAS_POR_CLIENTE][TAM_TEMA] = {{{0}}};

// Función que reenvía los mensajes a los suscriptores correctos
void reenviar_mensaje(const char *buffer)
{
    char tema[TAM_TEMA];
    char mensaje[TAM_BUFFER];

    // Se espera el formato: PUB:TEMA:MENSAJE
    if (sscanf(buffer, "PUB:%49[^:]:%[^\n]", tema, mensaje) != 2)
    {
        printf("Broker: Formato de publicación incorrecto.\n");
        return;
    }

    printf("Broker: Mensaje recibido - Tema: %s, Contenido: %s\n", tema, mensaje);

    // Recorremos todos los clientes y enviamos solo a los que estén suscritos al tema
    for (int i = 0; i < MAX_CLIENTES; i++)
    {
        if (sockets_clientes[i] > 0)
        {
            for (int t = 0; t < MAX_TEMAS_POR_CLIENTE; t++)
            {
                if (strcmp(mapa_temas[i][t], tema) == 0)
                {
                    send(sockets_clientes[i], mensaje, strlen(mensaje), 0);
                    printf("Broker: Reenviado a socket %d (tema: %s)\n", sockets_clientes[i], tema);
                    break;
                }
            }
        }
    }
}

// Función para registrar las suscripciones
void manejar_suscripcion(int socket_origen, const char *buffer)
{
    char tema[TAM_TEMA];

    if (sscanf(buffer, "SUB:%49[^\n]", tema) != 1)
    {
        printf("Broker: Formato de suscripción inválido.\n");
        return;
    }

    for (int i = 0; i < MAX_CLIENTES; i++)
    {
        if (sockets_clientes[i] == socket_origen)
        {
            // Verificar si ya estaba suscrito
            for (int t = 0; t < MAX_TEMAS_POR_CLIENTE; t++)
            {
                if (strcmp(mapa_temas[i][t], tema) == 0)
                {
                    printf("Broker: El socket %d ya estaba suscrito a %s\n", socket_origen, tema);
                    return;
                }
            }

            // Guardar el nuevo tema
            for (int t = 0; t < MAX_TEMAS_POR_CLIENTE; t++)
            {
                if (mapa_temas[i][t][0] == '\0')
                {
                    strncpy(mapa_temas[i][t], tema, TAM_TEMA - 1);
                    mapa_temas[i][t][TAM_TEMA - 1] = '\0';
                    printf("Broker: Socket %d suscrito al tema: %s\n", socket_origen, tema);
                    return;
                }
            }

            printf("Broker: El socket %d alcanzó el límite de temas.\n", socket_origen);
            return;
        }
    }
}

int main()
{
    int socket_maestro, nuevo_socket, tam_dir, actividad, i, leidos;
    struct sockaddr_in direccion;
    char buffer[TAM_BUFFER] = {0};
    fd_set lectura;

    socket_maestro = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_maestro == 0)
    {
        perror("Error creando el socket");
        exit(EXIT_FAILURE);
    }

    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(PUERTO_BROKER);

    if (bind(socket_maestro, (struct sockaddr *)&direccion, sizeof(direccion)) < 0)
    {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_maestro, 3) < 0)
    {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    tam_dir = sizeof(direccion);
    printf("Broker iniciado en el puerto %d. Esperando conexiones...\n", PUERTO_BROKER);

    int max_fd = socket_maestro;

    while (1)
    {
        FD_ZERO(&lectura);
        FD_SET(socket_maestro, &lectura);

        for (i = 0; i < MAX_CLIENTES; i++)
        {
            if (sockets_clientes[i] > 0)
            {
                FD_SET(sockets_clientes[i], &lectura);
                if (sockets_clientes[i] > max_fd)
                    max_fd = sockets_clientes[i];
            }
        }

        actividad = select(max_fd + 1, &lectura, NULL, NULL, NULL);
        if ((actividad < 0) && (errno != EINTR))
        {
            perror("Error en select");
        }

        // Nueva conexión entrante
        if (FD_ISSET(socket_maestro, &lectura))
        {
            nuevo_socket = accept(socket_maestro, (struct sockaddr *)&direccion, (socklen_t *)&tam_dir);
            if (nuevo_socket < 0)
            {
                perror("Error en accept");
            }
            else
            {
                for (i = 0; i < MAX_CLIENTES; i++)
                {
                    if (sockets_clientes[i] == 0)
                    {
                        sockets_clientes[i] = nuevo_socket;
                        memset(mapa_temas[i], 0, sizeof(mapa_temas[i]));
                        printf("Broker: Nueva conexión en el socket %d (índice %d)\n", nuevo_socket, i);
                        break;
                    }
                }
            }
        }

        // Revisar si algún cliente envió algo
        for (i = 0; i < MAX_CLIENTES; i++)
        {
            int sd = sockets_clientes[i];
            if (sd > 0 && FD_ISSET(sd, &lectura))
            {
                leidos = read(sd, buffer, TAM_BUFFER);

                if (leidos == 0)
                {
                    close(sd);
                    sockets_clientes[i] = 0;
                    memset(mapa_temas[i], 0, sizeof(mapa_temas[i]));
                    printf("Broker: Conexión cerrada en el socket %d\n", sd);
                }
                else
                {
                    buffer[leidos] = '\0';
                    if (strncmp(buffer, "SUB:", 4) == 0)
                        manejar_suscripcion(sd, buffer);
                    else if (strncmp(buffer, "PUB:", 4) == 0)
                        reenviar_mensaje(buffer);
                    else
                        printf("Broker: Mensaje desconocido desde %d: %s\n", sd, buffer);
                }
            }
        }
    }

    return 0;
}
