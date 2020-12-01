#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#define PORTNUM 5239       /* the port number the server will listen to*/
#define DEFAULT_PROTOCOL 0 /* constant for default protocol */
#define SEMKEY ((key_t)400L)
#define NUM_CARDS 18

pthread_t th1, th2, th3, th4, th5;  //threads for clients in turn based, and for reading in free for all
pthread_t th6, th7, th8, th9, th10; //threads for writing to clients in free for all
pthread_mutex_t mutex;
int order = 0;
int ready_count = 0;

void write_board();

struct Player
{
    int player_num;
    int points;
    bool turn;
};
struct Player p[5];
char symbols[NUM_CARDS] = {'=', '=', '&', '&', '!', '!', '?', '?', '*', '*', '%', '%', '@', '@', '}', '}', '#', '#'};

struct Card
{
    char hidden_symbol;
    char face_symbol;
    bool showing;
};
struct Card card_set[NUM_CARDS];

struct two_sockets
{
    int p_newsockfd;
    int next_sock;
};

//char score_str[256];
/* Format
Player #1: 00
Player #2: 00
Player #3: 00
Player #4: 00
Player #5: 00
//*/
void print_scores()
{
    //memset(&score_str[0], 0, sizeof(score_str));
    int i;
    for (i = 0; i < order; i++)
    {
        printf("Player #%d: %02d\n", (p[i].player_num), p[i].points);
    }
}

int find_max_points()
{
    int i, max_idx = 0, max = p[0].points;
    for (i = 1; i < 5; i++)
    {
        if (p[i].points > max)
        {
            max = p[i].points;
            max_idx = i;
        }
    }
    return max_idx;
}

