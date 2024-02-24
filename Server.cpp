#include <iostream>
#include <arpa/inet.h>
#include <queue>
#include <csignal>
#include "dataBase.h"
#include "Commands.h"

#define PORT 2728
#define SERV_BACKLOG 1000  //cati clienti pot astepta la rand la port in listen
#define NR_THREADS 4

using namespace std;

map <int, int> isUserLogged;
set <int> loggedUserIds;


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
void signalHandler(int signum);

/* programul */
int main ()
{
  struct sockaddr_in server;	/* structurile pentru server si clienti */
  struct sockaddr_in from;
  struct timeval tv;		/* structura de timp pentru select() */
  fd_set readfds;		/* multimea descriptorilor de citire */
  fd_set actfds;		/* multimea descriptorilor activi */
  int sd, client;		/* descriptori de socket */
  int optval=1; 			/* optiune folosita pentru setsockopt()*/ 
  int fd;			/* descriptor folosit pentru 
				   parcurgerea listelor de descriptori */
  int nfds;			/* numarul maxim de descriptori */
  int len;			/* lungimea structurii sockaddr_in */

  signal(SIGINT, signalHandler);

  if (sqlite3_open("dataBase.db", &myDataBase) != SQLITE_OK) {
    cout<<"Can't open database.\n";
    return 0;
  }

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
      isUserLogged[client] = -1;
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
          if (myRead(fd, pInfo->request, LGMAX) < 0) {
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

  extractToken(request, command, 1);
  if (M.find(command) == M.end()) {
    strcpy(response,"Erorr. Invalid command.\n");
  }
  else {
    char* presponse = M[command](request, client);
    strcpy(response, presponse);
    delete[] presponse;
  }

  if (myWrite(client, response, sizeof(response)) < 0)  {
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

void signalHandler(int signum) {
  char response[LGMAX] = "";
  strcpy(response, "Server exited!\n");

  pthread_mutex_lock(&loggedMutex);
  for (auto user = isUserLogged.begin(); user != isUserLogged.end(); user++) {
    myWrite(user->first, response, LGMAX);
    close(user->first);
  }
  pthread_mutex_unlock(&loggedMutex);

  exit(0);
}