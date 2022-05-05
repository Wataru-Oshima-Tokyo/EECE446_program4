#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>

#define TRUE             1
#define FALSE            0
#define MAX_FILES        10
#define MAX_FILENAME_LEN 255


int sendall(int s, char* buf, int* len);
struct peer_entry {
   uint32_t id; // ID of peer
   int socket_descriptor; // Socket descriptor for connection to peer
   char files[MAX_FILES][MAX_FILENAME_LEN]; // Files published by peer
   struct sockaddr_in address; // Contains IP address and port number
};


void joinFunction(int s, struct peer_entry *peer){
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      int ret = getpeername(s, (struct sockaddr*)&addr, &len);
      if(ret<0){
         perror("getpeername");
         exit(EXIT_FAILURE);
      }
      peer->socket_descriptor = s;
      peer->address.sin_addr = addr.sin_addr;
      peer->address.sin_port = addr.sin_port;
}

void publishFunction(struct peer_entry *peer, char *_buffer){
      int file_count=0;
      memcpy(&file_count, _buffer+1,4);
      file_count = htonl(file_count);
      printf("%d ", file_count);
      int total_len = 5;
      int _length=strlen(_buffer+5);
      for(int j=0; j<file_count;j++){
         memcpy(&peer->files[j], _buffer+total_len, _length);
         printf("%s ", peer->files[j]);
         total_len +=_length+1;
         _length = strlen(_buffer+total_len);
      }
      printf("\n");
}

int searchFunction(struct peer_entry *peer, char *filename){
      for(int j =0; j<(sizeof(peer->files)/sizeof(peer->files[0])); j++){
         if(strcmp(filename,peer->files[j])==0)
            return 1;
      }
      return 0;
}

