/* this program shows how to create sockets for a client.
it also shows how the client connects to a server socket.
and sends a message to it. the server must already be running
on a machine. The name of this machine must be entered in the function gethostbyname in the code below. The port number where the server is listening is specified in PORTNUM. This port number must also be specified in the server code.
 * main program */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>

#define PORTNUM 5240                /* the port number that the server is listening to*/
#define DEFAULT_PROTOCOL 0          /* constant for default protocol*/
#define SERVER_NODE_NAME "osnode05" /* UPDATE THIS STRING WITH NODE THE SERVER IS RUNNING ON */
pthread_t th1, th2;
pthread_mutex_t mutex;
bool first_check = false;
bool second_check = false; //all these global bools was me and Al trying to figure out how to print first and second card repeatidly
void *listen_connection(void *p_newsockfd) //thread function just for reading **FIX ME** Don't know how to make it print out the things in proper order
{
  int newsockfd = *((int *)p_newsockfd);
  char buffer[256];
  int status;
  int count = 0;
  while(1)
  {
      bzero(buffer, 256);
      status = read(newsockfd, buffer, 255);
      if(first_check)
      {
        strcat(buffer,"Enter first card");
      }
      if(second_check)
      {
        strcat(buffer,"Enter second card");
      }
      printf("\n%s",buffer);
  }
}
void *write_connection(void *p_newsockfd) //thread function for writing mainly
{
  int newsockfd = *((int *)p_newsockfd);
  char buffer[256];
  int status;
  
  int res = 1;
  while (res != 0)
  {
      bzero(buffer, 256); //starts socket
      fgets(buffer, 255, stdin);

      res = strcmp(buffer, "ready\n"); // ready check
  }
  status = write(newsockfd, buffer, strlen(buffer));
  if (status < 0)
  {
      printf("error while sending client message to server\n");
  }

  while(1)
    {
      int res = 1;
      char f;
      bool first_choice_valid = false;
      while (!first_choice_valid) //first card,**NOTE** we want it to print out again after each time a new read happens which is the difficult part
      {
            printf("Enter first card: ");
            first_check = true;
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            f = buffer[0];
            if (f > 96 && f < 115)
            {
                first_choice_valid = 1;
            }
            else
            {
                printf("Not a valid letter choice.\n");
            }
      }
      first_check = false;
      pthread_mutex_lock(&mutex);
      status = write(newsockfd, buffer, strlen(buffer)); //protected write for first client to enter in their card and modifies the board
      pthread_mutex_unlock(&mutex);
      if (status < 0)
      {
          printf("error while sending client message to server\n");
      }
      bool second_choice_valid = false;
      while (!second_choice_valid) //second card,**NOTE** we want it to print out again after each time a new read happens which is the difficult part
      {
            printf("Enter second card: ");
            second_check = true;
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            char s = buffer[0];
            if (s > 96 && s < 115 && f != s)
            {
                second_choice_valid = 1;
            }
            else
            {
                printf("Not a valid letter choice.\n");
            }
      }
      second_check = false;
      pthread_mutex_lock(&mutex);
      status = write(newsockfd, buffer, strlen(buffer)); //protected write for first client to enter in their card and modifies the board
      pthread_mutex_unlock(&mutex);
      if (status < 0)
      {
          printf("error while sending client message to server\n");
      }
      
     }
}

void main()
{
    int port;
    int socketid;     /*will hold the id of the socket created*/
    int status;       /* error status holder*/
    char buffer[256]; /* the message buffer*/
    char buffer2[256];
    struct sockaddr_in serv_addr;
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_mutex_init(&mutex, NULL);
    
    struct hostent *server;

    /* this creates the socket*/
    socketid = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (socketid < 0)
    {
        printf("error in creating client socket\n");
        exit(-1);
    }

    printf("created client socket successfully\n");

    /* before connecting the socket we need to set up the right     	values in the different fields of the structure server_addr 
   you can check the definition of this structure on your own*/

    server = gethostbyname(SERVER_NODE_NAME);

    if (server == NULL)
    {
        printf(" error trying to identify the machine where the server is running\n");
        exit(0);
    }

    port = PORTNUM;
    /*This function is used to initialize the socket structures with null values. */
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);

    /* connecting the client socket to the server socket */

    status = connect(socketid, (struct sockaddr *)&serv_addr,
                     sizeof(serv_addr));
    if (status < 0)
    {
        printf(" error in connecting client socket with server	\n");
        exit(-1);
    }

    printf("connected client socket to the server socket \n");
    
    pthread_create(&th1, &attr, *write_connection, &socketid); //creates both threads, one for writing and the other for reading
    pthread_create(&th2, &attr, *listen_connection, &socketid);
    
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    /* this closes the socket*/
    close(socketid);
    printf("TEST\n");
}