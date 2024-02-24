#pragma once

#include <cstring>
#include "Parse.h"
#include "dataBase.h"
#include <pthread.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <unistd.h>
#include <random>
#include <algorithm>

using namespace std;

typedef char* (*cmdHandle)(const char*, const int);

extern string commands[];
extern map<string, cmdHandle> M;


extern pthread_mutex_t loggedMutex;
extern map <int, int> isUserLogged;
extern set <int> loggedUserIds;

char* myLogin(const char* str, const int client);
char* logout(const char* str, const int client);
char* recommend(const char* str, const int client);
char* download(const char* str, const int client);
char* myRegister(const char* str, const int client);
char* exit(const char* str, const int client);
char* search(const char* str, const int client);
char* help(const char* str, const int client);
char* genres(const char* str, const int client);
char* subgenres(const char* str, const int client);
char* authors(const char* str, const int client);

size_t myRead(int sd, char* buff, size_t buffSize);
size_t myWrite(int sd, const char* buff, size_t buffSize);