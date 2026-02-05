#include <stdio.h>
#include <string.h>
#include "html.h"
#include "game.h"

//convert from board to HTML code
char *convert(struct wordle_board *board, int status, char *word){
    static char buf[4096];
    char *ptr = buf;
    int remaining = sizeof(buf);
    ptr += snprintf(ptr, remaining,
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head>\n"
            "    <title>WORDLE</title>\n"
            "    <style>\n"
            "        body {\n"
            "            background-color: white;\n"
            "            margin: 0;\n"
            "            display: flex;\n"
            "            justify-content: center;\n"
            "            align-items: center;\n"
            "            min-height: 100vh;\n"
            "            font-family: Arial, sans-serif;\n"
            "        }\n"
            "        .game-container {\n"
            "            text-align: center;\n"
            "        }\n"
            "        .title {\n"
            "            color: black;\n"
            "            font-weight: bold;\n" 
            "            font-size: 36px;\n"
            "            letter-spacing: 4px;\n"
            "            margin-bottom: 30px;\n"
            "        }\n"
            "        .board {\n"
            "            display: flex;\n"
            "            flex-direction: column;\n"
            "            gap: 5px;\n"
            "        }\n"
            "        .row {\n"
            "            display: flex;\n"
            "            gap: 5px;\n"
            "        }\n"
            "        .tile {\n"
            "            width: 60px;\n"
            "            height: 60px;\n"
            "            border: 2px solid #3a3a3c;\n"
            "            display: flex;\n"
            "            justify-content: center;\n"
            "            align-items: center;\n"
            "            font-size: 32px;\n"
            "            font-weight: bold;\n"
            "            color: white;\n"
            "            text-transform: uppercase;\n"
            "        }\n"
            "        .tile.correct {\n"
            "            background-color: #6ca965;\n"
            "            border-color: #6ca965;\n"
            "        }\n"
            "        .tile.present {\n"
            "            background-color: #c8b653;\n"
            "            border-color: #c8b653;\n"
            "        }\n"
            "        .tile.absent {\n"
            "            background-color: #787c7f;\n"
            "            border-color: #787c7f;\n"
            "        }\n"
            "        .message {\n"
            "            margin-top: 50px;\n"
            "            color: black;\n"
            "            font-weight: bold;\n"
            "            font-size: 18px;\n"
            "            letter-spacing: 4px;\n"
            "        }\n"
            "        .lookup-form {\n"
            "            margin-top: 30px;\n"
            "        }\n"
            "    </style>\n"
            "</head>\n"
            "<body>\n"
            "    <div class=\"game-container\">\n"
            "        <div class=\"title\">WORDLE</div>\n"
            "        <div class=\"board\">\n");
    remaining =  sizeof(buf) - (ptr-buf);
    
    // copy from board to HTML
    for (int row = 0; row < 6; row++) {
        ptr += snprintf(ptr, remaining, "            <div class=\"row\">\n");
        remaining = sizeof(buf) - (ptr - buf);
        for (int col = 0; col < 5; col++) {
            char letter = board->letters[row][col];
            char color = board->colors[row][col];
            char css_class[20] = "tile";
            if (color == 'f') {
                strcpy(css_class, "tile filled");
            } else if (color == 'g') {
                strcpy(css_class, "tile correct");
            } else if (color == 'y') {
                strcpy(css_class, "tile present");
            } else if (color == 'b') {
                strcpy(css_class, "tile absent");
            }
            if (letter != '\0' && letter != ' ') {
                ptr += snprintf(ptr, remaining,
                        "                <div class=\"%s\">%c</div>\n",
                        css_class, letter);
            } else {
                ptr += snprintf(ptr, remaining,
                        "                <div class=\"%s\"></div>\n",
                        css_class);
            }
            remaining = sizeof(buf) - (ptr - buf);

        }
        ptr += snprintf(ptr, remaining, "            </div>\n");
        remaining = sizeof(buf) - (ptr - buf);
    }
    if(status < 2){
ptr += snprintf(ptr, remaining, 
        "</div>\n"
        "       <div class=\"lookup-form\">\n"
        "       <p>\n"
        "       <form method=GET action=/wordle>\n"
        "       guess: <input type=text name=key>\n"
        "       <input type=submit>\n"
        "       </form>\n"
        "       <p>\n"
        "</div>\n");}
remaining = sizeof(buf) - (ptr - buf);
    // If word is valid
    if(status == 1){
        ptr += snprintf(ptr, remaining,
                "    </div>\n"
                "</body>\n"
                "</html>\n");
    }
    // If word is not valid
    else if(status == 0){
        ptr += snprintf(ptr, remaining,
                "        <div class=\"message\" id=\"game-message\">"
                "              Invalid Word!"
                "      </div>"
                "    </div>\n"
                "</html>\n");
    }
    else if(status == 2){
        ptr += snprintf(ptr, remaining,
                "        <div class=\"message\" id=\"game-message\">"
                "              YOU LOST! Word was %s"
                "      </div>"
                "    </div>\n"
                "</html>\n", word);

    for(int row = 0; row<6; row++){

    for(int i = 0; i < 5; i++){
        board->letters[row][i] = NULL;
        board->colors[row][i] = NULL;

    }
    }
    }
    else if(status == 3){
        ptr += snprintf(ptr, remaining,
                "        <div class=\"message\" id=\"game-message\">"
                "              YOU WON!"
                "      </div>"
                "    </div>\n"
                "</html>\n");

    for(int row = 0; row<6; row++){

    for(int i = 0; i < 5; i++){
        board->letters[row][i] = NULL;
        board->colors[row][i] = NULL;

    }
    }
    }
    return buf;


}

