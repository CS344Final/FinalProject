#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/types.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define NAME_SIZE 255 /*Includes room for null */
#define DIR_SIZE 9000

struct menu{
  unsigned char line1[30];
  unsigned char line2[30];
  unsigned char line3[30];
} men;

void DieWithError(char *errorMessage);  /* Error handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);

unsigned int sendMenuAndWaitForResponse(int);

void getDirectoryName(int, char *);
void directoryListing(int, char *);
void getFileName(int, char *);
void sendFile(int, char *);
void sendFileSize(int, char *);
void awaitConfirmation(int);

//this function is done
void HandleTCPClient(int clntSocket)
{
    unsigned int response = 0;
    char directoryName[NAME_SIZE]; //max length 20
    int number = 0;
    unsigned char errorMsg[] = "Invalid Choice";
    unsigned char bye[] = "Bye!";
    unsigned char fileName[NAME_SIZE];  

    response = sendMenuAndWaitForResponse(clntSocket);      //send + receive input
    while(response != 3)
    {
        switch(response)
        {
            case 1: printf("Client selected 1.\n");
                    getDirectoryName(clntSocket,directoryName);     //send + receive input
                    directoryListing(clntSocket, directoryName);    //send + receive confirm
                    break;
            case 2: printf("Client selected 2.\n");
                    getFileName(clntSocket,fileName);               //send + receive input
                    sendFileSize(clntSocket, fileName);             //send + receive confirm
                    sendFile(clntSocket,fileName);                  //send + receive confirm
                    break;
            default: printf("Client selected junk.\n"); put(clntSocket, errorMsg, sizeof(errorMsg)); break;
        }
        response = sendMenuAndWaitForResponse(clntSocket);
    }

    put(clntSocket, bye, sizeof(bye));

    if(close(clntSocket) < 0)
        DieWithError("close() failed");     /* Close client socket */
    printf("Connection with client %d closed.\n", clntSocket);
}

unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    memset(&mainMenu, 0, sizeof(struct menu));   /* Zero out structure */
    strcpy(mainMenu.line1,"1) Get directory listing\n");
    strcpy(mainMenu.line2, "2) Select file\n");
    strcpy(mainMenu.line3, "3) Quit\n");
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu));
    get(clntSocket, &response, sizeof(unsigned int));
    return ntohl(response);
}

//gets dirname from user and stores it
void getDirectoryName(int sock,char *dirname){
    unsigned char msg[255];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter the name of the directory to list:\n");
    put(sock,msg,sizeof(msg));
    memset(dirname, 0, NAME_SIZE);
    get(sock,dirname, NAME_SIZE);
    printf("Client entered %s\n",dirname);
}
 
void directoryListing(int sock, char *dirname) {
    printf("Attempting to list directory\n");
    struct stat mystat, *sp = &mystat;
    int r;
    char path[1024], cwd[1024];
    unsigned char msg[DIR_SIZE];
    strcpy(msg,"\0");

    DIR *dp;
    struct dirent *dirp;
    dp = opendir(dirname);
    char dnamecpy[1024]; 
    while ((dirp = readdir(dp)) != NULL){
        sprintf(msg+strlen(msg),"%s", dirp->d_name);
        strcpy(dnamecpy, dirname);
        strcat(dnamecpy,"/");
        strcat(dnamecpy, dirp->d_name);
        if (dirp->d_type == DT_DIR) {
            sprintf(msg+strlen(msg),"*");
        }
        sprintf(msg+strlen(msg),"\n");
    }
    closedir(dp);

    sprintf(msg+strlen(msg), "\0");
    printf("sending contents to client\n");
    put(sock,msg, DIR_SIZE);
    awaitConfirmation(sock);
}

void getFileName(int sock, char *filename) {
    unsigned char msg[255];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter the name of the file to download:\n");
    put(sock,msg,sizeof(msg));
    memset(filename, 0, NAME_SIZE);
    get(sock,filename, NAME_SIZE);
    printf("Client entered %s\n",filename);
}

void sendFileSize(int sock, char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        DieWithError("stat() failed");
    }
    long fileSize = st.st_size;
    char fileSizeStr[32];
    sprintf(fileSizeStr, "%ld", fileSize);
    put(sock, fileSizeStr, sizeof(unsigned int));
    awaitConfirmation(sock);
}

void sendFile(int sock, char *filenameOut) {
    printf("Attempting to send file %s\n", filenameOut);
    FILE *file = fopen(filenameOut, "rb");
    if(file == NULL)    
        DieWithError("fopen() failed");
    unsigned char buffer[RCVBUFSIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, RCVBUFSIZE, file)) > 0) {
        printf("%d bytes read", bytesRead);
        put(sock, buffer, bytesRead);
    }   
    fclose(file);
    printf("\nFile %s sent\nAwaiting confirmation from client\n", filenameOut);
    awaitConfirmation(sock);
}

void awaitConfirmation(int sock) {
    unsigned char msg[NAME_SIZE];
    memset(msg, 0, NAME_SIZE);
    get(sock, msg, NAME_SIZE);
    printf("%s\n",msg);
}