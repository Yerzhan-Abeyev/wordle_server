
#ifndef GAME_H
#define GAME_H


struct wordle_board {
      char letters[6][5];      // Letters in each cell (0 if empty)
    char colors[6][5];       // Color codes: ' '=empty, 'f'=filled, 'c'=correct, 'p'=present, 'a'=absent
};

void cleanAll();
int start();
char* chooseTarget(FILE* fp);
char *screen();
int ifValidWord(char *word, FILE* fp);
int check(char *guess);
void toLower(char * str);
void wordCheck(char* res,char* guess, char* correct);


#endif
