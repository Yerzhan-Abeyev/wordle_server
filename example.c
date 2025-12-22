#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>  // For printf
#include <stdlib.h> // For rand() and srand()
#include <time.h>   // For time() to seed the generator
#include "hashmap.h"
#define WORD_LENGTH 5
#define MAX_TRIES 6
#define GREEN "\033[0;32m"
  #define YELLOW "\033[0;33m"
  #define BLUE "\033[0;34m"
  #define RESET "\033[0m"
int ifValidWord(char* a, FILE *fp);
void toLower(char* a);
void wordCheck(char* a,char* guess, char* correct);
void gameScreen(FILE* fp, int count, char* a, char* color, int qw);
void resultScreen(int i, char*b, FILE *s, int c, int d, int e);
char* chooseTarget(FILE* fp);
void game(char *word, FILE *as,FILE *fp, int clntsock, int*a);

static void die(const char *message)
{
    perror(message);
    exit(1); 
}
static FILE *input = NULL;
static FILE *fpView;
static FILE *fpAll  = NULL;
static FILE *fpSol= NULL;
static char *tempor = NULL;
static int* stata = NULL;
static int servsock = -1;
void sigint_handler(int sig) {
    printf("\nCaught Ctrl+C (SIGINT). Cleaning up...\n");

    // Close files if open
    if (fpAll != NULL) {
        fclose(fpAll);
        fpAll = NULL;
    }
    if (fpView != NULL) {
        fclose(fpView);
        fpView = NULL;
    }
    if (tempor!= NULL) {
        free(tempor);
        tempor = NULL;
    }
    if (stata!= NULL) {
        free(stata);
        stata = NULL;
    }


    if(input != NULL){
        fclose(input);
        input = NULL;
    }
    if (fpSol !=NULL) {
        fclose(fpSol);
        fpSol = NULL;
    }

    // Close socket if open
    if (servsock >= 0) {
        close(servsock);
        servsock = -1;
    }

    printf("Cleanup complete. Exiting.\n");
    exit(0);
}
int main(int argc, char **argv)
{
        // ignore SIGPIPE so that we don’t terminate when we call
    // send() on a disconnected socket.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        die("signal() failed");
    /*
     * open the database file specified in the command line
     */
if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        die("Failed to set SIGINT handler");
    }
    // standard checking
    if (argc != 4) {
        fprintf(stderr, "%s\n", "usage: wordle_server <server-port> <allWords> <solutionWords>");
        exit(1);
    }
    fpAll = fopen(argv[2],"rb");
    fpSol = fopen(argv[3],"rb");
    if(fpAll == NULL){
        die("allWords file failed to be opened");
    }
    if(fpSol == NULL){
        fclose(fpAll);
         die("solutionWords file failed to be opened");
        }
    //convert port
    unsigned short port = atoi(argv[1]);
    
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
     fclose(fpAll);
     fclose(fpSol);
        die("socket failed");}

    // Construct local address structure

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(port);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
     fclose(fpAll);
     fclose(fpSol);
        die("bind failed");}

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0){
     fclose(fpAll);
     fclose(fpSol);
        die("listen failed");}
    char line[1000];
        int r;
     int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;
    while(1){
        fprintf(stderr, "waiting for client ... ");

        clntlen = sizeof(clntaddr); // initialize the in-out parameter

        if ((clntsock = accept(servsock,
                        (struct sockaddr *) &clntaddr, &clntlen)) < 0)
            die("accept failed");

        // accept() returned a connected socket (also called client socket)
        // and filled in the client's address into clntaddr

        fprintf(stderr, "client ip: %s\n", inet_ntoa(clntaddr.sin_addr));
        input = fdopen(clntsock, "r");
        int status = 1;
        stata = (int*) malloc(sizeof(int)*2);
        stata[1] =0;
        stata[0] =0;
        r =  sprintf(line, "+-------------------+\n| WELCOME TO WORDLE |\n+-------------------+\n");
        if (send(clntsock, line, r, 0) < r) {                 fprintf(stderr, "ERR: send failed\n");
                break;


            }
        while(status){
                char *targetWord = chooseTarget(fpSol);
                if(targetWord == NULL){
                    break;
                }
                
                tempor = targetWord;
                stata[0] = stata[0] +1;
                game(targetWord,fpAll, input, clntsock, stata);
                free(tempor);
                sprintf(line, "Type \"AGAIN\" to continue, or anything else to exit\n");
                send(clntsock, line, strlen(line),0);
                fgets(line, sizeof(line), input);
                toLower(line);
                tempor = NULL;
               status =0;
                if(strcmp(line, "again\n") == 0){
                        status= 1;
                }
        
        }
       free(stata);
       stata = NULL; 
       if(tempor != NULL){
           free(tempor);
           tempor = NULL;
       }      
       fclose(input); 
       input = NULL;


    }
    fclose(fpAll);
    fclose(fpSol);




}
char* chooseTarget(FILE* fp){
    srand(time(NULL));
    int randomLine = rand() % 2315;  // 0 to 2314 inclusive

    fseek(fp, 0, SEEK_SET);
    fseek(fp, 6 * randomLine, SEEK_SET);

    char* word = malloc(6);
    if (word == NULL) {
        die("Memory allocation failed!\n");
        return NULL;
    }
    if (fread(word, 1, 6, fp) == 6) {
        word[5] = '\0';
        return word;
    } else {
        free(word);
        die("Failed to read word from file");
        return NULL;
    }
    // close both files
}

