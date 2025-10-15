#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PUERTO_BROKEN 9000
#define MAX_CLIENTES 10
#define TAMANIO_BUFFER 1024
#define MAX_TEMAS 50

// Estructura para almacenar las suscripciones
struct suscripcion
{
    struct sockaddr_in direccion_cliente;
    char tema[MAX_TEMAS];
};

struct suscripcion suscripciones[MAX_CLIENTES] = {0}; // Arreglo de suscripciones
int num_suscripciones = 0;                            // Número actual de suscripciones

void redistribuir_mensaje(const char *buffer, int socket_principal)
{
    char tema[MAX_TEMAS];
    char mensaje_a_enviar[TAMANIO_BUFFER];

    // Parsear el mensaje en formato 'PUB:TEMA:MENSAJE'
    if (sscanf(buffer, "PUB:%49[^:]:%[^\n]", tema, mensaje_a_enviar) != 2)
    {
        printf("Broker: Formato de publicación inválido.\n");
        return;
    }

    printf("Broker: Publicación recibida - TEMA: %s, MENSAJE: %s\n", tema, mensaje_a_enviar);

    // Enviar el mensaje a todos los clientes suscritos al tema
    for (int i = 0; i < num_suscripciones; i++)
    {
        if (strcmp(suscripciones[i].tema, tema) == 0)
        {
            if (sendto(socket_principal, mensaje_a_enviar, strlen(mensaje_a_enviar), 0,
                       (struct sockaddr *)&suscripciones[i].direccion_cliente, sizeof(suscripciones[i].direccion_cliente)) == -1)
            {
                perror("Broker: Error al enviar mensaje");
            }
            else
            {
                printf("Broker: Mensaje enviado a cliente suscrito al tema '%s'.\n", tema);
            }
        }
    }
}

void manejar_suscripcion(struct sockaddr_in *cliente, const char *buffer)
{
    char tema[MAX_TEMAS];

    // Parsear el mensaje en formato 'SUB:TEMA'
    if (sscanf(buffer, "SUB:%49[^\n]", tema) != 1)
    {
        printf("Broker: Formato de suscripción inválido.\n");
        return;
    }

    // Verificar si el cliente ya está suscrito
    for (int i = 0; i < num_suscripciones; i++)
    {
        if (memcmp(&suscripciones[i].direccion_cliente, cliente, sizeof(struct sockaddr_in)) == 0)
        {
            // Actualizar el tema si ya está suscrito
            strncpy(suscripciones[i].tema, tema, MAX_TEMAS - 1);
            suscripciones[i].tema[MAX_TEMAS - 1] = '\0'; // Asegurar terminación nula
            printf("Broker: Cliente actualizado al tema: %s\n", tema);
            return;
        }
    }

    // Registrar una nueva suscripción
    if (num_suscripciones < MAX_CLIENTES)
    {
        suscripciones[num_suscripciones].direccion_cliente = *cliente;
        strncpy(suscripciones[num_suscripciones].tema, tema, MAX_TEMAS - 1);
        suscripciones[num_suscripciones].tema[MAX_TEMAS - 1] = '\0'; // Asegurar terminación nula
        num_suscripciones++;
        printf("Broker: Nueva suscripción registrada - TEMA: %s\n", tema);
    }
    else
    {
        printf("Broker: Límite de suscripciones alcanzado.\n");
    }
}

int main()
{
    int socket_principal, valread;
    struct sockaddr_in direccion, cliente;
    socklen_t cliente_len = sizeof(cliente);

    char buffer[TAMANIO_BUFFER] = {0};

    // Crear socket principal (UDP)
    socket_principal = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_principal < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del socket
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(PUERTO_BROKEN);

    // Asociar el socket con la dirección y el puerto
    if (bind(socket_principal, (struct sockaddr *)&direccion, sizeof(direccion)) < 0)
    {
        perror("Bind failed");
        close(socket_principal);
        exit(EXIT_FAILURE);
    }

    printf("Broker UDP escuchando en el puerto %d\n", PUERTO_BROKEN);

    // Bucle principal
    while (1)
    {
        // Recibir mensaje del cliente
        valread = recvfrom(socket_principal, buffer, TAMANIO_BUFFER, 0, (struct sockaddr *)&cliente, &cliente_len);
        if (valread < 0)
        {
            perror("Error al recibir datos");
            continue;
        }

        buffer[valread] = '\0'; // Asegurar terminación nula
        printf("Broker: Mensaje recibido: %s\n", buffer);

        // Determinar si es suscripción o publicación
        if (strncmp(buffer, "SUB:", 4) == 0)
        {
            manejar_suscripcion(&cliente, buffer);
        }
        else if (strncmp(buffer, "PUB:", 4) == 0)
        {
            redistribuir_mensaje(buffer, socket_principal);
        }
        else
        {
            printf("Broker: Comando desconocido: %s\n", buffer);
        }
    }

    close(socket_principal);
    return 0;
}