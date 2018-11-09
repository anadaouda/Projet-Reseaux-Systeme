#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common_impl.h"

int nbMachines(char * path) {
  FILE * file = fopen(path, "r");

  int wordCount = 0;
  int wordLetterCount = 0;
  int nbLines = 0;
  char letter;

  do {
    letter = fgetc(file);

    if (letter == '\n') {
      nbLines++;
      wordCount++;
      wordLetterCount = 0;
    }
    else if (letter == ' ') {
      wordCount++;
      wordLetterCount = 0;
    }
    else {
      wordLetterCount++;
    }

  } while(!feof(file));

  fclose(file);

  return nbLines;
}


void nomMachines(char * path, char ** text) {
  FILE * file = fopen(path, "r");

  int wordCount = 0;
  int wordLetterCount = 0;
  char letter;

  text[wordCount] = malloc(20*sizeof(char));

  do {
    letter = fgetc(file);
    printf("%c", letter);

    if ((letter == '\n')||(letter == ' ')) {
      text[wordCount][wordLetterCount] = '\0';
      // printf("%s\n", text[wordCount]);
      // printf("%i %i\n", wordCount, wordLetterCount);
      wordCount++;
      wordLetterCount = 0;
      text[wordCount] = malloc(20*sizeof(char));
    }
    else {
      text[wordCount][wordLetterCount] = letter;
      wordLetterCount++;
    }
  } while(!feof(file));
  printf("%i\n",wordCount);
  //fclose(file);

  return;
}
