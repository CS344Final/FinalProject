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

struct menu{
  unsigned char line1[30];
  unsigned char line2[30];
  unsigned char line3[30];
} men;

void DieWithError(char *errorMessage);  /* Error handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);

unsigned int sendMenuAndWaitForResponse(int);

//these functions should be changed for our purposes
void getDirectoryName(int, char *);
void directoryListing(int, char *);
void getFileName(int, char *);
void sendFile(int, char *); //this method is extended below

void HandleTCPClient(int clntSocket)
{
    int recvMsgSize;                    /* Size of received message */
    unsigned int response = 0;
    char directoryName[NAME_SIZE]; //max length 20
    int number = 0;
    unsigned char errorMsg[] = "Invalid Choice";
    unsigned char bye[] = "Bye!";
    unsigned char fileName[NAME_SIZE];  

    response = sendMenuAndWaitForResponse(clntSocket);
    while(response != 3)
    {
        switch(response)
        {       // here too we need functions for directory listing and selecting a file
            case 1: printf("Client selected 1.\n");
                    getDirectoryName(clntSocket,directoryName);
                    directoryListing(clntSocket, directoryName); 
                    break;
            case 2: printf("Client selected 2.\n");
                    getFileName(clntSocket,fileName);
                    sendFile(clntSocket,fileName);
                    break;
            default: printf("Client selected junk.\n"); put(clntSocket, errorMsg, sizeof(errorMsg)); break;
        }
        response = sendMenuAndWaitForResponse(clntSocket);
    }//end while

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
    char path[1024], cwd[1024], msg[4096];

    if ((r = lstat(dirname, sp)) < 0)
	{
        printf(msg, "No such file: %s\n",dirname);
        DieWithError(msg);
	}
	strcpy(path, dirname);
	if (path[0] != '/') // filename is relative
    {
		getcwd(cwd, 1024);
		strcpy(path, cwd);
		strcat(path, "/");
		strcat(path, dirname);
	}
    
    DIR *dir = fdopendir(path);
    
    if (dir == NULL) {
        perror("fdopendir");
        closedir(dir);
    }

    // Loop through the directory entries
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR)
            strcat(msg,"/");
        strcat(msg, entry->d_name);
    }

    // Close the directory
    closedir(dir);
    put(sock,msg, sizeof(msg));
}

void getFileName(int sock, char *filename){
    unsigned char msg[255];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter the name of the file to download:\n");
    put(sock,msg,sizeof(msg));
    memset(filename, 0, NAME_SIZE);
    get(sock,filename, NAME_SIZE);
    printf("Client entered %s\n",filename);
}

void sendFile(int sock, char *filenameOut){
    printf("Attempting to send file %s\n", filenameOut);
    FILE *file = fopen(filenameOut, "rb");
    if(file == NULL)    
        DieWithError("fopen() failed");
    unsigned char buffer[RCVBUFSIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        put(sock, buffer, bytesRead);
    }
    fclose(file);
    printf("File %s sent\n", filenameOut);
}
/*
this project isn't getting finished. 
I changed the menu and tried to add file transfer, 
but it just makes a new file of the same name
and doesn't transfer the contents. -jack
*/