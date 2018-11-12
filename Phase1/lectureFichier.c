#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
  char * str = malloc(20);

  while(fgets(str, 20, file) != NULL) {
      printf("%s, %i\n", str, wordCount);
      text[wordCount] = malloc(20);
      strcpy(text[wordCount], str);
      wordCount++;
  }
  fclose(file);

  return;
}
