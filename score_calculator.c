#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_USERNAME_LEN 31
#define MAX_CLUE_LEN 127
#define TREASURE_FILE "treasures.dat"
#define MAX_USERS 100

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN + 1];
    double latitude;
    double longitude;
    char clue[MAX_CLUE_LEN + 1];
    int value;
} Treasure;

typedef struct {
    char username[MAX_USERNAME_LEN + 1];
    int total_value;
    int treasure_count;
} UserScore;

int main(){

    return 0;
}