#include "test.h"

void makeTeste() {
   printf("hello \n");
}

// char **ReadWordSet() {

//   // Open file
//    char wordsfile[30];
//    sprintf(wordsfile, "palavras.txt"); 
//    FILE *f = fopen(wordsfile, "r");

//    // Count number of words on base
//    int nwords = 0;
//    char ch;
//    while(!feof(f)) {
//       ch = fgetc(f);
//       if(ch == '\n') {
//          nwords++;
//       }
//    }
//    fclose(f);
//    printf("nwords: %d\n", nwords);

//    // Register words on internal structure
//    char **words;
//    int i;
//    words = (char**) malloc(nwords*sizeof(char*));
//    for (i = 0; i < nwords; i++) {
//       words[i] = (char*) malloc(40*sizeof(char));
//    }  
//    // size_t *wordslen = (size_t*) malloc(nwords*sizeof(size_t));
//    // ssize_t read;
//    char aux[40];
//    f = fopen(wordsfile, "r");

//    if (f == NULL) {
//       perror("Error opening file");
//       return(-1);
//    }

//    for (i = 0; i < nwords; i++) {
      
//       printf("sizeof(words[%d]): %lu\n", i, sizeof(words[i]));

//       if (fgets(words[i], sizeof(words[i]), f) != NULL) {
//          printf("%s\n", words[i]);
//          // strcpy(words[i], aux);
//       }
//    }

//    fclose(f);
//    for (i = 0; i < nwords; i++) {
//       printf("words[%d]: %s\n", i, words[i]);
//    }

//    return (words);
// }