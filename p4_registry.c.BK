//Wataru Oshima
//Jesus Ramirez
//EECE 446 Spring 2022

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <dirent.h>
#define PORT 8080
int main(int argc, char *argv[]) {
        
    int s;
    if(argc <2){
        printf("Usage: ./p4_registry <port>\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    printf("%d", port);
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    // char* hello = "Hello from server";
    puts("Hello from server\n");
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))< 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    // valread = read(new_socket, buffer, 1024);
    // printf("%s\n", buffer);
    // send(new_socket, hello, strlen(hello), 0);
    // printf("Hello message sent\n");
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int ret = getpeername(new_socket, (struct sockaddr*)&addr, &len);
    if(ret<0){
        perror("getpeername");
        exit(EXIT_FAILURE);
    }
    printf("Peer IP address: %s\n", inet_ntoa(addr.sin_addr));
    printf("Peer port      : %d\n", ntohs(addr.sin_port));
    // while(1){
    //     char response[100];
    //     int flag = recv(new_socket, response, 10, 0);
    //     // printf("flag is %d\n", flag);
    //     if(flag==1){
    //         printf("error\n");
    //     }
    //     else {
    //             char content[10];
    //             // memcpy(&content, response, 10);
    //             printf("%s", response);
    //     }
    // }

    return 0;
}
