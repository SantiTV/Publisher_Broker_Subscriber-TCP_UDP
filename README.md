# Publisher_Broker_Subscriber-TCP_UDP

---

#  Justificación y Documentación de la Interacción con la API de Sockets

Cada función de la API de Sockets interactúa directamente con el kernel para realizar una tarea específica:

Función: socket()
El programa utiliza la función socket(AF_INET, SOCK_DGRAM, 0) una sola vez al inicio del main(). Esta llamada solicita al kernel la creación de un nuevo endpoint de comunicación de tipo UDP (SOCK_DGRAM) en el dominio IPv4 (AF_INET). El kernel asigna y devuelve un descriptor de archivo, almacenado en la variable socket_principal, que representa el punto de comunicación.

Función: htons()
La función htons(PUERTO_BROKEN) se utiliza para garantizar el orden de bytes de red. El programa la aplica al número de puerto local (PUERTO_BROKEN) antes de la configuración de la dirección. Esto asegura que el puerto se almacene correctamente en el formato estándar de la red, independientemente de la arquitectura de la máquina (little-endian o big-endian).

Función: bind()
El broker llama a bind() una vez, pasándole el descriptor socket_principal y la dirección configurada. Esta llamada al sistema asocia el socket con la IP local (INADDR_ANY) y el puerto (9000), convirtiendo formalmente al programa en un servidor de escucha para ese puerto específico.

Función: recvfrom()
Esta función se ejecuta de manera repetitiva dentro del loop principal. El kernel bloquea la ejecución del programa hasta que llega un datagrama UDP al puerto asignado. Una vez recibido, recvfrom() no solo extrae los datos al buffer, sino que, crucialmente para UDP, también almacena la dirección IP y el puerto completo del cliente emisor (&cliente), permitiendo que el broker sepa a quién debe responder o registrar.

Función: sendto()
La función sendto() se utiliza dentro de redistribuir_mensaje() para reenviar las publicaciones. El programa especifica el mensaje (mensaje_a_enviar) y la dirección de destino completa (obtenida previamente por recvfrom durante la suscripción). El kernel es responsable de construir el paquete UDP completo y delegar su transmisión a través de la tarjeta de red.

Función: close()
Aunque el broker está diseñado para un loop infinito, close(socket_principal) es esencial en las secciones de manejo de errores o en la terminación controlada del programa. Esta llamada al kernel libera el descriptor de archivo del socket y el puerto asociado, devolviendo los recursos al sistema operativo.