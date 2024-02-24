#pragma once
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <sqlite3.h>

#define OFFSET 23
#define LGMAX 1000 //lungimea maxima a request-ului trimis de client

extern sqlite3 *myDataBase;

using namespace std;

typedef bool (*srcFunction)(const char*, char*);
extern map<string, srcFunction> srcKeywords;

bool existsUser(const char* usrname);
bool isPasswdCorrect(const char* usrname, const char* passwd);
int getUserId(const char* usrname);
bool insertUser(const char* usrname, const char* passwd);
bool getGenres(char* response);
bool getSubgenres(char* response);
bool getAuthors(char* response);
bool getBookPath(int bookId, char* path);
bool getBooks(vector<string> &recs);

bool searchAuthor(const char* name, char* response);
bool searchTitle(const char* title, char* response);
bool searchGenre(const char* genre, char* response);
bool searchSubgenre(const char* subgenre, char* response);
bool searchISBN(const char* isbn, char* response);
bool searchYear(const char* year, char* response);

void decodePasswd(const char* encoded, char* decoded);
void encodePasswd(const char* decoded, char* encoded);
