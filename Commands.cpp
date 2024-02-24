#include "Commands.h"

string commands[] = {"login", "logout", "recommend", "download", "register", "exit", "search", 
                    "help", "genres", "subgenres", "authors"};
map <string, cmdHandle> M = { {"login", myLogin}, {"logout", logout}, {"recommend", recommend},
                              {"download", download}, {"register", myRegister}, {"exit", exit},
                              {"search", search}, {"help", help}, {"genres", genres},
                              {"subgenres", subgenres}, {"authors", authors}
                            };
const char sentinel[] = "!SENTINEL!";

char* myLogin(const char* str, const int client) {
  char* response = new char[LGMAX];
  char usrname[30] = "", passwd[30] = "";
  bzero(response, LGMAX);

  if (isUserLogged[client] != -1) {
    strcpy(response, "User is already logged in. You need to logout in order to switch accounts.\n");
    return response;
  }

  if (checkNrTokens(str, 3) == 0) {
    strcpy(response, "Wrong syntax!\nFormat: login <username> <password>\n");
    return response;
  }

  extractToken(str, usrname, 2);
  extractToken(str, passwd, 3);

  if (!existsUser(usrname)) {
    strcpy(response, "Username doesn't exist.\n");
    return response;
  }

  if (!isPasswdCorrect(usrname, passwd)) {
    strcpy(response, "Wrong password.\n");
    return response;
  }

  int userID = getUserId(usrname);

  if (userID <= 0) {
    strcpy(response, "Login failed.\n");
    return response;
  }

  if (loggedUserIds.find(userID) != loggedUserIds.end()) {
    strcpy(response, "Account already in use. Cannot login at the moment.\n");
    return response;
  }

  pthread_mutex_lock(&loggedMutex);
  isUserLogged[client] = userID;
  loggedUserIds.insert(userID);
  pthread_mutex_unlock(&loggedMutex);

  strcpy(response, "Login successful.\n");

  return response;
}

char* logout(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (isUserLogged[client] == -1) {
    strcpy(response, "No user logged.\n");
    return response;
  }

  pthread_mutex_lock(&loggedMutex);
  loggedUserIds.erase(isUserLogged[client]);
  isUserLogged[client] = -1;
  pthread_mutex_unlock(&loggedMutex);

  strcpy(response, "User logged out.\n");
  return response;
}

char* recommend(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (isUserLogged[client] == -1) {
    strcpy(response, "User not logged in.\nCannot perform recommend.\n");
    return response;
  }

  if (checkNrTokens(str, 1) == 0) {
    strcpy(response, "Wrong syntax.\nFormat: recommend\n");
    return response;
  }

  vector<string> recs;
  if (getBooks(recs) == 0) {
    strcpy(response, "Internal database query error.\n");
    return response;
  }

  random_device rd;
  mt19937 mt(rd());

  shuffle(recs.begin(), recs.end(), mt);
  strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

  for (int i = 0; i < 5; i++) {
    strcat(response, recs[i].c_str());
  }

  return response;
}

char* download(const char* str, const int client)  {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (isUserLogged[client] == -1) {
    strcpy(response, "User not logged in.\nCannot perform download.\n");
    return response;
  }

  if (checkNrTokens(str, 2) == 0) {
    strcpy(response, "Wrong syntax.\nFormat: download <book_id>\n");
    return response;
  }

  // strcpy(response, "Inside download!\n");
  // return response;

  char bookIdStr[LGMAX] = "";
  extractToken(str, bookIdStr, 2);
  int bookId = atoi(bookIdStr);

  char path[LGMAX] = "";
  if (getBookPath(bookId, path) == 0) {
    strcpy(response, "Internal database query error.\n");
    return response;
  }

  if (path[0] == 0) {
    strcpy(response, "Cannot find path. Check if book_id is correct.\n");
    return response;
  }

  FILE *fd = fopen(path, "rb");

  if (fd == NULL) {
    strcpy(response, "Error opening file.\n");
    return response;
  }

  strcpy(response, "downloading...\n");
  myWrite(client, response, LGMAX);

  int bytes_read;
  while (true) {
      if ((bytes_read = fread(response, 1, LGMAX, fd)) > 0) {
          myWrite(client, response, bytes_read);
      }
      else
          break;
  }

  myWrite(client, sentinel, sizeof(sentinel));
  fclose(fd);

  strcpy(response, ".");

  return response;
}

