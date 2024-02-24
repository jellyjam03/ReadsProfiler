#include "Parse.h"

void extractToken(const char* request, char* token, const int tokenIndex) {
  int nrSpaces = 0, i = 0, j = 0;
  while (nrSpaces < tokenIndex - 1) {
    for (; request[i] != ' ' && request[i]; i++);
    for (; request[i] == ' ' && request[i]; i++);
    nrSpaces++;
  }

  for (j = i; request[j] != ' ' && request[j]; j++)
    token[j - i] = request[j];
  token[j - i] = 0;
}

bool checkNrTokens(const char* request, const int nrTokens) {
  int i, nrSpace = 0;

  for (i = 0; request[i]; i++)
    if (request[i] == ' ' && request[i+1] != ' ' && request[i+1] != 0)
      nrSpace++;

  return (nrSpace == nrTokens - 1);
}

void removeFirstKTokens(const char* source, char* dest, const int k) {
  int i, q, j;
  
  for(q = 0; source[q] == ' '; q++);

  for (i = 0; i < k; i++) {
    for (; source[q] != ' ' && source[q]; q++);
    for (; source[q] == ' ' && source[q]; q++);
  }

  for (j = q; source[j] != ' ' && source[j]; j++)
    dest[j - q] = source[j];
  dest[j - q] = 0;
}

bool isNumber(const char* str) {
  int i = 1;
  if (! ('1' <= str[0] && str[0] <= '9')) return 0;

  for (i = 1; '0' <= str[i] && str[i] <= '9'; i++);
  for (; str[i] == ' '; i++);
  
  return (str[i] == 0);
}