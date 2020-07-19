#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <zconf.h>
#include <fcntl.h>
#include <asm/errno.h>
#include <errno.h>
#include <bits/signum.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <stdint.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PATH "/tmp/socc"
#define FIRMWARE_FILE "./updated_firmaware_received"
#define FILE_BUFFER_SIZE 1500

char firmware_version[20] = "1.0";
int retry_time=1;

int fd_set_blocking(int fd, int blocking);
long get_uptime(); //esta funcion devuelve el uptime del SO del satelite
//funcion 1
int update_firmware(int sockfd_arg);
//funcion 2
int start_scanning(int sockfd);
int send_telemetria();
void get_dir();


/**
 * @brief Programa cliente. Tiene como objetivo simular el firmaware del satelite que se conecta al programa servidor (Base Terrestre) y queda a la espera de ordenes del mismo.
 * @return
 */
int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN);
    printf("DEBUG: ejecutando main \n");
    printf("Version del firmware: %s\n", firmware_version);
    int sockfd;
    //int ip_srv_load;
    struct sockaddr_un dest_addr;

    char buffer_recepcion[BUFFER_SIZE];

    //crer socket
    sockfd= socket(AF_UNIX, SOCK_STREAM, 0);
    //int setting_ok;
    fd_set_blocking(sockfd,1); //la funcion devuelve un int pero no se usa. CPPCHECK no compila si no se usa.

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
            printf("DEBUG: se ha invocado a la funcion de actializar firmaware\n");
            //recv(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0 );

            update_firmware(sockfd); // le paso a la func el sockfd que debe usar ya que ahi llegaran los datos

        }
        else if( strcmp(buffer_recepcion, "2") == 0 ){ // Start Scanning
            printf("se ha invocado a la funcion de escaneo de imagen\n\n");
            memset(buffer_recepcion, 0, sizeof(buffer_recepcion));
            //////////start_scanning(sockfd);
        }
        else if( strcmp(buffer_recepcion, "3") == 0 ){ // Get Telemetry
            memset(buffer_recepcion, 0, sizeof(buffer_recepcion));
            printf("se ha invocado a la funcion de envio de telemetria\n\n\n");
            ///////////send_telemetria();
        }
        else{ // manejo de errores: opciones no validas o perdida de conexion
            printf("DEBUG: se recibio un comando no valido\n");
            if (send(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0) < 0){
                perror("error en la comunicacion");
                if (errno == EPIPE){
                    //relanzar el cliente con execv o similar
                    char ejecutable[100] = "";
                    strcat(ejecutable, "./tp1_client_u" );
                    char *argv[] = {"./tp1_client_u", NULL};
                    //ejemplo de uso de excev
                    /*
                     https://pubs.opengroup.org/onlinepubs/009695399/functions/exec.html
                     Using execv()

                        The following example passes arguments to the ls command in the cmd array.

                        #include <unistd.h>


                        int ret;
                        char *cmd[] = { "ls", "-l", (char *)0 };
                        ...
                        ret = execv ("/bin/ls", cmd);
                    */
                    if (execv(ejecutable, argv) < 0){
                        perror("error al intentar reiniciar");
                        exit(231);
                    }
                }
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

/**
 * @brief Funcion que devuelve el uptime del sistema cuando es invacada
 * @return Devuelve un long con el uptime del sistema en segundos
 */
long get_uptime(){
    struct sysinfo s_info;
    int error = sysinfo(&s_info);
    if(error != 0)
    {
        printf("code error = %d\n", error);
    }
    return s_info.uptime;
}

//funcion 1
/**
 * @brief Recibe mediante el socjet TCP de la comunicacion establecida un nuevo firmware. Luego de recibirlo satisfactoriamente se cierra el proceso actual y se ejecuta en nuevo firmaware perdiendo la conexion.
 * @param sockfd_arg El socket de la comunicacion TCP establecida.
 * @return Devuelve 0 si no se ha podido crear en el file system el archivo para la recepcion del firmware. Devuelve 1 en caso de que no se haya podido reiniciar el proceso cliente.
 */
int update_firmware(int sockfd_arg){
    printf("DEBUG: ha invocado la funcion 'Update Satellite Firmware'\n");
    printf("la version actual del firmware en este dispositivo es: %s\n", firmware_version);
    int firmware_fd;
    int bytes_escritos=0;

    //try to open fd
    /*
    //https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html
    //int open(const char *path, int oflag, file_permissions );
    */
    if ( (firmware_fd=open(FIRMWARE_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0777) ) < 0 ){
        perror("error al crear el archivo");
        return 0;
    }

    char buffer_recepcion[FILE_BUFFER_SIZE]; //FILE_BUFFER_SIZE=16000
    long byte_leido;

    //uint32_t bytes_recibidos;
    char bytes_recibidos[BUFFER_SIZE];

    //atencion que estoy leyendo de a 4 bytes a la vez, no leo el stream completo
    /* Read N bytes into BUF from socket FD.
   Returns the number read or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.
    extern ssize_t recv (int __fd, void *__buf, size_t __n, int __flags);
    */
    if ( (byte_leido=recv(sockfd_arg, &bytes_recibidos, sizeof(bytes_recibidos), 0) ) != 0 ){
        if ( byte_leido <= 0 ){
            perror("error en la recepcion a traves del socket_arg");
        }

    }
    //pongo esto en duro para probar...
    char src[20];
    //strcpy(src, "13624");
    //strncpy(bytes_recibidos, src, sizeof(bytes_recibidos));
    //se transforma el numero de bytes recibidos de network a host long (uint32_t)
    //bytes_recibidos=ntohl(bytes_recibidos);
    printf("Cantidad de bytes en el archivo a recibir (stream TCP): %s\n", bytes_recibidos);
    char *ptr; //puntero requerido por la funcion strtol()
    long bytes_a_recibir=1;
    bytes_a_recibir=strtol(bytes_recibidos, &ptr, 10);

    //mientras queden bytes sin leer...
    while(bytes_a_recibir){
        memset(buffer_recepcion, 0, sizeof(buffer_recepcion));

        //controlo un prosible error de recepcion
        if( (byte_leido = recv(sockfd_arg, buffer_recepcion, sizeof(buffer_recepcion), 0)) != 0){ //puede ser cero al final de la recepcion del archivo
            if (byte_leido < 0){
                perror("error en la recepcion en el socket 'sockfd_arg'");
            }
        }

        //voy a ir escribiendo el archivo que cree con los bytes que vaya "sacando/leyendo" del socket
        if(0 > (bytes_escritos = write(firmware_fd, buffer_recepcion, (size_t) byte_leido))  ){
            perror("error al escribir el archivo creado");
            _exit(EXIT_FAILURE);
        }

        printf("%d\n", bytes_escritos);
        bytes_a_recibir -= byte_leido;
    }

    //cierro el file descriptor
    close(firmware_fd);

    printf("DEBUG: Finalizada la recepcion del archivo desde la estacion terrestre\n");
    printf("reiniciando el sistema con el nuevo firmware\n");
    //cierro tambien el socket_arg
    close(sockfd_arg); //esto lo cierro solo cuando implemente el RE-EJECUTAR

    //reiniciar automaticamente el programa con la nueva version de firmware
    char ejecutable[100] = "";
    strcat(ejecutable, FIRMWARE_FILE );
    char *argv[] = {FIRMWARE_FILE, NULL};
    //ejemplode uso de excev
    /*
     https://pubs.opengroup.org/onlinepubs/009695399/functions/exec.html
     Using execv()

        The following example passes arguments to the ls command in the cmd array.

        #include <unistd.h>


        int ret;
        char *cmd[] = { "ls", "-l", (char *)0 };
        ...
        ret = execv ("/bin/ls", cmd);
    */
    if (execv(ejecutable, argv) < 0){
        perror("error al reiniciar");
    }
    return 1;
}