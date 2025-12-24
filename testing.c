#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "game.h"

int main() {
    printf("%d\n", start());
    
    char *buf;
    printf("%d\n", check("power"));
    
    buf = screen();
    printf("%s\n", buf);
    
    // Clean up
    cleanAll();
    
    return 0;
}


