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
pthread_t th1,th2,th3,th4,th5; //threads
pthread_mutex_t mutex;
int order = 0;
void write_board();
struct Player{
   int player_num;
   int points;
   bool turn;
};struct Player p[5];
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
    card_set[findex].face_symbol = 'X';
    card_set[sindex].face_symbol = 'X';
  }
  else
  {
    printf("WRONG\n");
    printf("%c ",card_set[findex].hidden_symbol);
    printf("%c\n",card_set[sindex].hidden_symbol);
  }
  write_board();
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
void write_board() {
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
bool check = false;
void * handle_connection(void* p_newsockfd)
{
     int newsockfd = *((int*)p_newsockfd);
     char buffer[256];
     int status;
     int new_order = 0;
     
     read_from(newsockfd); //read for the ready
     int res_ready = strcmp(buffer2,"ready\n");
     if(res_ready != 0) 
     {
       status= write(newsockfd, "Send message \'ready\' to begin game.\n", 36);

       if (status < 0) {
          perror("ERROR writing to socket");
          exit(1);
       }
       printf("Client must send message \'ready\' to begin game.\n");
       read_from(newsockfd);
     }
     while(1){
       int count = 0;
       if(pthread_self() == th1)  //player1
       {
        printf("THREAD 1\n");
        new_order = 0;
       }
       if(pthread_self() == th2)  //player2
       {
        printf("THREAD 2\n");
        new_order = 1;
       }     
       if(pthread_self() == th3)  //player2
       {
        printf("THREAD 3\n");
        new_order = 2;
       }  
       if(pthread_self() == th4)  //player2
       {
        printf("THREAD 4\n");
        new_order = 3;
       }  
       if(pthread_self() == th5)  //player2
       {
        printf("THREAD 5\n");
        new_order = 4;
       }  
       char message1[255] = "Please wait for your turn\n";
       while(!p[new_order].turn)  //this is where players wait if its not their turn
       {
         if(count == 0)
         {
           status = write(newsockfd,message1,strlen(message1));   //sends please wait
           count++;
         }
         if(check && count == 1)
         {
           status = write(newsockfd,board_str,strlen(board_str)); //sends the board to other clients ***NOTE*** This is where you would send a new board showing the hidden symbols once that is implemented
           count++;
         }
         if(!check && count == 2)
         {
           status = write(newsockfd,board_str,strlen(board_str)); //send the board to other clients a 2nd time ***NOTE*** This is where you would send a new board showing the hidden symbols once that is implemented
           count++;
         }
       }
      char message2[255] = "It is the start of your turn\n"; //sends that its start of their turn and exits the client out of the loop they are in
      status = write(newsockfd, message2, strlen(message2));
       bool first_choice_valid = false;
       bool second_choice_valid = false;
       char f = '\0', s = '\0';
       while(!first_choice_valid || !second_choice_valid)
       {
         count = 0;
         char first[255];   
         char second[255];    //mutex lock which reads the input and saves it as strings
         read_from(newsockfd); 
         strcpy(first,buffer2);
         check = true; 
         read_from(newsockfd);    
         strcpy(second,buffer2); 
         check = false;       
       
         f = first[0];
         s = second[0];
         if(f > 96 && f < 123)
         {first_choice_valid = true;}
         else
         {status = write(newsockfd, "Not a valid letter for choice 1.");}
         if(s > 96 && s < 123)
         {second_choice_valid = true;}
         else
         {status = write(newsockfd, "Not a valid letter for choice 2.");}
       }
       pthread_mutex_lock(&mutex);
       pick_two(f,s); 
       pthread_mutex_unlock(&mutex);     
          
       
       p[new_order].turn = false; //makes current players turn complete
       if(new_order+1 < order+1)
       {
         p[new_order+1].turn = true;  //makes next players turn next
       }
       else
       {
         p[0].turn = true;     //restarts the player queue
       }
       printf("%s\n", board_str);   //prints the modified board server side
     }
     return NULL;
}

int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen, newsockfd2, newsockfd3, newsockfd4, newsockfd5;
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
   while(order < 5) //exists after 2 players have connected. ** will need to append in implementation later
   {
     if(order == 0)
     {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd < 0){
          perror("ERROR on accept");
          exit(1);
        }
        p[order].turn = true;   //sets p1's turn as on for being the first to connect
        pthread_create(&th1, &attr, *handle_connection, &newsockfd);   //creates thread function for p1
        order++;
     }
     if(order == 1)
     {
        newsockfd2 = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd2 < 0){
          perror("ERROR on accept");
          exit(1);
        }
        p[order].turn = false;     //sets p2's turn as not on
        pthread_create(&th2, &attr, *handle_connection, &newsockfd2);  //creates thread function for p2
        order++;
     }
     if(order == 2)
     {
        newsockfd3 = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd3 < 0){
          perror("ERROR on accept");
          exit(1);
        }
        p[order].turn = false;     //sets p3's turn as not on
        pthread_create(&th3, &attr, *handle_connection, &newsockfd3);  //creates thread function for p3
        order++;
     }
     if(order == 3)
     {
        newsockfd4 = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd4 < 0){
          perror("ERROR on accept");
          exit(1);
        }
        p[order].turn = false;     //sets p4's turn as not on
        pthread_create(&th4, &attr, *handle_connection, &newsockfd4);  //creates thread function for p4
        order++;
     }
     if(order == 4)
     {
        newsockfd5 = accept(sockfd, (struct sockaddr *) &cli_addr, 	&clilen);
        if(newsockfd5 < 0){
          perror("ERROR on accept");
          exit(1);
        }
        p[order].turn = false;     //sets p5's turn as not on
        pthread_create(&th5, &attr, *handle_connection, &newsockfd5);  //creates thread function for p5
        order++;
     }
   }
   pthread_join(th1,NULL);
   pthread_join(th2,NULL);
   pthread_join(th3,NULL);
   pthread_join(th4,NULL);
   pthread_join(th5,NULL);
      
   pthread_mutex_destroy(&mutex); 
}