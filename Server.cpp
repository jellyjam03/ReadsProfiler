#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <map>
#include <string>
#include <pthread.h>
#include <queue>

#define PORT 2728
#define SERV_BACKLOG 100  //cati clienti pot astepta la rand la port in listen
#define NR_THREADS 20
#define LGMAX 100 //lungimea maxima a request-ului trimis de client

using namespace std;

typedef char* (*cmdHandle)(const char*);

char* myLogin(const char* str);
char* logout(const char* str);
char* recommend(const char* str);
char* download(const char* str);
char* myRegister(const char* str);
char* exit(const char* str);

string commands[] = {"login", "logout", "recommend", "download", "register", "exit"};
map <string, cmdHandle> M = { {"login", myLogin}, {"logout", logout}, {"recommend", recommend},
                              {"download", download}, {"register", myRegister}, {"exit", exit}
                            };

map <int, bool> isUserLogged;

struct clientInfo {
  int pclient;
  char request[LGMAX];
  fd_set* readfds;
};

pthread_t thread_pool[NR_THREADS];
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connectMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loggedMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

std::queue<clientInfo*> client_queue;

void* threadFunction(void* arg);
void handleConnection(clientInfo* pInfo);
void extract_command(const char* request, char* command);

/* programul */
int main ()
{
  struct sockaddr_in server;	/* structurile pentru server si clienti */
  struct sockaddr_in from;
  fd_set readfds;		/* multimea descriptorilor de citire */
  fd_set actfds;		/* multimea descriptorilor activi */
  struct timeval tv;		/* structura de timp pentru select() */
  int sd, client;		/* descriptori de socket */
  int optval=1; 			/* optiune folosita pentru setsockopt()*/ 
  int fd;			/* descriptor folosit pentru 
				   parcurgerea listelor de descriptori */
  int nfds;			/* numarul maxim de descriptori */
  int len;			/* lungimea structurii sockaddr_in */

  //cream threadurile care for da handle la comenzi
  for (int i = 0; i < NR_THREADS; i++) {
    pthread_create(&thread_pool[i], NULL, threadFunction, NULL);
  }

  /* creare socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server] Eroare la socket().\n");
      return errno;
    }

  /*setam pentru socket optiunea SO_REUSEADDR pentru a elibera portul imediat cum se inchide programul*/ 
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  /* pregatim structurile de date */
  bzero (&server, sizeof (server));

  /* umplem structura folosita de server */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);

  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1) {
    perror ("[server] Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, SERV_BACKLOG) == -1)  {
    perror ("[server] Eroare la listen().\n");
    return errno;
  }
  
  /* completam multimea de descriptori de citire */
  FD_ZERO (&readfds);		/* initial, multimea este vida */
  FD_SET (sd, &readfds);		/* includem in multime socketul creat */

  tv.tv_sec = 1;		/* se va astepta un timp de 1 sec. */
  tv.tv_usec = 0;
  
  /* valoarea maxima a descriptorilor folositi */
  nfds = sd;

  printf ("[server] Asteptam la portul %d...\n", PORT);
  fflush (stdout);

  while (1) {
    /* ajustam multimea descriptorilor activi (efectiv utilizati) */
    pthread_mutex_lock(&connectMutex);
    bcopy ((char *) &readfds, (char *) &actfds, sizeof (readfds));
    pthread_mutex_unlock(&connectMutex);

    /* apelul select() */
    if (select (nfds+1, &actfds, NULL, NULL, &tv) < 0) {
      perror ("[server] Eroare la select().\n");
      return errno;
    }
    /* vedem daca e pregatit socketul pentru a-i accepta pe clienti */
    if (FD_ISSET (sd, &actfds))  {
      /* pregatirea structurii client */
      len = sizeof (from);
      bzero (&from, sizeof (from));

      /* a venit un client, acceptam conexiunea */
      client = accept (sd, (struct sockaddr *) &from, (socklen_t*) &len);

      /* eroare la acceptarea conexiunii de la un client */
      if (client < 0) {
        perror ("[server] Eroare la accept().\n");
        continue;
      }

      pthread_mutex_lock(&loggedMutex);
      isUserLogged[client] = 0;
      pthread_mutex_unlock(&loggedMutex);

      if (nfds < client) /* ajusteaza valoarea maximului */
        nfds = client;
              
      /* includem in lista de descriptori activi si acest socket */
      pthread_mutex_lock(&connectMutex);
      FD_SET (client, &readfds);
      pthread_mutex_unlock(&connectMutex);

      fflush (stdout);
    }
    /* vedem daca e pregatit vreun socket client pentru a trimite raspunsul */
    for (fd = 0; fd <= nfds; fd++)  {	/* parcurgem multimea de descriptori */
      /* este un socket de citire pregatit? */
      if (fd != sd && FD_ISSET (fd, &actfds))
        {
          clientInfo* pInfo = new clientInfo;
          pInfo->pclient = fd;
          bzero(pInfo->request, LGMAX);
          if (read(fd, pInfo->request, LGMAX) < 0) {
            perror("[server] Eroare la read() de la client.\n");
            continue;
          }
          //pInfo->readfds = new fd_set; *pInfo->readfds = readfds;
          pInfo->readfds = &readfds;
          pthread_mutex_lock(&queueMutex);
          client_queue.push(pInfo);
          //pthread_cond_signal(&condition_var);
          pthread_mutex_unlock(&queueMutex);
          pthread_cond_signal(&condition_var);
        }
    }			/* for */
  }				/* while */
}				/* main */

