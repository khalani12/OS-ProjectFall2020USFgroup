#include <string.h>
#include<stdio.h>
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

#define NUM_CARDS 18

struct Player{
   int player_num;
   int points;
};

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
void write_board (int sock) {
   int status;
   int i, j = 0;
   char board_str[(NUM_CARDS*2)+6];
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
   printf("%s\n", board_str);
   board_str[j] = '\0';
   status= write(sock, board_str,(NUM_CARDS*2)+6);
   
   if (status < 0) {
      perror("ERROR writing to socket");
      exit(1);
   }
}

int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int status, pid;

   assign_cards(); // initializes game board IMPORTANT
   
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
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
		
      if (newsockfd < 0) {
         perror("ERROR on accept");
         exit(1);
      }
      
      /* Create child process */
      pid = fork();
		
      if (pid < 0) {
         perror("ERROR on fork");
         exit(1);
      }
      
      if (pid == 0) {
         /* This is the client process */
         int res_quit = strcmp(buffer2,"quit\n");
         while(res_quit!=0)
         {
            read_from(newsockfd);
            int res_ready = strcmp(buffer2,"ready\n");
            if(res_ready != 0)
            {
               status= write(newsockfd, "Send message \'ready\' to begin game.\n", 36);
   
               if (status < 0) {
                  perror("ERROR writing to socket");
                  exit(1);
               }
               printf("Client must send message \'ready\' to begin game.\n");
            }
            else
            {
               printf("%s", buffer2); // print ready
               // ideally we would have a function that takes u into gameplay, but for now it just prints the board 8~)
               // which also means that itll ask u to send a ready message for each turn rn
               // ALSO ideally all this logic won't be in main bc it looks super messy
               write_board(newsockfd);
               // once we accept choices from the user, we can use ASCII value comparions to
               // be sure choices are between 'a' and 'r'
            }
            res_quit = strcmp(buffer2,"quit\n");
            // bzero(buffer2,256);
         }
         close(sockfd);
         exit(0);
      }
      else {
         close(newsockfd);
      }
		
   } /* end of while */
}
