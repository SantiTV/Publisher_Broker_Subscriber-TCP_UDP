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
#define TAM_TEMA 50

// Estructura para representar cada cliente y los temas a los que está suscrito
struct Cliente
{
    struct sockaddr_in direccion;
    char temas[MAX_TEMAS_POR_CLIENTE][TAM_TEMA];
    int num_temas;
};

struct Cliente clientes[MAX_CLIENTES];
int num_clientes = 0;

// Función que guarda una nueva suscripción o actualiza un cliente existente
void manejar_suscripcion(struct sockaddr_in *cliente, const char *buffer)
{
    char tema[TAM_TEMA];

    if (sscanf(buffer, "SUB:%49[^\n]", tema) != 1)
    {
        printf("Broker: Formato de suscripción inválido.\n");
        return;
    }

    // Buscar si el cliente ya existe en la lista
    for (int i = 0; i < num_clientes; i++)
    {
        if (memcmp(&clientes[i].direccion, cliente, sizeof(struct sockaddr_in)) == 0)
        {
            // Si ya está, verificamos si ya tenía este tema
            for (int t = 0; t < clientes[i].num_temas; t++)
            {
                if (strcmp(clientes[i].temas[t], tema) == 0)
                {
                    printf("Broker: El cliente ya estaba suscrito al tema '%s'\n", tema);
                    return;
                }
            }

            // Agregar el nuevo tema
            if (clientes[i].num_temas < MAX_TEMAS_POR_CLIENTE)
            {
                strncpy(clientes[i].temas[clientes[i].num_temas], tema, TAM_TEMA - 1);
                clientes[i].temas[clientes[i].num_temas][TAM_TEMA - 1] = '\0';
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

    // Si no existe, lo agregamos como nuevo cliente
    if (num_clientes < MAX_CLIENTES)
    {
        clientes[num_clientes].direccion = *cliente;
        clientes[num_clientes].num_temas = 1;
        strncpy(clientes[num_clientes].temas[0], tema, TAM_TEMA - 1);
        clientes[num_clientes].temas[0][TAM_TEMA - 1] = '\0';
        num_clientes++;
        printf("Broker: Nuevo cliente suscrito al tema '%s'\n", tema);
    }
    else
    {
        printf("Broker: Límite máximo de clientes alcanzado.\n");
    }
}

// Función que reenvía el mensaje a todos los suscriptores de ese tema
void reenviar_mensaje(const char *buffer, int socket_principal)
{
    char tema[TAM_TEMA];
    char mensaje[TAM_BUFFER];

    // Espera formato PUB:TEMA:MENSAJE
    if (sscanf(buffer, "PUB:%49[^:]:%[^\n]", tema, mensaje) != 2)
    {
        printf("Broker: Formato de publicación inválido.\n");
        return;
    }

    printf("Broker: Publicación recibida - Tema: %s | Mensaje: %s\n", tema, mensaje);

    // Buscar los clientes suscritos al tema y enviarles el mensaje
    for (int i = 0; i < num_clientes; i++)
    {
        for (int t = 0; t < clientes[i].num_temas; t++)
        {
            if (strcmp(clientes[i].temas[t], tema) == 0)
            {
                sendto(socket_principal, mensaje, strlen(mensaje), 0,
                       (struct sockaddr *)&clientes[i].direccion,
                       sizeof(clientes[i].direccion));
                printf("Broker: Enviado a cliente suscrito al tema '%s'\n", tema);
            }
        }
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in servidor, cliente;
    socklen_t tam_cliente = sizeof(cliente);
    char buffer[TAM_BUFFER];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error al crear socket UDP");
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

    printf("Broker UDP activo en el puerto %d...\n", PUERTO_BROKER);

    while (1)
    {
        int bytes = recvfrom(sockfd, buffer, TAM_BUFFER - 1, 0,
                             (struct sockaddr *)&cliente, &tam_cliente);

        if (bytes < 0)
        {
            perror("Error al recibir datos");
            continue;
        }

        buffer[bytes] = '\0';

        if (strncmp(buffer, "SUB:", 4) == 0)
        {
            manejar_suscripcion(&cliente, buffer);
        }
        else if (strncmp(buffer, "PUB:", 4) == 0)
        {
            reenviar_mensaje(buffer, sockfd);
        }
        else
        {
            printf("Broker: Comando desconocido: %s\n", buffer);
        }
    }

    close(sockfd);
    return 0;
}
