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

#define PORTNUM 5252                /* the port number that the server is listening to*/
#define DEFAULT_PROTOCOL 0          /* constant for default protocol*/
#define SERVER_NODE_NAME "osnode05" /* UPDATE THIS STRING WITH NODE THE SERVER IS RUNNING ON */

pthread_t th1, th2;
pthread_mutex_t mutex;
bool first_check = false;
bool second_check = false; //all these global bools was me and Al trying to figure out how to print first and second card repeatidly
bool third_check = false;
bool not_taken = true;
bool taken = false;
bool reading = true;
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
        int res = strcmp(buffer,"Taken\n"); //this is where im trying to make it realize the symbol is taken
        int res2 = strcmp(buffer,"Not Taken\n");
        if(res == 0)
        {
          taken = true;   //if taken
          printf("%s",buffer); 
        }
        else if(res2 == 0)
        {
          taken = false;  //if not taken exists the loop waiting to see whether or not something is taken
          reading = false;
          //printf("%s",buffer); 
        }
        else
        {
          printf("%s",buffer); 
        }
        if(first_check && !second_check && !third_check)
        {
          printf("Enter first card\n");  //prints 1st card
        }
        if(second_check)
        {
          printf("Enter second card\n"); //prints 2nd card
          third_check = true;
        }
        if(third_check)
        {
          //printf("\n");
          third_check = false;
        }
  }
}
void *write_connection(void *p_newsockfd) //thread function for writing mainly
{
  int newsockfd = *((int *)p_newsockfd);
  char buffer[256];
  int status;
  int count = 0;
  int res = 1;
  while(1)
    {
      int res = 1;
      char f;
      bool first_choice_valid = false;
      while (!first_choice_valid) //first card,**NOTE** we want it to print out again after each time a new read happens which is the difficult part
      {
          reading = true; //resets the taken loop
          taken = false;
          if(count == 0)
          {
            printf("Enter first card\n");
            count++;
          }
          first_check = true; //enables first print
          bzero(buffer, 256);
          fgets(buffer, 255, stdin); //takes in first card
          f = buffer[0];
          if (f > 96 && f < 115)
          {
              first_choice_valid = 1;
          }
          else
          {
              printf("Not a valid letter choice.\n");
          }
          //pthread_mutex_lock(&mutex);//protected write for first client to enter in their card and modifies the board
          status = write(newsockfd, buffer, strlen(buffer)); 
          //pthread_mutex_unlock(&mutex);  
          while(reading) //loop for checking if taken or not
          {
            first_check = false;
            if(taken)
            {
              first_choice_valid=0;
              reading = false;
              first_check = true;
            }
          }
      }
      second_check = true; //enables 2nd print
      first_check = false; //closes 1st print
      reading = true;
      if (status < 0)
      {
          printf("error while sending client message to server\n");
      }
      bool second_choice_valid = false;
      while (!second_choice_valid) //second card,**NOTE** we want it to print out again after each time a new read happens which is the difficult part
      {
          reading = true;
          taken = false;
          //printf("Enter second card: \n");
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
          status = write(newsockfd, buffer, strlen(buffer));
          while(reading)//loop for checking if taken or not
          {
            second_check = false;
            if(taken)
            {
              second_choice_valid=0;
              reading = false;
              second_check = true;
            }
          }
      }
      third_check = true; //prevents 1st print to early
      second_check = false; //prevents 2nd print to early
      //pthread_mutex_lock(&mutex); //protected write for first client to enter in their card and modifies the board
     // pthread_mutex_unlock(&mutex);
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
    
    bzero(buffer, 256);
    status = read(socketid, buffer, 13);
    printf("%s", buffer);
    char * game_mode;
    strcpy(game_mode,buffer);  //reads the game mode type

    bzero(buffer, 256);
    status = read(socketid, buffer, 18); //reads the player # and prints it
    printf("%s", buffer);
    printf("Send message \'ready\' to begin game.\n", 36);
    int res = 1;
    while (res != 0)
    {
        bzero(buffer, 256); //starts socket
        fgets(buffer, 255, stdin);

        res = strcmp(buffer, "ready\n"); // ready check
    }
    status = write(socketid, buffer, strlen(buffer));
    if (status < 0)
    {
        printf("error while sending client message to server\n");
    }
    res = strcmp(game_mode,"Turn Based  \n");
    if(res == 0)
    {
      int res_quit = 1;
      while (res_quit != 0)
      {
          int res_wait = 1;
          while (res_wait != 0)
          {
              int count1 = 0;
              int count2 = 0;
              bzero(buffer, 256);
              status = read(socketid, buffer, 39);
              if (status < 0)
              {
                  perror("error while reading message from server");
                  exit(1);
              }
              char buffer3[256];
              while (count1 < 28)
              {
                  buffer3[count1] = buffer[count1]; //buffer for potential start of turn
                  count1++;
              }
              buffer3[28] = '\0';
              
              char buffer4[256]; // buffer for potential winner string
              while (count2 < 11)
              {
                  buffer4[count2] = buffer[count2];
                  count2++;
              }
              buffer4[11] = '\0';
              res_wait = strcmp(buffer3, "It is the start of your turn");
              res_quit = strcmp(buffer4, "Winner!   \n");
              if (res_wait == 0)
              {printf("%s\n", buffer3);} //turn start printed here
              else if (res_quit == 0)
              {break;} //other player won :(
              else
              {printf("%s\n", buffer);} //please wait is printed here
          }
          if(res_quit == 0)
          {break;}
          status = read(socketid, buffer, 255);
          if (status < 0)
          {
              perror("error while reading message from server");
              exit(1);
          }
          printf("%s\n", buffer); //read board
  
          bool first_choice_valid = false;
          char f;
          while (!first_choice_valid)
          {
              printf("Enter first card: "); //writes first card
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
  
          status = write(socketid, buffer, strlen(buffer));
          if (status < 0)
          {
              printf("error while sending client message to server\n");
          }
  
          bzero(buffer, 256);
          status = read(socketid, buffer, 39);
          if (status < 0)
          {
              perror("error while reading message from server");
              exit(1);
          }
          printf("BOARD:\n%s\n", buffer); // game board after first selection
  
          bool second_choice_valid = false;
          while (!second_choice_valid)
          {
              printf("Enter second card: "); //writes second card
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
  
          status = write(socketid, buffer, strlen(buffer));
          if (status < 0)
          {
              printf("error while sending client message to server\n");
          }
  
          bzero(buffer, 256);
          status = read(socketid, buffer, 39);
          if (status < 0)
          {
              perror("error while reading message from server");
              exit(1);
          }
          printf("BOARD:\n%s\n", buffer); // game board after second selection
  
          bzero(buffer, 256);
          status = read(socketid, buffer, 11);
          if (status < 0)
          {
              perror("error while reading message from server");
              exit(1);
          }
          res_quit = strcmp(buffer, "Winner!   \n");
          if (res_quit != 0)
          {
              printf("Choice is: %s\n", buffer); //reads 'Correct!' or 'Incorrect!' message
          }
      }
    }
    else
    {
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_mutex_init(&mutex, NULL);
      
      pthread_create(&th1, &attr, *write_connection, &socketid); //creates both threads, one for writing and the other for reading
      pthread_create(&th2, &attr, *listen_connection, &socketid);
      
      pthread_join(th1, NULL);
      pthread_join(th2, NULL);
    }
    status = read(socketid, buffer, 18);
    printf("\n%s\n", buffer);//prints winner message

    /* this closes the socket*/
    close(socketid);
}