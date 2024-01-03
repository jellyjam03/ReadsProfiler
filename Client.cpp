#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#define LGMAX 100

using namespace std;

int main(int argc, char* argv[]) {

    int sd, port; //socket descriptor si port de conectare
    struct sockaddr_in server;
    char command[LGMAX], response[LGMAX];

    if (argc != 3)  {
      printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

    port = atoi(argv[2]);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons (port);

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)  {
            perror ("[client] Eroare la socket().\n");
            return errno;
        }
    
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)   {
            perror ("[client]Eroare la connect().\n");
            return errno;
        }

    while (true) {

        cin >> command;

        if (write (sd, command, LGMAX) <= 0)  {
            perror ("[client]Eroare la write() spre server.\n");
            return errno;
        }

        if (read (sd, response, LGMAX) < 0)  {
            perror ("[client]Eroare la read() de la server.\n");
            return errno;
        }

        printf ("[client]Mesajul primit este: %s\n", response);
        
        if (strcmp(response, "Server exited!\n") == 0)  {
            break;
        }
    }
    
    close(sd);

    return 0;
}