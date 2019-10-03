#include <stdio.h>
#include "parse.h"

int main (int argc, char **argv) {
    bash shell;
    shell.head = NULL;
    shell.stack_num = 0;
    shell.job_num = 0;
    if (argc == 1) {    
        while (1) {
            // Prompt
            printPrompt();
            // Input
            char buffer[BUFFER_SIZE] = {0};
            fgets(buffer, BUFFER_SIZE, stdin);
            // Parse
            parseInst(buffer,&shell);
        }
    } else {
        FILE *input = fopen(argv[1], "r");
        char buffer[BUFFER_SIZE] = {0};
        while (fgets(buffer, BUFFER_SIZE, input)) {
            parseInst(buffer,&shell);
        }
        fclose(input);
    }
    return 0;
}