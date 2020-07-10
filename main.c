#include <stdio.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <zconf.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PATH "/tmp/socc"

char firmware_version[20] = "1.0";
int retry_time=1;

int fd_set_blocking(int fd, int blocking);


/**
 * @brief Programa cliente. Tiene como objetivo simular el firmaware del satelite que se conecta al programa servidor (Base Terrestre) y queda a la espera de ordenes del mismo.
 * @return
 */
int main(int argc, char *argv[]) {

    printf("DEBUG: ejecutando main \n");
    printf("Version del firmware: %s\n", firmware_version);
    int sockfd;
    //int ip_srv_load;
    struct sockaddr_un dest_addr;

    char buffer_recepcion[BUFFER_SIZE];

    //crer socket
    sockfd= socket(AF_UNIX, SOCK_STREAM, 0);
    int setting_ok;
    setting_ok = fd_set_blocking(sockfd,1);

    if (sockfd <0){
        perror("error al abrir el socket cliente");
    }

    //setear la estructura sockaddr
    memset(&dest_addr, 0, sizeof(dest_addr) ); //limpieza de la estructura
    dest_addr.sun_family= AF_UNIX;
    //dest_addr.sun_path= Hay que cargarlo, no se puede asignar

    if (argv[1] != NULL){
        strncpy(dest_addr.sun_path, argv[1], sizeof(dest_addr.sun_path));
    }
    else{
        char *path=DEFAULT_PATH;
        strncpy(dest_addr.sun_path, path, sizeof(dest_addr.sun_path));
    }

    /*ip_server = (char *)&ip_server_buff;
    //https://www.systutorials.com/docs/linux/man/3-inet_aton/
    //The inet_aton() function returns 1 if the address is successfully converted, or 0 if the conversion failed.
    if (  inet_aton(ip_server, &dest_addr.sin_addr ) == 0 )  {
        fprintf(stderr, "Invalid address\n");

    }*/

    //Conexion
    //Upon successful completion, connect() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
    while( connect( sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr) ) < 0 ){
        perror("Intentando conectar al servidor");
        //printf("Reintentando coenctar en %d segundos \n", retry_time);
        sleep(retry_time);
    }
    printf("Se ha realizado la conexion de manera exitosa con el servidor \n");


    while( 1 ){


        //En este punto se recibe el comando desde la estacion terrestre en forma de codigo numerico
        //Upon successful completion, recv() returns the length of the message in bytes. If no messages are available to be received and the peer has performed an orderly shutdown, recv() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
        recv(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0);


        //opciones del lado del cliente:
        /*
        1 - Update Satellite Firmware
        2 - Start Scanning
        3 - Get Telemetry
        */
        printf("%s\n", buffer_recepcion);
        if (strcmp(buffer_recepcion, "1") == 0 ){ // Update Satellite Firmware
            memset(buffer_recepcion, 0, sizeof(buffer_recepcion));
            printf("se ha invocado a la funcion de actializar firmaware\n");
            //recv(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0 );

            ////////////update_firmware(sockfd); // le paso a la func el sockfd que debe usar ya que ahi llegaran los datos

        }
        else if( strcmp(buffer_recepcion, "2") == 0 ){ // Start Scanning
            printf("se ha invocado a la funcion de escaneo de imagen\n\n");
            //////////start_scanning(sockfd);
        }
        else if( strcmp(buffer_recepcion, "3") == 0 ){ // Get Telemetry
            printf("se ha invocado a la funcion de envio de telemetria\n\n\n");
            ///////////send_telemetria();
        }
        else{ // manejo de errores: opciones no validas o perdida de conexion
            printf("DEBUG: se recibio algo distinto de 1 2 o 3\n");
            if (buffer_recepcion == NULL){
                printf("buffer vacio, desconexion\n");
            }
            else{
                printf("opcion no valida, eligir una opcion...\n");
            }
            sleep(2);
        }


    }//end while

    if ( shutdown( sockfd, SHUT_RDWR ) < 0 ){
        perror("shutdown fail");
    }

    return 0;
}//end main

/**
 * Set a file descriptor to blocking or non-blocking mode.
 *
 * @param fd The file descriptor
 * @param blocking 0:non-blocking mode, 1:blocking mode
 *
 * @return 1:success, 0:failure.
 **/
int fd_set_blocking(int fd, int blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}