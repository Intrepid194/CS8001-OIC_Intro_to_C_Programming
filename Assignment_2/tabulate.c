#include <stdlib.h>
#include <stdio.h>

struct lettersCount {
    char letters[2];
    int count[26];
};

void addLetterCount(char character, struct lettersCount* countLetters);

void printLetters(struct lettersCount* countLetters);

int main(int argc, char* argv[]) {

    int i, j, length;

    struct lettersCount* countLs;
    countLs->letters[0] = 'ab';

    for (i=1; i <argc; i++) {
        char* word = argv[i];

        length = sizeof(word) / sizeof(word[0]);

        for (j=0; j<length; j++) {
            addLetterCount(word[j], countLs);
        };
    };

    printLetters(countLs);
    return 0;
};

void addLetterCount(char character, struct lettersCount* countLetters) {
    for(int i=0; i<27; i++) {
        if (character == countLetters->letters[i]) {
            countLetters->count[i]++;
            break;
        } else {
            countLetters->letters[i] = character;
            countLetters->count[i] = 1;
            break;
        }
    };
};

void printLetters(struct lettersCount* countLetters) {
    for (int i=0; i<27; i++) {
        printf("%c %d", countLetters->letters[i], countLetters->count[i]);
    };
};