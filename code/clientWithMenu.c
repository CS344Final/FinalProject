#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define RCVBUFSIZE 100 /* Size of receive buffer */
#define NAME_SIZE 255  /*Includes room for null */
#define DIR_SIZE 9000

struct menu {
    unsigned char line1[30];
    unsigned char line2[30];
    unsigned char line3[30];
};

void DieWithError(char *errorMessage); /* Error handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
void talkToServer(int);
unsigned int displayMenuAndSendSelection(int);
void receiveDirList(int sock);
void sendDirName(int, char *);
void sendFilename(int, char *);
void receiveFile(int, char *, unsigned int *);
void sendConfirmation(int, char *);
void getFileSize(int, unsigned int *);


int main(int argc, char *argv[]) {
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char *echoString;                /* String to send to echo server */
    unsigned int echoStringLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv()
                                       and total bytes read */
    int answer;    
    /* Test for correct number of arguments */
    if ((argc < 2) || (argc > 3)) {
        fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n",
                argv[0]);
        exit(1);
    }

    servIP = argv[1]; /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        echoServPort = atoi(argv[2]); /* Use given port, if any */
    else
        echoServPort = 7; /* 7 is the well-known port for the echo service */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);      /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    talkToServer(sock);

    close(sock);
    exit(0);
}

void talkToServer(int sock) {
    unsigned int selection = 0;
    unsigned char bye[5];
    unsigned char fileName[NAME_SIZE];
    unsigned char dirName[NAME_SIZE];
    unsigned int *fileSize;
    *fileSize = 0;

    while (1)
    {
        selection = displayMenuAndSendSelection(sock);      //receive + send input
        printf("Client selected: %d\n", selection);
        switch (selection) {
        case 1:
            sendDirName(sock, dirName);                     //receive + send input
            receiveDirList(sock);                           //receive + send confirm
            break;
        case 2:
            sendFilename(sock, fileName);                   //receive + send input
            getFileSize(sock, fileSize);                    //receive + send confirm
            receiveFile(sock, fileName, fileSize);          //receive + send confirm
            break;
        }
        if (selection == 3)
            break;
    }
    selection = htonl(selection);
    put(sock, &selection, sizeof(unsigned int));
    get(sock, bye, 5);
    printf("%s\n", bye);
}

unsigned int displayMenuAndSendSelection(int sock) {
    struct menu menuBuffer; /* Buffer for echo string */
    unsigned int response = 0;
    unsigned int output;
    char junk;

    printf("Inside client display menu\n");
    get(sock, &menuBuffer, sizeof(struct menu)); // in this case server is also sending null
    printf("%s\n", menuBuffer.line1);
    printf("%s\n", menuBuffer.line2);
    printf("%s\n", menuBuffer.line3);
    scanf("%d", &response);
    getc(stdin);
    output = htonl(response);
    put(sock, &output, sizeof(unsigned int));
    return response;
}

void sendDirName(int sock, char *dirname) {
    unsigned char msg[255];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(dirname, 0, NAME_SIZE);
    fgets(dirname, NAME_SIZE, stdin);
    dirname[strcspn(dirname, "\n")] = 0;
    put(sock, dirname, NAME_SIZE);
}

void receiveDirList(int sock) {
    printf("receiveDirList called\n");
    unsigned char dirList[DIR_SIZE];
    memset(dirList, 0, sizeof(dirList));
    get(sock,dirList, DIR_SIZE);
    printf("%s\n",dirList);
    sendConfirmation(sock, "Directory list");
}

void sendFilename(int sock, char *fileNameIn) {
    unsigned char msg[255];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(fileNameIn, 0, NAME_SIZE);
    fgets(fileNameIn, NAME_SIZE, stdin);
    fileNameIn[strcspn(fileNameIn, "\n")] = 0;
    put(sock, fileNameIn, NAME_SIZE);
}

void getFileSize(int sock, unsigned int *size){
    unsigned char fileSize[sizeof(unsigned int)];
    memset(fileSize, 0, sizeof(unsigned int));
    get(sock, fileSize, sizeof(fileSize));
    *size = atoi(fileSize);
    printf("File of size %u bytes ready to download\n",*size);
    sendConfirmation(sock, "File size");
}

//sends a confirm message to server
void sendConfirmation(int sock, char *dataName){
    unsigned char msg[NAME_SIZE];
    sprintf(msg, "%s was received by the client\n",dataName);
    put(sock, msg, NAME_SIZE);
    printf("Sent confirmation to the server.\n");
}

void receiveFile(int sock, char *filenameIn, unsigned int *fileSize) {
    unsigned char fileData[RCVBUFSIZE];
    printf("Attempting to receive file %s\n", filenameIn);
    FILE *file = fopen(filenameIn, "wb");
    if (file == NULL) {
        DieWithError("fopen() failed");
    }
    printf("file %s opened\n",filenameIn);

    int totalBytesReceived = 0;
    int bytesReceived = 0;
    while (totalBytesReceived < *fileSize) {
        bytesReceived = recv(sock, fileData, RCVBUFSIZE, 0);
        if (bytesReceived <= 0) {
            break;
        }
        if (fwrite(fileData, 1, bytesReceived, file) != bytesReceived) {
            DieWithError("fwrite() failed");
        }
        totalBytesReceived += bytesReceived;
        printf("written: %s\n",fileData);
    }
    if (totalBytesReceived != *fileSize) {
        DieWithError("File transfer incomplete");
    }

    fclose(file);
    printf("File %s received\n", filenameIn);
    sendConfirmation(sock, "File");
}
