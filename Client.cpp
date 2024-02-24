#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <csignal>
#define LGMAX 1000

using namespace std;

int sd, port; //socket descriptor si port de conectare
struct sockaddr_in server;

const char sentinel[] = "!SENTINEL!";
char command[LGMAX], response[LGMAX];

void signalHandler(int signum);
size_t myRead(int sd, char* buff, size_t buffSize);
size_t myWrite(int sd, char* buff, size_t buffSize);
void handleDownload();
bool isEndOfFile(char* data, size_t &dataSize);

int main(int argc, char* argv[]) {

    if (argc != 3)  {
      printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

    signal(SIGINT, signalHandler);

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

    printf("Welcome to ReadsProfiler!\nYou can type help for a list of the available commands.\n");

    while (true) {

        printf("\n[client] ");
        cin.getline(command, LGMAX);

        if (myWrite (sd, command, LGMAX) <= 0)  {
            perror ("[client]Eroare la write() spre server.\n");
            return errno;
        }

        if (myRead (sd, response, LGMAX) < 0)  {
            perror ("[client]Eroare la read() de la server.\n");
            return errno;
        }

        printf ("%s", response);
        
        if (strstr(response, "Server exited!\n") != NULL)  {
            break;
        }

        if (strcmp(response, "downloading...\n") == 0) {
            handleDownload();
            cout<<"finished downloading!\n";
        }
    }
    
    close(sd);

    return 0;
}

size_t myRead(int sd, char* buff, size_t buffSize) {
    ssize_t total = 0;
    ssize_t bytes_read;

    bzero(buff, buffSize);

    while (total < buffSize) {
        bytes_read = read(sd, buff + total, buffSize - total);

        if (bytes_read > 0) {
            total += bytes_read;
        } else if (bytes_read == 0) {
            break;  // Connection closed by the other end
        } else {
            // Handle read error
            perror("Error reading from socket");
            return total;  // or return bytes_read directly
        }
    }

    // Debug output (byte by byte)
    // cout << "Read data: ";
    // for (size_t i = 0; i < total; i++) {
    //     cout << hex << static_cast<int>(static_cast<unsigned char>(buff[i])) << " ";
    // }
    // cout << '\n';

    return total;
}

size_t myWrite(int sd, char* buff, size_t buffSize) {
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

void signalHandler(int signum) {
    strcpy(command, "exit");

    printf("\n[client] ");

    if (write (sd, command, LGMAX) <= 0)  {
        perror ("[client]Eroare la write() spre server.\n");
        exit(errno);
    }

    if (myRead (sd, response, LGMAX) < 0)  {
        perror ("[client]Eroare la read() de la server.\n");
        exit(errno);
    }

    strcpy(response, "");

    close(sd);
    exit(0);
}

void handleDownload() {
    size_t datasize = 0;
    char aux[LGMAX];
    FILE* fd = fopen("./download.txt", "wb");

    bzero(aux, LGMAX);

    while (true)
    {
        datasize = myRead(sd, aux, LGMAX);
        if (!isEndOfFile(aux, datasize))
            fwrite(aux, 1, datasize, fd);
        else {
            fwrite(aux, 1, datasize - sizeof(sentinel), fd);
            break;
        }
    }

    cout<<aux<<'\n';

    fclose(fd);
}

bool isEndOfFile(char* data, size_t &dataSize) {
    int i;
    for (i = 0; data[i]; i++);
    dataSize = i;

    if (dataSize < sizeof(sentinel) -1) return 0;

    return (memcmp(data + dataSize - sizeof(sentinel) + 1, sentinel, sizeof(sentinel) - 1) == 0);
}