bool check_completion()
{
    int i, total = 0;
    for (i = 0; i < NUM_CARDS; i++)
    {
        if (card_set[i].face_symbol == 'X')
        {
            total++;
        }
    }
    if (total == NUM_CARDS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void shuffle_symbols(char *array, int size)
{
    srand((unsigned)time(NULL));
    int i;
    for (i = 0; i < size - 1; i++)
    {
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
void assign_cards()
{
    shuffle_symbols(symbols, NUM_CARDS);
    char letter = 'a';
    int i;
    for (i = 0; i < NUM_CARDS; i++)
    {
        card_set[i].face_symbol = letter;
        card_set[i].hidden_symbol = symbols[i];
        card_set[i].showing = false;
        letter++;
    }
}

bool pick_two(char first, char second, int player)
{
    int findex = first - 97;
    int sindex = second - 97;
    if (card_set[findex].hidden_symbol == card_set[sindex].hidden_symbol && card_set[findex].face_symbol != 'X')
    {
        printf("Correct\n");
        card_set[findex].showing = false;
        card_set[sindex].showing = false;
        card_set[findex].face_symbol = 'X';
        card_set[sindex].face_symbol = 'X';
        write_board();
        // p[order].points += 1;
        p[player].points += 1;
        return true;
    }
    else
    {
        printf("WRONG\n");
        printf("%c ", card_set[findex].hidden_symbol);
        printf("%c\n", card_set[sindex].hidden_symbol);
        write_board();
        return false;
    }
}

char buffer2[256];
void read_from(int sock)
{
    int status;
    char buffer[256];
    bzero(buffer, 256);
    status = read(sock, buffer, 255);
    strcpy(buffer2, buffer);
    if (status < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }
}

/**
 * function writes the current game board
 * to the given socket. 
*/
char board_str[(NUM_CARDS * 2) + 6];
void write_board()
{
    int i, j = 0;
    int new_line = NUM_CARDS / 3;
    for (i = 0; i < NUM_CARDS; i++)
    {
        if (card_set[i].showing == false)
        {
            board_str[j] = card_set[i].face_symbol;
        }
        else
        {
            board_str[j] = card_set[i].hidden_symbol;
        }
        j++;
        board_str[j] = ' ';
        j++;
        if ((i + 1) % new_line == 0)
        {
            board_str[j] = '\n';
            j++;
        }
    }
}
bool check = false;                             //what enables all sockets to be written to.
void *handle_connection_sync(void *socket_pack) //new thread function just for the randomized mode
{
    struct two_sockets *socks = (struct two_sockets *)socket_pack; //sock packet stuff
    int *sock1 = &(socks->p_newsockfd);
    int *sock2 = &(socks->next_sock);
    int newsockfd = *((int *)sock1);
    int next_socket = *((int *)sock2);
    char buffer[256];
    int status;
    int player;

    bzero(buffer, 256);
    //status = write(newsockfd, "Free-for-all\n", 255); //sends message of the game mode to client
    //status = write(newsockfd,"Send message \'ready\' to begin game.\n", 36);
    read_from(newsockfd); //read for the ready
    printf("%s", buffer);
    int res_ready = strcmp(buffer2, "ready\n");
    while (res_ready != 0)
    {
        status = write(newsockfd, "Send message \'ready\' to begin game.\n", 36);

        if (status < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
        printf("Client must send message \'ready\' to begin game.\n");
        read_from(newsockfd);
        res_ready = strcmp(buffer2, "ready\n");
    }
    ready_count++;
    if (ready_count < order) //waiting for all players
    {
        printf("%d out of %d players connected.\n", ready_count, order);
        //printf("Waiting for more players...\n");
        while (ready_count < order)
        {
            // *shrugs*
        }
    }
    if ((pthread_self() == th1 && order == 1) || (pthread_self() == th2 && order == 2) ||
        (pthread_self() == th3 && order == 3) || (pthread_self() == th4 && order == 4) ||
        (pthread_self() == th5 && order == 5))
    {
        char buffyyyy[2] = "\0";
        struct sockaddr_in serv_addr;
        char *host_addr = "127.0.0.1";
        int self_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (status < 0)
        {
            printf("Error : Could not create socket \n");
        }

        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTNUM);
        serv_addr.sin_addr.s_addr = inet_addr(host_addr);
        status = connect(self_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (status < 0)
        {
            printf(" error in connecting \n");
        }

        status = write(newsockfd, buffyyyy, strlen(buffyyyy));
        //status = shutdown(next_socket, SHUT_RD);
        close(next_socket);
    }
    if (pthread_self() == th1) //game start
    {
        printf("Game Start!\n");
        player = 0;
    }
    else if (pthread_self() == th2) //game start
    {
        player = 1;
    }
    else if (pthread_self() == th3) //game start
    {
        player = 2;
    }
    else if (pthread_self() == th4) //game start
    {
        player = 3;
    }
    else if (pthread_self() == th5) //game start
    {
        player = 4;
    }
    while (1)
    {
        bool wrong = true;
        char first[255];
        char second[255];
        char f;
        char s;
        while (wrong) //wrong loop which exists if choice is not taken
        {
            read_from(newsockfd); //takes in first card
            strcpy(first, buffer2);
            f = first[0];
            if (card_set[f - 97].showing || card_set[f - 97].face_symbol == 'X')
            {
                bzero(buffer, 256); //writes taken if board symbol is showing
                char *message = "Taken\n";
                status = write(newsockfd, message, strlen(message));
            }
            else
            {
                bzero(buffer, 256); //writes taken if board symbol is showing
                char *message = "Not Taken\n";
                status = write(newsockfd, message, strlen(message));
                wrong = false;
            }
        }

        pthread_mutex_lock(&mutex); //modifies the board
        card_set[f - 97].showing = true;
        write_board();
        pthread_mutex_unlock(&mutex);

        wrong = true;
        check = true; //switches the write function to true where it writes to all nodes the board once
        while (wrong) //wrong loop which exists if choice is not taken
        {
            read_from(newsockfd); //takes in second card
            strcpy(second, buffer2);
            s = second[0];
            if (card_set[s - 97].showing || card_set[f - 97].face_symbol == 'X')
            {
                bzero(buffer, 256); //writes taken if board symbol is showing
                char *message = "Taken\n";
                status = write(newsockfd, message, strlen(message));
            }
            else
            {
                bzero(buffer, 256); //writes taken if board symbol is showing
                char *message = "Not Taken\n";
                status = write(newsockfd, message, strlen(message));
                wrong = false;
            }
        }

        pthread_mutex_lock(&mutex);
        card_set[s - 97].showing = true; //modifies the board
        write_board();
        pthread_mutex_unlock(&mutex);

        check = true; //switches the write function to true where it writes to all nodes the board once

        pthread_mutex_lock(&mutex);
        bool correct_choice = pick_two(f, s, player); //checks if its right or wrong
        pthread_mutex_unlock(&mutex);
        if (correct_choice)
        {
            status = write(newsockfd, "Correct!  \n", 11);
        }
        else
        {
            status = write(newsockfd, "Incorrect!\n", 11);
        }
        card_set[f - 97].showing = false; //resets the showing of the board
        card_set[s - 97].showing = false;
        write_board();
        print_scores();

        bool complete = check_completion(); //checks if the game is complete
        if (complete)
        {
            break;
        }
    }
}
void *handle_connection_sync_write(void *p_newsockfd) //thread thats only purpose is to write to all sockets just for randomized mode
{
    int newsockfd = *((int *)p_newsockfd);
    char buffer[256];
    int status;
    while (1)
    {
        if (check)
        {
            status = write(newsockfd, board_str, 255);
            check = false;
        }
    }
}
void *handle_connection(void *socket_pack)
{
    struct two_sockets *socks = (struct two_sockets *)socket_pack;
    int *sock1 = &(socks->p_newsockfd);
    int *sock2 = &(socks->next_sock);
    int newsockfd = *((int *)sock1);
    int next_socket = *((int *)sock2);
    char buffer[256];
    int status;
    int new_order = 0;

    read_from(newsockfd); //read for the ready
    int res_ready = strcmp(buffer2, "ready\n");
    while (res_ready != 0)
    {
        status = write(newsockfd, "Send message \'ready\' to begin game.\n", 36);

        if (status < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }
        printf("Client must send message \'ready\' to begin game.\n");
        read_from(newsockfd);
        res_ready = strcmp(buffer2, "ready\n");
    }
    ready_count++;
    if (ready_count < order)
    {
        printf("%d out of %d players connected.\n", ready_count, order);
        //printf("Waiting for more players...\n");
        while (ready_count < order)
        {
            // *shrugs*
        }
    }
    if ((pthread_self() == th1 && order == 1) || (pthread_self() == th2 && order == 2) ||
        (pthread_self() == th3 && order == 3) || (pthread_self() == th4 && order == 4) ||
        (pthread_self() == th5 && order == 5))
    {
        char buffyyyy[2] = "\0";
        struct sockaddr_in serv_addr;
        char *host_addr = "127.0.0.1";
        int self_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (status < 0)
        {
            printf("Error : Could not create socket \n");
        }

        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTNUM);
        serv_addr.sin_addr.s_addr = inet_addr(host_addr);
        status = connect(self_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (status < 0)
        {
            printf(" error in connecting \n");
        }

        status = write(newsockfd, buffyyyy, strlen(buffyyyy));
        //status = shutdown(next_socket, SHUT_RD);
        close(next_socket);
    }
    if (pthread_self() == th1)
    {
        printf("Game Start!\n");
    }

    bool complete = check_completion();
    while (!complete)
    {
        int count = 0;
        if (pthread_self() == th1) //player1
        {
            //printf("THREAD 1\n");
            new_order = 0;
        }
        if (pthread_self() == th2) //player2
        {
            //printf("THREAD 2\n");
            new_order = 1;
        }
        if (pthread_self() == th3) //player3
        {
            //printf("THREAD 3\n");
            new_order = 2;
        }
        if (pthread_self() == th4) //player4
        {
            //printf("THREAD 4\n");
            new_order = 3;
        }
        if (pthread_self() == th5) //player5
        {
            //printf("THREAD 5\n");
            new_order = 4;
        }
        char message1[255] = "Please wait for your turn\n";
        while (!p[new_order].turn) //this is where players wait if its not their turn
        {
            if (count == 0)
            {
                status = write(newsockfd, message1, strlen(message1)); //sends please wait
                count++;
            }
            if (check && count == 1)
            {
                status = write(newsockfd, board_str, strlen(board_str)); //sends the board to other clients ***NOTE*** This is where you would send a new board showing the hidden symbols once that is implemented
                count++;
            }
            if (!check && count == 2)
            {
                status = write(newsockfd, board_str, strlen(board_str)); //send the board to other clients a 2nd time ***NOTE*** This is where you would send a new board showing the hidden symbols once that is implemented
                count++;
            }
            complete = check_completion();
            if (complete)
            {
                break;
            }
        }
        complete = check_completion();
        if (complete)
        {
            break;
        }
        char message2[255] = "It is the start of your turn12345678910"; //sends that its start of their turn and exits the client out of the loop they are in
        status = write(newsockfd, message2, strlen(message2));
        status = write(newsockfd, board_str, strlen(board_str));

        count = 0;
        char first[255];
        char second[255]; //mutex lock which reads the input and saves it as strings
        read_from(newsockfd);
        strcpy(first, buffer2);
        char f = first[0];
        card_set[f - 97].showing = true;
        write_board();
        status = write(newsockfd, board_str, strlen(board_str));

        check = true;
        read_from(newsockfd);
        strcpy(second, buffer2);
        check = false;
        char s = second[0];
        card_set[s - 97].showing = true;
        write_board();
        status = write(newsockfd, board_str, strlen(board_str));

        pthread_mutex_lock(&mutex);
        bool correct_choice = pick_two(f, s, new_order);
        pthread_mutex_unlock(&mutex);
        complete = check_completion();

        card_set[f - 97].showing = false;
        card_set[s - 97].showing = false;
        if (complete)
        {
            break;
        }
        else if (correct_choice)
        {
            status = write(newsockfd, "Correct!  \n", 11);
        }
        else
        {
            status = write(newsockfd, "Incorrect!\n", 11);
        }
        write_board();
        print_scores();

        p[new_order].turn = false; //makes current players turn complete
        if (new_order + 1 < order)
        {
            p[new_order + 1].turn = true; //makes next players turn next
        }
        else
        {
            p[0].turn = true; //restarts the player queue
        }
        printf("%s\n", board_str); //prints the modified board server side
    }

    int winner = find_max_points();
    status = write(newsockfd, "Winner!   \n", 11);
    char winner_message[18];
    snprintf(winner_message, sizeof(winner_message), "Player %d has won!\n", winner + 1);
    status = write(newsockfd, winner_message, strlen(winner_message));
    return NULL;
}

int main(int argc, char *argv[])
{
    //srand(time(0));
    //int num = (rand() %(2-1+1))+1;
    char num = '\0';
    int sockfd, newsockfd, portno, clilen, newsockfd2, newsockfd3, newsockfd4, newsockfd5;
    struct two_sockets *sock_pack1 = (struct two_sockets *)malloc(sizeof(struct two_sockets));
    struct two_sockets *sock_pack2 = (struct two_sockets *)malloc(sizeof(struct two_sockets));
    struct two_sockets *sock_pack3 = (struct two_sockets *)malloc(sizeof(struct two_sockets));
    struct two_sockets *sock_pack4 = (struct two_sockets *)malloc(sizeof(struct two_sockets));
    struct two_sockets *sock_pack5 = (struct two_sockets *)malloc(sizeof(struct two_sockets));
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int status, pid;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_mutex_init(&mutex, NULL);

    while (num != '1' && num != '2')
    {
        bzero(buffer, 256);
        printf("Turn based play (enter '1') or Free-for-all (enter '2'): ");
        fgets(buffer, 255, stdin);
        num = buffer[0];
    }
    assign_cards(); // initializes game board IMPORTANT

    int i;
    for (i = 0; i < NUM_CARDS; i++)
    {
        card_set[i].showing = true;
    }
    write_board(); //Displays answers on server side
    printf("%s\n", board_str);

    for (i = 0; i < NUM_CARDS; i++)
    {
        card_set[i].showing = false;
    }
    write_board(); //Displays flipped board on server side
    printf("%s\n", board_str);
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = PORTNUM;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/

    status = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (status < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now Server starts listening clients wanting to connect. No 	more than 5 clients allowed */

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    if (num == '1') //if num one its turn based
    {
        printf("Turn Based\n");
        //while (order < 5) //exists after 2 players have connected. ** will need to append in implementation later
        //{
        if (order == 0)
        {
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0)
            {
                perror("ERROR on accept");
                exit(1);
            }
            p[order].turn = true; //sets p1's turn as on for being the first to connect
            p[order].points = 0;
            p[order].player_num = order + 1;
            bzero(buffer, 256);
            status = write(newsockfd, "Turn Based  \n", 13);
            bzero(buffer, 256);
            status = write(newsockfd, "You are Player #1\n", 18);
            sock_pack1->p_newsockfd = newsockfd;
            sock_pack1->next_sock = sockfd;
            pthread_create(&th1, &attr, *handle_connection, (void *)sock_pack1); //creates thread function for p1
            order++;
        }
        if (order == 1)
        {
            newsockfd2 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd2 < 0)
            {
                perror("ERROR on accept");
                exit(1);
            }
            if (ready_count != order)
            {
                p[order].turn = false; //sets p2's turn as not on
                p[order].points = 0;
                p[order].player_num = order + 1;
                bzero(buffer, 256);
                status = write(newsockfd2, "Turn Based  \n", 13);
                bzero(buffer, 256);
                status = write(newsockfd2, "You are Player #2\n", 18);
                sock_pack2->p_newsockfd = newsockfd2;
                sock_pack2->next_sock = sockfd;
                pthread_create(&th2, &attr, *handle_connection, (void *)sock_pack2); //creates thread function for p2
                order++;
            }
        }
        if (order == 2)
        {
            newsockfd3 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd3 < 0)
            {
                perror("ERROR on accept");
                exit(1);
            }
            if (ready_count != order)
            {
                p[order].turn = false; //sets p3's turn as not on
                p[order].points = 0;
                p[order].player_num = order + 1;
                sock_pack3->p_newsockfd = newsockfd3;
                sock_pack3->next_sock = sockfd;
                bzero(buffer, 256);
                status = write(newsockfd3, "Turn Based  \n", 13);
                bzero(buffer, 256);
                status = write(newsockfd3, "You are Player #3\n", 18);
                pthread_create(&th3, &attr, *handle_connection, (void *)sock_pack3); //creates thread function for p3
                order++;
            }
        }
        if (order == 3)
        {
            newsockfd4 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd4 < 0)
            {
                perror("ERROR on accept");
                exit(1);
            }
            if (ready_count != order)
            {
                p[order].turn = false; //sets p4's turn as not on
                p[order].points = 0;
                p[order].player_num = order + 1;
                sock_pack4->p_newsockfd = newsockfd4;
                sock_pack4->next_sock = sockfd;
                bzero(buffer, 256);
                status = write(newsockfd4, "Turn Based  \n", 13);
                bzero(buffer, 256);
                status = write(newsockfd4, "You are Player #4\n", 18);
                pthread_create(&th4, &attr, *handle_connection, (void *)sock_pack4); //creates thread function for p4
                order++;
            }
        }
        if (order == 4)
        {
            newsockfd5 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd5 < 0)
            {
                perror("ERROR on accept");
                exit(1);
            }
            if (ready_count != order)
            {
                p[order].turn = false; //sets p5's turn as not on
                p[order].points = 0;
                p[order].player_num = order + 1;
                sock_pack5->p_newsockfd = newsockfd5;
                sock_pack5->next_sock = sockfd;
                bzero(buffer, 256);
                status = write(newsockfd5, "Turn Based  \n", 13);
                bzero(buffer, 256);
                status = write(newsockfd5, "You are Player #5\n", 18);
                pthread_create(&th5, &attr, *handle_connection, (void *)sock_pack5); //creates thread function for p5
                order++;
            }
        }
        //}
        printf("Joining threads\n");
        pthread_join(th1, NULL);
        close(newsockfd);
        if (order > 1)
        {
            close(newsockfd2);
            pthread_join(th2, NULL);
        }
        if (order > 2)
        {
            close(newsockfd3);
            pthread_join(th3, NULL);
        }
        if (order > 3)
        {
            close(newsockfd4);
            pthread_join(th4, NULL);
        }
        if (order > 4)
        {
            close(newsockfd5);
            pthread_join(th5, NULL);
        }
        printf("Threads have been joined\n");
    }
    else //if num is 2 its random
    {
        printf("Free-for-all\n");
        while (order < 5) //exists after 2 players have connected. ** will need to append in implementation later
        {
            if (order == 0)
            {
                newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd < 0)
                {
                    perror("ERROR on accept");
                    exit(1);
                }
                p[order].points = 0;
                p[order].player_num = order + 1;
                sock_pack1->p_newsockfd = newsockfd;
                sock_pack1->next_sock = sockfd;
                bzero(buffer, 256);
                status = write(newsockfd, "Free-for-all\n", 13);
                bzero(buffer, 256);
                status = write(newsockfd, "You are Player #1\n", 18);
                pthread_create(&th1, &attr, *handle_connection_sync, (void *)sock_pack1); //creates thread function for p1
                pthread_create(&th6, &attr, *handle_connection_sync_write, &newsockfd);   //creates thread function for writing the board
                order++;
            }
            if (order == 1)
            {
                newsockfd2 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd2 < 0)
                {
                    perror("ERROR on accept");
                    exit(1);
                }
                if (ready_count != order)
                {
                    p[order].points = 0;
                    p[order].player_num = order + 1;    
                    sock_pack2->p_newsockfd = newsockfd2;
                    sock_pack2->next_sock = sockfd;
                    bzero(buffer, 256);
                    status = write(newsockfd2, "Free-for-all\n", 13);
                    bzero(buffer, 256);
                    status = write(newsockfd2, "You are Player #2\n", 18);
                    pthread_create(&th2, &attr, *handle_connection_sync, (void *)sock_pack2); //creates thread function for p2
                    pthread_create(&th7, &attr, *handle_connection_sync_write, &newsockfd2);  //creates thread function for writing the board
                    order++;
                }
            }
            if (order == 2)
            {
                newsockfd3 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd3 < 0)
                {
                    perror("ERROR on accept");
                    exit(1);
                }
                if (ready_count != order)
                {
                    p[order].points = 0;
                    p[order].player_num = order + 1; 
                    sock_pack3->p_newsockfd = newsockfd3;
                    sock_pack3->next_sock = sockfd;
                    bzero(buffer, 256);
                    status = write(newsockfd3, "Free-for-all\n", 13);
                    bzero(buffer, 256);
                    status = write(newsockfd3, "You are Player #3\n", 18);
                    pthread_create(&th3, &attr, *handle_connection_sync, (void *)sock_pack3); //creates thread function for p3
                    pthread_create(&th8, &attr, *handle_connection_sync_write, &newsockfd3);  //creates thread function for writing the board
                    order++;
                }
            }
            if (order == 3)
            {
                newsockfd4 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd4 < 0)
                {
                    perror("ERROR on accept");
                    exit(1);
                }
                if (ready_count != order)
                {
                    p[order].points = 0;
                    p[order].player_num = order + 1; 
                    sock_pack4->p_newsockfd = newsockfd4;
                    sock_pack4->next_sock = sockfd;
                    bzero(buffer, 256);
                    status = write(newsockfd4, "Free-for-all\n", 13);
                    bzero(buffer, 256);
                    status = write(newsockfd4, "You are Player #4\n", 18);
                    pthread_create(&th4, &attr, *handle_connection_sync, (void *)sock_pack4); //creates thread function for p4
                    pthread_create(&th9, &attr, *handle_connection_sync_write, &newsockfd4);  //creates thread function for writing the board
                    order++;
                }
            }
            if (order == 4)
            {
                newsockfd5 = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd5 < 0)
                {
                    perror("ERROR on accept");
                    exit(1);
                }
                if (ready_count != order)
                {
                    p[order].points = 0;
                    p[order].player_num = order + 1; 
                    sock_pack5->p_newsockfd = newsockfd5;
                    sock_pack5->next_sock = sockfd;
                    bzero(buffer, 256);
                    status = write(newsockfd5, "Free-for-all\n", 13);
                    bzero(buffer, 256);
                    status = write(newsockfd5, "You are Player #5\n", 18);
                    pthread_create(&th5, &attr, *handle_connection_sync, (void *)sock_pack5); //creates thread function for p5
                    pthread_create(&th10, &attr, *handle_connection_sync_write, &newsockfd5); //creates thread function for writing the board
                    order++;
                }
            }
        }
        printf("Waiting to join threads\n");
        pthread_join(th1, NULL);
        pthread_join(th6, NULL);
        close(newsockfd);
        if (order > 0)
        {
            pthread_join(th2, NULL);
            pthread_join(th7, NULL);
            close(newsockfd2);
        }
        if (order > 1)
        {
            pthread_join(th3, NULL);
            pthread_join(th8, NULL);
            close(newsockfd3);
        }
        if (order > 2)
        {
            pthread_join(th4, NULL);
            pthread_join(th9, NULL);
            close(newsockfd4);
        }
        if (order > 3)
        {
            pthread_join(th5, NULL);
            pthread_join(th10, NULL);
            close(newsockfd5);
        }
        printf("Threads have been joined\n");
    }
    free(sock_pack1);
    free(sock_pack2);
    free(sock_pack3);
    free(sock_pack4);
    free(sock_pack5);
    printf("Exiting server\n");
    close(sockfd);
    pthread_mutex_destroy(&mutex);
}