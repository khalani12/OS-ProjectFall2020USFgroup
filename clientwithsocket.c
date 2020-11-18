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
   
    server = gethostbyname("osnode05"); 

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

   
   printf("Send message \'ready\' to begin game.\n", 36);
   int res = 1;
   while(res != 0)
   {
     bzero(buffer,256);                                    //starts socket
     fgets(buffer,255,stdin);
     
     res = strcmp(buffer,"ready\n"); // ready check
   }
    status = write(socketid, buffer, strlen(buffer));
    if (status < 0)
    {
      printf("error while sending client message to server\n");
    }
   
    int res_quit = 1;
    while(res_quit != 0)  
    {
      int res_wait = 1;
     while(res_wait != 0)
     {
       bzero(buffer,256);                         
       status = read(socketid, buffer, 29);
       if (status < 0) {
          perror("error while reading message from server");
          exit(1);
       }
       printf("%s\n",buffer);
       res_wait = strcmp(buffer,"It is the start of your turn\n");
     }
     status = read(socketid, buffer, 255);
     if (status < 0) {
       perror("error while reading message from server");
       exit(1);
     }
     printf("%s\n",buffer);//read board

     bool first_choice_valid = false;
     while(!first_choice_valid)
     {
      printf("Enter first card: ");               //writes first card
      bzero(buffer,256);
      fgets(buffer,255,stdin);
      char f = buffer[0];
      if(f > 96 && f < 115)
      {first_choice_valid = 1;}
      else
      {printf("Not a valid letter choice.\n");}
     }
     res_quit = strcmp(buffer,"quit\n");
     if(res_quit == 0)
     {
       printf("Exiting game loop.\n");
       break;
     }

     status = write(socketid, buffer, strlen(buffer));
     if (status < 0)
     {
       printf("error while sending client message to server\n");
     }
  
     bzero(buffer,256); 
     status = read(socketid, buffer, 255);
     if (status < 0) {
       perror("error while reading message from server");
       exit(1);
     }
     printf("BOARD:\n%s\n",buffer); // game board after first selection

     bool second_choice_valid = false;
      while(!second_choice_valid)
      {
        printf("Enter second card: ");             //writes second card
        bzero(buffer,256);
        fgets(buffer,255,stdin);   
        char s = buffer[0];
        if(s > 96 && s < 115)
        {second_choice_valid = 1;}
        else
        {printf("Not a valid letter choice.\n");}
      }
     res_quit = strcmp(buffer,"quit\n");
     if(res_quit == 0)
     {
       printf("Exiting game loop.\n");
       break;
     }
     status = write(socketid, buffer, strlen(buffer));
     if (status < 0)
     {
       printf("error while sending client message to server\n");
     }
     
     bzero(buffer,256); 
     status = read(socketid, buffer, 255);
     if (status < 0) {
        perror("error while reading message from server");
        exit(1);
     }
      printf("BOARD:\n%s\n",buffer); // game board after second selection
     
     bzero(buffer,256); 
     status = read(socketid, buffer, 11);
     if (status < 0) {
        perror("error while reading message from server");
        exit(1);
     }
      printf("Choice is: %s\n",buffer); //reads 'Correct!' or 'Incorrect!' message
    }

   /* this closes the socket*/
   close(socketid);  
} 