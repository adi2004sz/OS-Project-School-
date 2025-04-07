#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_USERNAME_LEN 31
#define MAX_CLUE_LEN 127

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN + 1];
    double latitude;
    double longitude;
    char clue[MAX_CLUE_LEN + 1];
    int value;
} Treasure;

int main(){

    return 0;
}

//ex treasure_manager --remove 'game7' 'treasure2'
