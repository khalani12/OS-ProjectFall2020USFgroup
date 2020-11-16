/* this program shows how to create sockets for a client.
it also shows how the client connects to a server socket.
and sends a message to it. the server must already be running
on a machine. The name of this machine must be entered in the function gethostbyname in the code below. The port number where the server is listening is specified in PORTNUM. This port number must also be specified in the server code.
 * main program */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdbool.h>

#define PORTNUM  5220 /* the port number that the server is listening to*/
#define DEFAULT_PROTOCOL 0  /*constant for default protocol*/



void main()


{
   int  port;
   int  socketid;      /*will hold the id of the socket created*/
   int  status;        /* error status holder*/
   char buffer[256];   /* the message buffer*/
   char buffer2[256];
   struct sockaddr_in serv_addr;

   struct hostent *server;

   /* this creates the socket*/
   socketid = socket (AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
   if (socketid < 0) 
    {
      printf( "error in creating client socket\n"); 
      exit (-1);
    }

    printf("created client socket successfully\n");

   /* before connecting the socket we need to set up the right     	values in the different fields of the structure server_addr 
   you can check the definition of this structure on your own*/
   
    server = gethostbyname("osnode08"); 

   if (server == NULL)
   {
      printf(" error trying to identify the machine where the 	server is running\n");
      exit(0);
   }

   port = PORTNUM;
/*This function is used to initialize the socket structures with null values. */
   bzero((char *) &serv_addr, sizeof(serv_addr));

   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length); 
   serv_addr.sin_port = htons(port);
   
   /* connecting the client socket to the server socket */

   status = connect(socketid, (struct sockaddr *) &serv_addr, 
                            sizeof(serv_addr));
   if (status < 0)
    {
      printf( " error in connecting client socket with server	\n");
      exit(-1);
    }

   printf("connected client socket to the server socket \n");

   /* now lets send a message to the server. the message will be
     whatever the user wants to write to the server.*/
   
   int res = strcmp(buffer,"ready\n");                //ready check
   while(res != 0)
   {
     //printf("enter a nessage that you want the server to receive\n");
     bzero(buffer,256);                                    //starts socket
     fgets(buffer,255,stdin);
     status = write(socketid, buffer, strlen(buffer));
     
     res = strcmp(buffer,"ready\n");
   
     if (status < 0)
     {
       printf("error while sending client message to server\n");
     }
   }
   
    res = strcmp(buffer,"quit\n");
    while(res != 0)  
    {
     bool check = false;
     bzero(buffer,256);                         //reads "wait for your turn"
     status = read(socketid, buffer, 255);
     int res_wait = strcmp(buffer,"Please wait for your turn\n");
     while(res_wait == 0)
     {
       check = true;
       printf("%s\n",buffer);
       bzero(buffer,256);                         
       status = read(socketid, buffer, 255);
       if (status < 0) {
          perror("error while reading message from server");
          exit(1);
       }
       res_wait = strcmp(buffer,"Please wait for your turn\n");
     }
     if(check)
       printf("It is the start of your turn\n");
     else
       printf("%s\n",buffer);
     printf("Enter first card: ");               //writes first card
     bzero(buffer,256);
     fgets(buffer,255,stdin);
     status = write(socketid, buffer, strlen(buffer));
     
     if (status < 0)
     {
       printf("error while sending client message to server\n");
     }
     
     printf("Enter second card: ");             //writes second card
     bzero(buffer2,256);
     fgets(buffer2,255,stdin);
     status = write(socketid, buffer2, strlen(buffer2));
     
     if (status < 0)
     {
       printf("error while sending client message to server\n");
     }
    }

   /* this closes the socket*/
   close(socketid);

  
} 
