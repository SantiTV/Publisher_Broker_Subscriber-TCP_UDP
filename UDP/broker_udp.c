#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PUERTO_BROKER 9000
#define MAX_CLIENTES 10
#define MAX_TEMAS_POR_CLIENTE 10
#define TAM_BUFFER 1024
#define MAX_TEMA 50

struct Cliente
{
    struct sockaddr_in direccion;
    char temas[MAX_TEMAS_POR_CLIENTE][MAX_TEMA];
    int num_temas;
};

struct Cliente clientes[MAX_CLIENTES];
int num_clientes = 0;

// --- Registrar o actualizar suscripción ---
void manejar_suscripcion(struct sockaddr_in *cliente, const char *buffer)
{
    char tema[MAX_TEMA];
    if (sscanf(buffer, "SUB:%49[^\n]", tema) != 1)
    {
        printf("Broker: Formato de suscripción inválido.\n");
        return;
    }

    // Buscar si el cliente ya existe
    for (int i = 0; i < num_clientes; i++)
    {
        if (memcmp(&clientes[i].direccion, cliente, sizeof(struct sockaddr_in)) == 0)
        {
            // Ya existe → registrar tema nuevo si no lo tiene
            for (int t = 0; t < clientes[i].num_temas; t++)
            {
                if (strcmp(clientes[i].temas[t], tema) == 0)
                {
                    printf("Broker: Cliente ya estaba suscrito a %s\n", tema);
                    return;
                }
            }

            if (clientes[i].num_temas < MAX_TEMAS_POR_CLIENTE)
            {
                strncpy(clientes[i].temas[clientes[i].num_temas], tema, MAX_TEMA - 1);
                clientes[i].temas[clientes[i].num_temas][MAX_TEMA - 1] = '\0';
                clientes[i].num_temas++;
                printf("Broker: Cliente existente suscrito a nuevo tema: %s\n", tema);
            }
            else
            {
                printf("Broker: Cliente alcanzó el límite de temas.\n");
            }
            return;
        }
    }

    // Si el cliente no existía, registrarlo
    if (num_clientes < MAX_CLIENTES)
    {
        clientes[num_clientes].direccion = *cliente;
        clientes[num_clientes].num_temas = 1;
        strncpy(clientes[num_clientes].temas[0], tema, MAX_TEMA - 1);
        clientes[num_clientes].temas[0][MAX_TEMA - 1] = '\0';
        num_clientes++;
        printf("Broker: Nuevo cliente suscrito al tema %s\n", tema);
    }
    else
    {
        printf("Broker: Límite de clientes alcanzado.\n");
    }
}

// --- Reenviar mensajes a todos los suscriptores del tema ---
void redistribuir_mensaje(const char *buffer, int socket_principal)
{
    char tema[MAX_TEMA];
    char mensaje[TAM_BUFFER];

    if (sscanf(buffer, "PUB:%49[^:]:%[^\n]", tema, mensaje) != 2)
    {
        printf("Broker: Formato de publicación inválido.\n");
        return;
    }

    printf("Broker: Publicación recibida - TEMA: %s, MENSAJE: %s\n", tema, mensaje);

    for (int i = 0; i < num_clientes; i++)
    {
        for (int t = 0; t < clientes[i].num_temas; t++)
        {
            if (strcmp(clientes[i].temas[t], tema) == 0)
            {
                if (sendto(socket_principal, mensaje, strlen(mensaje), 0,
                           (struct sockaddr *)&clientes[i].direccion, sizeof(clientes[i].direccion)) == -1)
                {
                    perror("Broker: Error al enviar mensaje");
                }
                else
                {
                    printf("Broker: Mensaje reenviado a cliente (%s)\n", tema);
                }
            }
        }
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in servidor, cliente;
    socklen_t cliente_len = sizeof(cliente);
    char buffer[TAM_BUFFER];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error creando socket UDP");
        exit(EXIT_FAILURE);
    }

    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons(PUERTO_BROKER);

    if (bind(sockfd, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
        perror("Error en bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Broker UDP escuchando en el puerto %d...\n", PUERTO_BROKER);

    while (1)
    {
        int bytes = recvfrom(sockfd, buffer, TAM_BUFFER - 1, 0, (struct sockaddr *)&cliente, &cliente_len);
        if (bytes < 0)
        {
            perror("Error al recibir datos");
            continue;
        }

        buffer[bytes] = '\0';

        if (strncmp(buffer, "SUB:", 4) == 0)
            manejar_suscripcion(&cliente, buffer);
        else if (strncmp(buffer, "PUB:", 4) == 0)
            redistribuir_mensaje(buffer, sockfd);
        else
            printf("Broker: Comando desconocido: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
