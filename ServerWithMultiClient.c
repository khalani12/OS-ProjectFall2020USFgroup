#include <string.h>
#include<stdio.h>
#include <pthread.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<time.h>
#include<stdbool.h>
#define PORTNUM  5220 /* the port number the server will listen to*/
#define DEFAULT_PROTOCOL 0  /*constant for default protocol*/
#define SEMKEY ((key_t) 400L)
#define NUM_CARDS 18
pthread_t th1,th2; //threads
pthread_mutex_t mutex;
struct Player{
   int player_num;
   int points;
};struct Player player[5];
char symbols[NUM_CARDS] = {'=', '=', '&', '&', '!', '!', '?', '?', '*', '*', '%', '%', '@', '@', '}', '}', '#', '#'};

struct Card{
   char hidden_symbol;
   char face_symbol;
   bool showing;
};
struct Card card_set[NUM_CARDS];

void shuffle_symbols(char *array, int size) {
    srand((unsigned)time(NULL));
    int i;
    for (i = 0; i < size - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (size - i) + 1);
        char temp = array[j];
        array[j] = array[i];
        array[i] = temp;
    }
}


/**
 * Function shuffles the symbols and assigns
 * symbols to 18 Card objects.
 * Face letters range from 'a' to 'r'
*/
void assign_cards() {
   shuffle_symbols(symbols, NUM_CARDS);
   char letter = 'a';
   int i;
   for(i = 0; i < NUM_CARDS; i++)
   {
      card_set[i].face_symbol = letter;
      card_set[i].hidden_symbol = symbols[i];
      card_set[i].showing = false;
      letter++;
   }
}

void pick_two(char first, char second){
  int findex = first - 97;
  int sindex = second - 97;
  if(card_set[findex].hidden_symbol == card_set[sindex].hidden_symbol)
  {
    printf("Correct\n");
    card_set[findex].hidden_symbol = 'X';
    card_set[sindex].hidden_symbol = 'X';
  }
  else
  {
    printf("WRONG\n");
    printf("%c ",card_set[findex].hidden_symbol);
    printf("%c\n",card_set[sindex].hidden_symbol);
  }
}

char buffer2[256];
void read_from (int sock) {
   int status;
   char buffer[256];
   bzero(buffer,256);
   status= read(sock,buffer,255);
   strcpy(buffer2,buffer);
   if (status < 0) {
      perror("ERROR reading from socket");
      exit(1);
   }
}

/**
 * function writes the current game board
 * to the given socket. 
*/
char board_str[(NUM_CARDS*2)+6];
void write_board () {
   int i, j = 0;
   int new_line = NUM_CARDS/3;
   for(i = 0; i < NUM_CARDS; i++)
   {
      if(card_set[i].showing == false)
      {board_str[j] = card_set[i].face_symbol;}
      else
      {board_str[j] = ' ';} // if we are showing symbol instead of just not printing when a card is matched, line becomes:
      //{board_str[j] = card_set[i].hidden_symbol;}
      j++;
      board_str[j] = ' ';
      j++;
      if((i+1)%new_line == 0)
      {
         board_str[j] = '\n';
         j++;
      }
   }
}
void * handle_connection(void* p_newsockfd)
{
     int newsockfd = *((int*)p_newsockfd);
     char buffer[256];
     int counter = 0;
     bool playing = false;
     int status;
     int res_quit = strcmp(buffer2,"quit\n");
     int count = 0;
     char wait_turn[255] = "Please wait your turn\n"; //first write waiting message
     status = write(newsockfd,wait_turn,strlen(wait_turn));
     pthread_mutex_lock(&mutex);
     char start_turn[255] = "It is the start of your turn\n"; //second write ready message
     strcat(start_turn,board_str);
     status = write(newsockfd,start_turn,strlen(start_turn));
     
     while(res_quit!=0)
     {
        read_from(newsockfd);
        
        int res_ready = strcmp(buffer2,"ready\n");
        if(res_ready != 0 && !playing) 
        {
           status= write(newsockfd, "Send message \'ready\' to begin game.\n", 36);

           if (status < 0) {
              perror("ERROR writing to socket");
              exit(1);
           }
           printf("Client must send message \'ready\' to begin game.\n");
        }
        else if(playing)
        { 
          char first[255];  //prints out the card chosen
          strcpy(first,buffer2);
          printf("%s\n",first);
          counter++;
          if(counter == 2)
            break;
        }
        else
        {
          playing = true;
        }
        res_quit = strcmp(buffer2,"quit\n");
        // bzero(buffer2,256);
     }
     char message2[255] = "End of turn\n";
     status = write(newsockfd,message2,strlen(message2));
     pthread_mutex_unlock(&mutex);
     return NULL;
}
int order = 0;
int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen, newsockfd2;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int status, pid;
   
   
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_mutex_init(&mutex,NULL);

   assign_cards(); // initializes game board IMPORTANT
   
    write_board(); //create the board
    printf("%s\n", board_str);
   
   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM,DEFAULT_PROTOCOL );

   
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = PORTNUM;
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   
   /* Now bind the host address using bind() call.*/

   status =  bind(sockfd, (struct sockaddr *) &serv_addr, sizeof	(serv_addr)); 

   if (status < 0) {
      perror("ERROR on binding");
      exit(1);
   }
   
   /* Now Server starts listening clients wanting to connect. No 	more than 5 clients allowed */
   
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   while (1) {
     if(order == 0)
     {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd < 0){
          perror("ERROR on accept");
          exit(1);
        }
        pthread_create(&th1, &attr, *handle_connection, &newsockfd);
        order++;
     }
     if(order == 1)
     {
        newsockfd2 = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd2 < 0){
          perror("ERROR on accept");
          exit(1);
        }
        pthread_create(&th2, &attr, *handle_connection, &newsockfd2);
        order--;
     }
      
   } /* end of while */
   pthread_mutex_destroy(&mutex); 
}