char* myRegister(const char* str, const int client)  {
  char* response = new char[LGMAX];
  char usrname[30] = "", passwd[30] = "", secondPasswd[30] = "";
  bzero(response, LGMAX);

  if (!checkNrTokens(str, 4)) {
    strcpy(response, "Wrong syntax.\nFormat: register <username> <password> <reenter password>\n");
    return response;
  }

  extractToken(str, usrname, 2);
  extractToken(str, passwd, 3);
  extractToken(str, secondPasswd, 4);

  if (existsUser(usrname)) {
    strcpy(response, "Username unavailable.\n");
    return response;
  }

  if (strcmp(passwd, secondPasswd) != 0) {
    strcpy(response, "Passwords don't match.\n");
    return response;
  }

  if (insertUser(usrname, passwd) == 0) {
    strcpy(response, "Cannot register user.\n");
    return response;
  }

  strcpy(response, "User registered.\n");
  return response;
}

char* exit(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (isUserLogged[client] != -1) {
    strcpy(response, "User logged out!\n");
  }

  pthread_mutex_lock(&loggedMutex);
  loggedUserIds.erase(isUserLogged[client]);
  isUserLogged.erase(client);
  pthread_mutex_unlock(&loggedMutex);

  strcat(response, "Server exited!\n");

  return response;
}

char* search(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  char keyword[LGMAX] = "";
  extractToken(str, keyword, 2);
  
  if (srcKeywords.find(keyword) == srcKeywords.end() || checkNrTokens(str, 2)) {
    strcpy(response, "Wrong syntax.\nFormat: search <keyword> user_input\nWhere keyword is one of \"author\", \"ISBN\", \"title\", \"genre\", \"subgenre\", \"year\".\n");
    return response;
  }

  if ((strcmp(keyword, "ISBN") == 0 || strcmp(keyword, "year") == 0) 
      && checkNrTokens(str, 3) == 0) {

        strcpy(response, "Error. Search command must take only one token as argument for the used keyword.\n");
        return response;
      }

  char userInput[LGMAX] = "";
  removeFirstKTokens(str, userInput, 2);

  if (srcKeywords[keyword](userInput, response) == 0) {
    strcpy(response, "Internal database query error.\n");
    return response;
  }

  return response;
}

void tokenize(vector<string>& searchTokens, const char* searchQuery) {
  char aux[LGMAX] = "", *p;
  strcpy(aux, searchQuery);
  searchTokens.clear();

  p = strtok(aux, " ");// sarim peste token-ul 'search'
  p = strtok(NULL, " ");
  while (p) {
    searchTokens.push_back(string(p));
    p = strtok(NULL, " ");
  }
}

char* help(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  cout<<"hello"<<'\n';

  strcat(response, "List of commands:\n");

  for (int i = 0, nr = sizeof(commands) / sizeof(string); i < nr; i++) {
    strcat(response, commands[i].c_str());
    strcat(response, "\n");
  }

  return response;
}

char* genres(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (checkNrTokens(str, 1) == 0) {
    strcpy(response, "Wrong syntax. The command is 'genres'.\n");
    return response;
  }

  if (getGenres(response) == 0) {
    strcpy(response, "Internal database query error.\n");
    return response;
  }

  return response;
}

char* subgenres(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (checkNrTokens(str, 1) == 0) {
    strcpy(response, "Wrong syntax. The command is 'genres'.\n");
    return response;
  }

  if (getSubgenres(response) == 0) {
    strcpy(response, "Internal database query error.\n");
    return response;
  }

  return response;
}

char* authors(const char* str, const int client) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  if (checkNrTokens(str, 1) == 0) {
    strcpy(response, "Wrong syntax. The command is 'genres'.\n");
    return response;
  }

  if (getAuthors(response) == 0) {
    strcpy(response, "Internal database query error.\n");
    return response;
  }

  return response;
}

size_t myRead(int sd, char* buff, size_t buffSize) {
    ssize_t total = 0;
    ssize_t bytes_read;

    while (total < buffSize) {
        bytes_read = read(sd, buff + total, buffSize - total);

        if (bytes_read > 0) {
            total += bytes_read;
        } else if (bytes_read == 0) {
            break;
        } else {
            return bytes_read;
        }
    }
    return total;
}

size_t myWrite(int sd, const char* buff, size_t buffSize) {
    ssize_t total = 0;
    ssize_t bytes_written;

    while (total < buffSize) {
        bytes_written = write(sd, buff + total, buffSize - total);

        if (bytes_written >= 0) {
            total += bytes_written;
        } else {
            return bytes_written;
        }
    }

    return total;
}