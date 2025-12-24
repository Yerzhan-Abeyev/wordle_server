#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For rand() and srand()
#include <time.h>   // For time() to seed the generator
#include "hashmap.h"
#include "game.h"
#include "html.h"

#define WORD_LENGTH 5
#define MAX_TRIES 6

static int count =0;
static char *target = NULL;
static FILE *allWords = NULL;
static FILE *solWords = NULL;
struct wordle_board board;

// Exit process
void cleanAll(){
    if(target != NULL){
        free(target);
        target = NULL;
    }
    if(allWords != NULL){
        fclose(allWords);
        allWords = NULL;
    }
    if(solWords != NULL){
        fclose(solWords);
        solWords = NULL;
    }
}

// Initialization process
int start(){
    allWords = fopen("allWords.txt", "rb");
    solWords = fopen("solutionWords.txt", "rb");

    if (allWords == NULL) {
        printf("ERROR: Could not open allWords.txt\n");
        cleanAll();
        return 0;
    }
    if (solWords == NULL) {
        printf("ERROR: Could not open solWords.txt\n");
        cleanAll();
        return 0;}
    memset(&board, 0, sizeof(board));
    count = 0;    
    target = chooseTarget(solWords);
    if(target == NULL){
        cleanAll();
        return 0;

    }
    return 1;
}

// Choose target word
char* chooseTarget(FILE* fp){
    srand(time(NULL));
    int randomLine = rand() %  2315;  // 0 to 2314 inclusive

    fseek(fp, 0, SEEK_SET);
    fseek(fp, 6 * randomLine, SEEK_SET);

    char* word = malloc(6);
    if (word == NULL) {
        return NULL;
    }
    if (fread(word, 1, 6, fp) == 6) {
        word[5] = '\0';
        return word;
    } else {
        free(word);
        return NULL;
    }
    // close both files
}

// Return HTML code
char *screen(int a){
    return convert(&board, a);
}

// Check if word is valid and check its status
int check(char *guess){
    char word[6];
    char color[6];

    strcpy(word, guess);
    if(ifValidWord(word, allWords) == 0){
        return 0;
    }
    toLower(word);
    wordCheck(color, word, target);
    for(int i = 0; i < 5; i++){
        board.letters[count][i] = word[i];
        board.colors[count][i] = color[i];

    }
    count++;
    if(count == 6){
        return 2;
    }
    return 1;

}

// Check if the word is in dictionary
int ifValidWord(char *word, FILE* fp){
    if (strlen(word) != 5){
        return 0;}
    char test[6];
    fseek(fp,0, SEEK_SET);
    while(fread(test, 6,1 , fp)){
        test[5] = '\0';
        if(strcmp(test,word) == 0){
            fseek(fp,0,SEEK_SET);
            return 1;}



    }

    fseek(fp,0, SEEK_SET);
    return 0;
}

// Lower every letter
void toLower(char * str) {
    int i = 0;
    while (str[i]) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] |= 0x20; //Make lowercase
        }
        ++i;
    }
} 

// Check the status of the word
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
