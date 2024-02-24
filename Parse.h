#pragma once

void extractToken(const char* request, char* command, const int tokenIndex);
bool checkNrTokens(const char* command, const int nrTokens);
void removeFirstKTokens(const char* source, char* dest, const int k);
bool isNumber(const char* str);