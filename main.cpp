#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <sstream>
#include <math.h>

#include <pthread.h>

#define PERIODO 0.1
#define G 981.0
#define L0 15.0

double l1 = 0, l2 = 0, l1_ant = 0, l2_ant = 0, volts = 0, g[2][2], h[2];
bool run = false;
void *tanque_t(void *param);
void setup();

using namespace std;

int main() {
    setup();
    pthread_t thread_tanque;
    pthread_attr_t attr_thread_tanque;
    pthread_attr_init(&attr_thread_tanque);
    int server_sockfd, client_sockfd;
    size_t server_len;
    socklen_t client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    unsigned short porta = 20081;

    unlink("server_socket");  // remocao de socket antigo
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0))  < 0) {
        printf(" Houve erro na ebertura do socket ");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(porta);

    server_len = sizeof(server_address);

    if(bind(server_sockfd, (struct sockaddr *) &server_address, server_len) < 0) {
        perror("Houve error no Bind");
        exit(1);
    }

    if (listen(server_sockfd, 5) != 0) {
        perror("Houve error no Listen");
        exit(1);
    }

    while(1) {
        char ch;
        printf("Servidor esperando ...\n");

        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
        if (client_sockfd < 0) {
            perror("Houve erro no Accept");
            exit(2);
        }
        char *buf = (char*) malloc(256 * sizeof(char));

        printf("Passou o accept\n");
        run = true;
        // Inicializar thread
        pthread_create(&thread_tanque, &attr_thread_tanque, tanque_t, NULL);

        while (recv(client_sockfd, buf, 16, 0)) {
            std::stringstream msg(buf);
            std::string tipo;
            msg >> tipo;
            if (tipo.find("READ") == 0) {
                int canal;
                msg >> canal;
                if (canal == 0) {
                    sprintf(buf, "%lf\n", (l1/6.25));
                    write(client_sockfd, buf, strlen(buf));
                } else if (canal == 1){
                    sprintf(buf, "%lf\n", (l2/6.25));
                    write(client_sockfd, buf, strlen(buf));
                }
            } else if (tipo.find("WRITE") == 0) {
                int ch;
                msg >> ch >> volts;
                sprintf(buf, "ACK\n");
                write(client_sockfd, buf, strlen(buf));
            }
        }
        close(client_sockfd);
        run = false;
    	pthread_join(thread_tanque, NULL);
    }
}

void *tanque_t(void *param) {
    while (run){
        l1 = g[0][0]*l1_ant + h[0]*volts*3;
        l2 = g[1][0]*l1_ant + g[1][1]*l2_ant + h[1]*volts*3;
        l1_ant = l1;
        l2_ant = l2;
        printf("%lf\t%lf\t%lf\n", l1, l2, volts);
        usleep(PERIODO*1000000);
    }
    l1 = 0; l2 = 0; l1_ant = 0; l2_ant = 0; volts = 0;
    return 0;
}

void setup() {
    double area_base = (M_PI * pow(4.45, 2))/4.0f;
    double area_out = (M_PI * pow(0.52, 2))/4.0f;
    double alfa = (area_out/area_base)*sqrt(G/(2*L0));
    double beta = 3.3/area_base;
    g[0][0] = exp(-1*alfa*PERIODO);
    g[0][1] = 0;
    g[1][0] = alfa*PERIODO*exp(-1*alfa*PERIODO);
    g[1][1] = exp(-1*alfa*PERIODO);

    h[0] = (beta/alfa)*(1 - exp(-1*alfa*PERIODO));
    h[1] = (beta/alfa)*(1 - exp(-1*alfa*PERIODO)*((alfa*PERIODO)+1));
}
