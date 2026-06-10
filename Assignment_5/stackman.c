#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Only the characters 0, +, <, >, ?, _, ~, (, ) are valid. Treat all others as comments—ignore them.
int main(int argc, char* argv[]) {

    if (argc > 2) {
        printf("Error: Too many input arguments.");
        return 1;
    }

    if (argv[1] == "") {
        printf("Error: No filename was provided.");
        return 1;
    }

    const char* idx = strstr(argv[1], ".sm");

    if (idx == NULL) {
        printf("Error: File extension is not .sm, unable to open file.");
        return 1;
    }

    FILE* file;
    char* filename = argv[1];

    file = fopen(filename, "r");  

    if (file == NULL) {
        printf("Error: File does not exist or unable to open file.");
        return 1;
    }

    int stack[65536];

    char line[1024];

    fgets(line, sizeof(line), file);

    fclose(file);

    printf("%s", line);

    int line_length = 0;

    for (int i=0; i<sizeof(line); i++) {
        line_length++;
        printf("%c", line[i]);
        // if (line[i] == "/0") {
        //     break;
        // }
    }
    printf("%d", line_length);
    return 0;

}