int main (int argc, char *argv[])
{
   int    i, len, rc, on = 1;
   int    listen_sd, max_sd, new_sd;
   int    desc_ready, end_server = FALSE;
   int    close_conn;
   char   buffer[512];
   struct sockaddr_in   addr;
   fd_set master_set, working_set;
   struct peer_entry peer[5] ={0};
   int addrlen = sizeof(addr);
   int server_port_num;
   if(argc == 2)
      server_port_num = atoi(argv[1]);
   else{
      puts("Please put a port number\n");
      exit(1);
   }
   /*************************************************************/
   /* connections on                                            */
   /*************************************************************/
   listen_sd = socket(AF_INET, SOCK_STREAM, 0);
   if (listen_sd < 0)
   {
      perror("socket() failed");
      exit(-1);
   }

   /*************************************************************/
   /* Allow socket descriptor to be reuseable                   */
   /*************************************************************/
   rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
   if (rc < 0)
   {
      perror("setsockopt() failed");
      close(listen_sd);
      exit(-1);
   }

   /*************************************************************/
   /* Set socket to be nonblocking. All of the sockets for      */
   /* the incoming connections will also be nonblocking since   */
   /* they will inherit that state from the listening socket.   */
   /*************************************************************/
   rc = ioctl(listen_sd, FIONBIO, (char *)&on);
   if (rc < 0)
   {
      perror("ioctl() failed");
      close(listen_sd);
      exit(-1);
   }

   /*************************************************************/
   /* Bind the socket                                           */
   /*************************************************************/
      memset( &addr, 0, sizeof( addr ) );
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(server_port_num);

   rc = bind(listen_sd, (struct sockaddr*)&addr,sizeof(addr));
   if (rc < 0)
   {
      perror("bind() failed");
      close(listen_sd);
      exit(-1);
   }

   /*************************************************************/
   /* Set the listen back log                                   */
   /*************************************************************/
   rc = listen(listen_sd, 32);
   if (rc < 0)
   {
      perror("listen() failed");
      close(listen_sd);
      exit(-1);
   }

   /*************************************************************/
   /* Initialize the master fd_set                              */
   /*************************************************************/
   FD_ZERO(&master_set);
   max_sd = listen_sd;
   FD_SET(listen_sd, &master_set);

   /*************************************************************/
   /* Loop waiting for incoming connects or for incoming data   */
   /* on any of the connected sockets.                          */
   /*************************************************************/
   int num_peer = 0;
   do
   {
      /**********************************************************/
      /* Copy the master fd_set over to the working fd_set.     */
      /**********************************************************/
      memcpy(&working_set, &master_set, sizeof(master_set));

      /**********************************************************/
      /* Call select()                                          */
      /**********************************************************/
      rc = select(max_sd + 1, &working_set, NULL, NULL, NULL);

         /**********************************************************/
         /* Check to see if the select call failed.                */
         /**********************************************************/
         if (rc < 0)
         {
            perror("  select() failed");
            break;
         }

         /**********************************************************/
         /* One or more descriptors are readable.  Need to         */
         /* determine which ones they are.                         */
         /**********************************************************/
         desc_ready = rc;
         for (i=0; i <= max_sd  &&  desc_ready > 0; ++i)
         {
            /*******************************************************/
            /* Check to see if this descriptor is ready            */
            /*******************************************************/
            if (FD_ISSET(i, &working_set))
            {
               /****************************************************/
               /* A descriptor was found that was readable - one   */
               /* less has to be looked for.  This is being done   */
               /* so that we can stop looking at the working set   */
               /* once we have found all of the descriptors that   */
               /* were ready.                                      */
               /****************************************************/
               desc_ready -= 1;

               /****************************************************/
               /* Check to see if this is the listening socket     */
               /****************************************************/
               if (i == listen_sd)
               {
                  /*************************************************/
                  /* Accept all incoming connections that are      */
                  /* queued up on the listening socket before we   */
                  /* loop back and call select again.              */
                  /*************************************************/
                  do
                  {
                     /**********************************************/
                     /* Accept each incoming connection.  If       */
                     /* accept fails with EWOULDBLOCK, then we     */
                     /* have accepted all of them.  Any other      */
                     /* failure on accept will cause us to end the */
                     /* server.                                    */
                     /**********************************************/
                     new_sd = accept(listen_sd, (struct sockaddr*)&addr,(socklen_t*)&addrlen);
                     if (new_sd < 0)
                     {
                        if (errno != EWOULDBLOCK)
                        {
                           perror("  accept() failed");
                           end_server = TRUE;
                        }
                        break;
                     }

                     /**********************************************/
                     /* Add the new incoming connection to the     */
                     /* master read set                            */
                     /**********************************************/
                     FD_SET(new_sd, &master_set);
                     if (new_sd > max_sd)
                        max_sd = new_sd;

                     /**********************************************/
                     /* Loop back up and accept another incoming   */
                     /* connection                                 */
                     /**********************************************/
                  } while (new_sd != -1);
               }

               /****************************************************/
               /* This is not the listening socket, therefore an   */
               /* existing connection must be readable             */
               /****************************************************/
               else
               {
                  close_conn = FALSE;
                  /*************************************************/
                  /* Receive all incoming data on this socket      */
                  /* before we loop back and call select again.    */
                  /*************************************************/
                  rc = recv(i, buffer, sizeof(buffer), 0);

                  if(rc==0){
                    printf("  Connection closed\n");
                    close_conn = TRUE;
                  }
                
                  if (close_conn)
                  {
                     struct peer_entry temp[5] = {0};
                        int _count=0;
                        for(int j=0; j<sizeof(peer)/sizeof(peer[0]);j++){
                           if(peer[j].socket_descriptor != i){
                              temp[_count] = peer[j];
                              _count++;
                           }
                        }
                     
                     num_peer = _count;
                     memcpy(&peer, &temp, sizeof(temp));
                     close(i);
                     FD_CLR(i, &master_set);
                     if (i == max_sd)
                     {
                           while (FD_ISSET(max_sd, &master_set) == FALSE)
                              max_sd -= 1;
                     }
                     continue;
                  }




                     if (rc < 0)
                     {
                        if (errno != EWOULDBLOCK)
                        {
                              perror("  recv() failed");
                              close_conn = TRUE;
                        }
                        break;
                     }
                     len = rc;
                     uint16_t action =0;
                     uint32_t peerID=0;
                     memcpy(&action, buffer, 1);
                     if(action ==0 && len>0){
                        if(num_peer>4){
                           puts("The number of peer is the maximum please close at least one connected peern\n");
                        }else{
                           memcpy(&peerID, buffer+1, 4);
                           long int _peerID = htonl(peerID);
                           int process =TRUE;
                           for(int j=0; j<sizeof(peer)/sizeof(peer[0]);j++){
                              if(peer[j].socket_descriptor==i) process=FALSE;
                           }
                           if(process){
                              printf("TEST] JOIN %ld\n", _peerID);
                              peer[num_peer].id = peerID;
                              joinFunction(i,&peer[num_peer]);
                              num_peer++;
                           }else{
                              puts("Already joined\n");
                           }
                        }
                     }
                     else if(action==1){
                        printf("TEST] PUBLISH ");
                        int process =FALSE;
                        int temp_peer_num=0;
                        for(int j=0; j<sizeof(peer)/sizeof(peer[0]);j++){
                           if(peer[j].socket_descriptor==i){
                              process=TRUE;
                              temp_peer_num =j;
                           } 
                        }
                        if(process){
                           publishFunction(&peer[temp_peer_num], buffer);
                        }else{
                           puts("The peer is not joined yet\n");
                        }
                     }else if (action==2){
                        char temp[255] ={0};
                        memcpy(&temp, buffer+1, strlen(buffer+1));
                        printf("TEST] SEARCH %s", temp);
                        int fnum=-1;
                        for(int j=0; j<sizeof(peer)/sizeof(peer[0]);j++){
                           if(searchFunction(&peer[j],temp)>0){
                              fnum =j;
                              break;
                           }

                        }
                        char sendData[10] ={0};
                        int size = sizeof(sendData);
                        if(fnum>=0){
                           char addr_str[INET_ADDRSTRLEN];
                           inet_ntop(AF_INET, &(peer[fnum].address.sin_addr), addr_str, INET_ADDRSTRLEN);
                           memcpy(sendData, &peer[fnum].id, 4);
                           memcpy(sendData+4, &(peer[fnum].address.sin_addr), 4);
                           memcpy(sendData+8, &peer[fnum].address.sin_port, 2);
                           long int _peerID = htonl(peer[fnum].id);
                           printf(" %ld ", _peerID);
                           printf("%s:", inet_ntoa(peer[fnum].address.sin_addr));
                           printf("%d\n", ntohs(peer[fnum].address.sin_port));
                        }else{
                           printf(" 0 0.0.0.0:0\n");
                        }
                        if((sendall(i, sendData, &size)) == -1)
                        {
                                 printf("error\n");
                                 exit(1);
                        }
                     }
                  

            } /* End of existing connection is readable */
         } /* End of if (FD_ISSET(i, &working_set)) */
      } /* End of loop through selectable descriptors */
   } while (end_server == FALSE);

   /*************************************************************/
   /* Clean up all of the sockets that are open                 */
   /*************************************************************/
   // for (i=0; i <= max_sd; ++i)
   // {
   //    if (FD_ISSET(i, &master_set))
   //       close(i);
   // }

   return 0;
}


// Function to deal with partial send --- from Beej's guide on Partial send()/recv()
int sendall(int s, char* buf, int* len)
{
        int total = 0;        // how many bytes we've sent
        int bytesleft = *len; // how many we have left to send
        int n;

        while (total < *len) {
                n = send(s, buf + total, bytesleft, 0);
                if (n == -1) { break;}
                total = total + n;
                bytesleft = bytesleft - n;
        }

        *len = total; // return number actually sent here

        return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}