void* threadFunction(void* arg) {
  while (true) {
    pthread_mutex_lock(&queueMutex);
    while (client_queue.empty()) {
      pthread_cond_wait(&condition_var, &queueMutex);
    }
    clientInfo* pInfo = client_queue.front();
    client_queue.pop();
    pthread_mutex_unlock(&queueMutex);
    
    if (pInfo != NULL) {
      handleConnection(pInfo);
    }

  }
}

void handleConnection(clientInfo* pInfo) {
  int client = pInfo->pclient;
  fd_set* readfds = pInfo->readfds;
  char request[LGMAX], response[LGMAX], command[LGMAX];

  strcpy(request, pInfo->request);

  delete pInfo;

  bzero(response, LGMAX);
  bzero(command, LGMAX);

  extract_command(request, command);
  if (M.find(command) == M.end()) {
    strcpy(response,"Eroare. Comanda introdusa nu este valida.\n");
  }
  else {
    char* presponse = M[command](request);
    strcpy(response, presponse);
    delete presponse;
  }

  if (write(client, response, sizeof(response)) < 0)  {
    perror("[server] Eroare la write() in client.\n");
    return;
  }

  if (strcmp(command, "exit") == 0) {
    pthread_mutex_lock(&connectMutex);
    FD_CLR (client, readfds);/* scoatem si din multime */
    pthread_mutex_unlock(&connectMutex);
    close(client);
  }
}

char* myLogin(const char* str) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  strcpy(response, "Inside login!\n");

  pthread_mutex_lock(&loggedMutex);
  isUserLogged[client] = 0;
  pthread_mutex_unlock(&loggedMutex);

  return response;
}

char* logout(const char* str) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  strcpy(response, "Inside logout!\n");

  return response;
}

char* recommend(const char* str) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  strcpy(response, "Inside recommend!\n");

  return response;
}

char* download(const char* str)  {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  strcpy(response, "Inside download!\n");

  return response;
}

char* myRegister(const char* str)  {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  strcpy(response, "Inside register!\n");

  return response;
}

char* exit(const char* str) {
  char* response = new char[LGMAX];
  bzero(response, LGMAX);

  strcpy(response, "Server exited!\n");

  return response;
}

void extract_command(const char* request, char* command)  {
  int i;
  for (i = 0; request[i] != ' ' && request[i] != 0; i++)
    command[i] = request[i];
  command[i] = '\0';
}