void game(char *guessedWord, FILE *dict, FILE *fp, int clntsock, int*stata){
    int count = 0;
    int guessed = 0;
    char word[7];
    char buf[100];
    char* screenStorage = "gamegrind";
    fpView = fopen(screenStorage, "w+b");
    gameScreen(fpView, count, "", "", clntsock);
    while(count < MAX_TRIES && guessed == 0 && clntsock){
        sprintf(buf,"Type 5 letter word: \n");
        if(send(clntsock, buf, strlen(buf),0) <= 0){
                break;
        }
        if (fgets(word, sizeof(word), fp) == NULL) {
            sprintf(buf, "Error reading input\n");
            send(clntsock, buf,strlen(buf),0);
            break;
        }
        size_t len = strlen(word);
        if (len > 0 && word[len-1] == '\n') {
            word[len-1] = '\0';
        }

        // Validate word length
        if (strlen(word) != 5) {
            sprintf(buf, "Word must be exactly 5 letters. Try again.\n");
            send(clntsock, buf,strlen(buf),0);
            continue;
        }

        toLower(word);

        // Check if word is in dictionary
        if (ifValidWord(word, dict) == 0) {
            sprintf(buf, "Invalid word '%s', try again: \n", word);
            send(clntsock, buf,strlen(buf),0);
            continue;
        }

        count++;
        char* color = malloc(6);
        char* temp = color;
        wordCheck(color, word, guessedWord);
        gameScreen(fpView, count, word, color, clntsock);



        if(strcmp(word,guessedWord)== 0){
            guessed = 1;
            stata[1] +=1;
            free(temp);
            break;}
        free(temp);
    }
    fclose(fpView);
    fpView = NULL;
    resultScreen(guessed, guessedWord, fp, stata[0], stata[1], clntsock);


}
void resultScreen(int guessed, char *guessedWord, FILE *fp, int gameCount, int winCount, int clntsock){
    char line[1000];
    int r = 0;
    if (guessed){

        r = sprintf(line, "+-------------------+\n|     YOU WON!!!    |\n+-------------------+\n| %c | %c | %c | %c | %c |\n+-------------------+\n|     GAME : %d      |\n+-------------------+\n|     WIN : %d       |\n+-------------------+\n", guessedWord[0], guessedWord[1],guessedWord[2],guessedWord[3],guessedWord[4], gameCount, winCount);
        if (send(clntsock, line, r, 0) < r) {                 fprintf(stderr, "ERR: send failed\n");
               


            }

    }
    else{
        r = sprintf(line, "+-------------------+\n|     YOU LOST!!!   |\n+-------------------+\n| %c | %c | %c | %c | %c |\n+-------------------+\n|     GAME : %d      |\n+-------------------+\n|     WIN : %d       |\n+-------------------+\n", guessedWord[0], guessedWord[1],guessedWord[2],guessedWord[3],guessedWord[4], gameCount, winCount);
        if (send(clntsock, line, r, 0) < r) {                 fprintf(stderr, "ERR: send failed\n");
              


            }

    }
}
void gameScreen(FILE* fp, int count, char* word, char* color, int clntsock){
    char* a = "+-------------------+\n";
    char* b = "|   |   |   |   |   |\n";
    char buf[70];
    if(count == 0){
    fputs(a,fp);}
        
    if (count != 0) {
      fseek(fp, 0, SEEK_END);
    
    for(int i = 0; i < 5; i++){
        if(color[i] =='g'){
                sprintf(buf, "| \033[0;32m%c\033[0m ", word[i]);

        }
        else if(color[i] == 'y'){
                sprintf(buf, "| \033[0;33m%c\033[0m ", word[i]);
        }
        else{
         sprintf(buf, "| \033[0;90m%c\033[0m ", word[i]);
        }
        fputs(buf, fp);
    }
    fputs("|\n", fp);
    fputs(a, fp); 
    }
    fseek(fp, 0, SEEK_SET);
    while(fgets(buf, sizeof(buf),fp)){
        send(clntsock, buf, strlen(buf), 0);
            }
    
    for (int i = 0; i < 6-count; i++){
        send(clntsock, b, strlen(b), 0);
        send(clntsock, a, strlen(a), 0);

}
}
int ifValidWord(char *word, FILE* fp){
    if (strlen(word) != 5){
        return 0;}
    char test[6];
    while(fread(test, 6,1 , fp)){
        test[5] = '\0';
        if(strcmp(test,word) == 0){
            fseek(fp,0,SEEK_SET);
            return 1;}



    }

    fseek(fp,0, SEEK_SET);
    return 0;
}


void toLower(char * str) {
    int i = 0;
    while (str[i]) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] |= 0x20; //Make lowercase
        }
        ++i;
    }
} 

void wordCheck(char* res,char* guess, char* correct){
        strcpy(res, "eeeee");
        Map * complementMap = Map_create( 26 );
        for(int i = 0; i < strlen(correct); i++){
            if(Map_contains(complementMap, correct[i], 26) != 0){
                Map_add(complementMap, correct[i], Map_get(complementMap, correct[i], 26)+1, 26);
            }
            else{
                Map_add( complementMap, correct[i], 1, 26);
            }
        }
        for(int i = 0; i < strlen(guess); i++){
             if(guess[i] == correct[i]){
                res[i] = 'g';
                 Map_add(complementMap, correct[i], Map_get(complementMap, correct[i], 26)-1, 26);
             }
        }
        for(int i = 0; i <strlen(guess); i++)
        {
            if(res[i] != 'g'){
                if(Map_contains(complementMap, guess[i], 26) != 0 &&  Map_get(complementMap, guess[i], 26) > 0){
                    res[i] = 'y';
                    Map_add(complementMap, guess[i], Map_get(complementMap, correct[i], 26)-1, 26);
                }
                else{
                    res[i] = 'b';
                }
            }
        }
        Map_destroy(complementMap, 26);
}
