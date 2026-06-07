#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    int sum = 0;
    if (argc < 101) {
        for (int i =1; i<argc; i++) {
            char* numbers = "1234567890";
            int num = 0;
            char* arg = argv[i];

            int length = strlen(arg);

            for (int j=0;j<length;j++) {
                if (arg[j] == 'i' || arg[j] == 'I') {
                    num+=1;
                } else if (arg[j] == 'v' || arg[j] == 'V') {
                    num+=5;
                } else if (arg[j] == 'x' || arg[j] == 'X') {
                    num+=10;
                } else if(arg[j] == 'l' || arg[j] == 'L') {
                    num += 50;
                } else {
                    for (int k=0;k<11;k++) {
                        if (arg[j] == numbers[k]) {
                            num = atoi(arg);
                            break;
                        }
                    }
                }
            }
            if ((arg[length-2] == 'i' || arg[length-2] == 'I') && ((arg[length-1] == 'v' || arg[length-1] == 'V') || (arg[length-1] == 'x' || arg[length-1] == 'X'))) {
                num-=2;
            }

            if (arg[0] == '-') {
                num *= -1;
            } 

            if (num >= -1000 && num <= 1000) {
                sum += num;
            }
        }
        printf("%d", sum);
    } else {
        printf("%s\n", "Number of arguments > 100!");
    }
    
    return 0;
}
