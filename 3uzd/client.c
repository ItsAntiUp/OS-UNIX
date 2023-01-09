#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_LENGTH 10

//Constant
#define COLOR_NORMAL  "\x1B[0m"
#define COLOR_WHITE   "\x1B[37m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"

#define MSG_USAGE "USAGE: %s <ip> <port>\n"
#define MSG_GUESS_TEXT "(%d guesses left) Enter your guess - a word that consists of %d letters:\n"
#define MSG_WORD_TOO_SHORT "Word too short!\n"
#define MSG_WORD_TOO_LONG "Word too long!\n"
#define MSG_NOT_A_WORD "Not a word!\n"
#define MSG_YOU_WON "\nYou won! The correct word was - %s.\n"
#define MSG_GAME_OVER "\nGame Over! The correct word was - %s.\n"

#define ERR_INVALID_PORT "ERROR #1: invalid port specified.\n"
#define ERR_CANNOT_CREATE "ERROR #2: cannot create socket.\n"
#define ERR_INVALID_IP "ERROR #3: Invalid remote IP address.\n"
#define ERR_CANNOT_CONNECT "ERROR #4: error in connect().\n"
#define ERR_INCORRECT_DATA_SENT "ERROR #5: the sent data is incorrect.\n"
#define ERR_INCORRECT_DATA_RECEIVED "ERROR #6: the received data is incorrect.\n"

int main(int argc, char *argv[]){
    unsigned int port;
    int s_socket;
    int s_len;
    int r_len;

    //Struct of server address
    struct sockaddr_in servaddr;

    char buffer[BUFFER_LENGTH];
    char tempBuffer[BUFFER_LENGTH];

    if(argc != 3){
        fprintf(stderr, MSG_USAGE, argv[0]);
        return -1;
    }

    //Converting the third argument (port) from ascii to integer
    port = atoi(argv[2]);

    if((port < 1) || (port > 65535)){
        fprintf(stderr, ERR_INVALID_PORT);
        return -1;
    }

    //Creating the socket
    if((s_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, ERR_CANNOT_CREATE);
        return -1;
    }

    //Clearing the server's struct
    memset(&servaddr, 0, sizeof(servaddr));

    //Specifying the protocol: IPv4
    servaddr.sin_family = AF_INET;

    //Specifying the port
    servaddr.sin_port = htons(port);

    ///Converting the ip to numerical form
    if(inet_aton(argv[1], &servaddr.sin_addr) <= 0){
        fprintf(stderr, ERR_INVALID_IP);
        return -1;
    }

    //Connecting to the server
    if(connect(s_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr, ERR_CANNOT_CONNECT);
        return -1;
    }

    memset(&buffer, 0, BUFFER_LENGTH);

    //Receiving the word length, the guess count and the symbols from server
    if((r_len = recv(s_socket, buffer, BUFFER_LENGTH, 0)) <= 0){
        fprintf(stderr, ERR_INCORRECT_DATA_RECEIVED);
        close(s_socket);
        return -1;
    }

    // Constants that will be received from the server
    int guesses = buffer[0] - '0';
    const int WORD_LEN = buffer[1] - '0';
    const char CORRECT_SYMBOL_AND_POSITION = buffer[2];
    const char CORRECT_SYMBOL_NOT_POSITION = buffer[3];
    const char INCORRECT_SYMBOL = buffer[4];
    const char END_SYMBOL = buffer[5];

    for(;;){
        printf(MSG_GUESS_TEXT, guesses, WORD_LEN);
        fgets(buffer, BUFFER_LENGTH, stdin);
        int temp_len = strlen(buffer);

        // Checking if the word is the correct length
        if(temp_len < WORD_LEN + 1){
            fprintf(stderr, MSG_WORD_TOO_SHORT);
            continue;
        }

        if(temp_len > WORD_LEN + 1){
            fprintf(stderr, MSG_WORD_TOO_LONG);
            continue;
        }

        // Checking if the word contains only the characters from the alphabet
        int i;
        int is_a_word = 1;
        for(i = 0; i < temp_len - 1; ++i){
            if((buffer[i] < 'a' || buffer[i] > 'z') && (buffer[i] < 'A' || buffer[i] > 'Z')){
                is_a_word = 0;
                break;
            }
        }

        // If it is not a word
        if(is_a_word == 0){
            fprintf(stderr, MSG_NOT_A_WORD);
            continue;
        }

        //Sending the guess to server
        if((s_len = send(s_socket, buffer, temp_len, 0)) != temp_len){
            fprintf(stderr, ERR_INCORRECT_DATA_SENT);
            break;
        }

        // Copying the buffer
        memcpy(tempBuffer, buffer, temp_len);

        // Clearing buffer
        memset(&buffer, 0, BUFFER_LENGTH);
        --guesses;

        // Receiving the server's answer
        if((r_len = recv(s_socket, buffer, BUFFER_LENGTH, 0)) <= 0){
            fprintf(stderr, ERR_INCORRECT_DATA_RECEIVED);
            break;
        }

        // Checking if the game has been won
        int is_game_won = 1;
        for(i = 0; i < WORD_LEN; ++i){
            if(buffer[i] != CORRECT_SYMBOL_AND_POSITION){
                is_game_won = 0;
                break;
            }
        }

        for(i = 0; i < WORD_LEN; ++i){
            if(buffer[i] == CORRECT_SYMBOL_AND_POSITION)
                printf("%s%c", COLOR_GREEN, tempBuffer[i]);
            else if(buffer[i] == CORRECT_SYMBOL_NOT_POSITION)
                printf("%s%c", COLOR_YELLOW, tempBuffer[i]);
            else if(buffer[i] == INCORRECT_SYMBOL)
                printf("%s%c", COLOR_WHITE, tempBuffer[i]);
            else
                printf("%s%c", COLOR_NORMAL, tempBuffer[i]);
        }

        printf("%s\n", COLOR_NORMAL);

        // If the game has been won or the guess count reached zero
        if(is_game_won == 1 || guesses == 0){
            memset(&buffer, 0, BUFFER_LENGTH);
            buffer[0] = END_SYMBOL;

            // Sending an indication to server that the game ended
            if((s_len = send(s_socket, buffer, 1, 0)) != 1){
                fprintf(stderr, ERR_INCORRECT_DATA_SENT);
                break;
            }

            // Receiving the correct word of the day from the server
            if((r_len = recv(s_socket, buffer, BUFFER_LENGTH, 0)) <= 0){
                fprintf(stderr, ERR_INCORRECT_DATA_RECEIVED);
                break;
            }

            if(is_game_won == 1)
                printf(MSG_YOU_WON, buffer);
            else
                printf(MSG_GAME_OVER, buffer);  

            break;
        }
    }

    //Closing the socket
    close(s_socket);
    return 0;
}
