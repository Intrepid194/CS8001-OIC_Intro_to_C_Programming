#include <stdio.h>
#include <stdlib.h>


int main () {

    int arr[6] = {1, 2, 3, 4, 5, 4};
    int len = 6;
    int n = arr[5];

    int end = len-1;
    int start = end - n;

    int temp = arr[end-1];
    for (int i = end-1; i>start; i--) {
        arr[i] = arr[i-1];
    }
    arr[start] = temp;
    // printf("%d", arr[start]);
    for (int i =0; i<len-1;i++) {
        printf("%d ", arr[i]);
    }
   // printf("%d", n);
    return 0;
}