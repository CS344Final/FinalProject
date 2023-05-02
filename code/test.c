#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 256

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void get(int sock, void *buffer, unsigned int bufferSize)
{
    int totalBytesReceived = 0;
    int bytesReceived = 0;

    while (totalBytesReceived < bufferSize)
    {
        bytesReceived = recv(sock, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
        if (bytesReceived < 0)
            DieWithError("recv() failed");
        else if (bytesReceived == 0)
            DieWithError("Connection closed prematurely");
        totalBytesReceived += bytesReceived;
    }
}

void put(int sock, void *buffer, unsigned int bufferSize)
{
    int totalBytesSent = 0;
    int bytesSent = 0;

    while (totalBytesSent < bufferSize)
    {
        bytesSent = send(sock, buffer + totalBytesSent, bufferSize - totalBytesSent, 0);
        if (bytesSent < 0)
            DieWithError("send() failed");
        totalBytesSent += bytesSent;
    }
}

void handleClient(int clientSock)
{
    char buffer[BUFFER_SIZE];
    int choice;
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        put(clientSock, "Menu:\n1. Get directory listing\n2. Select file\n3. Quit\nEnter choice: ", strlen("Menu:\n1. Get directory listing\n2. Select file\n3. Quit\nEnter choice: "));
        get(clientSock, &choice, sizeof(int));

        switch (choice)
        {
        case 1:
            // Get directory listing
            // ...
            put(clientSock, "Directory listing:\n", strlen("Directory listing:\n"));
            // ...
            break;

        case 2:
            // Select file
            // (not finished)
            // put(clientSock, "What file would you like to reach?\n", strlen("What file would you like to reach?"\n))
            // get(clientSock, )

            // Ask for directory
            // ?!!??!?!!?!


            // Change to the desired directory
            sprintf(buffer, "CWD %s\r\n", directory);
            write(sockfd, buffer, strlen(buffer));
            bzero(buffer, sizeof(buffer));
            read(sockfd, buffer, sizeof(buffer));
            printf("%s", buffer);

            // Get a list of files in the directory
            sprintf(buffer, "NLST\r\n");
            write(sockfd, buffer, strlen(buffer));
            bzero(buffer, sizeof(buffer));



            // ...
            if (fileExists)
            {
                put(clientSock, "File contents:\n", strlen("File contents:\n"));
                // Send file contents
            }
            else
            {
                put(clientSock, "File not found\n", strlen("File not found\n"));
            }
            break;

        case 3:
            // Quit
            close(clientSock);
            exit(0);

        default:
            put(clientSock, "Invalid choice\n", strlen("Invalid choice\n"));
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    int serverSock, clientSock, portNum;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    portNum = atoi(argv[1]);

    // Create socket
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
    {
        DieWithError("Could not create socket");
    }

    // Set up server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);

    // Bind socket to address
    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        DieWithError("bind() failed");
    }

    if (listen(serverSock, MAX_PENDING) < 0)
    {
        DieWithError("listen() failed");
    }

    while (1)
    {
        clientLen = sizeof(clientAddr);
        if ((clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen)) < 0)
        {
            DieWithError("accept() failed");
        }

        pid_t pid = fork(); // Spawn a child process to handle the client
        if (pid < 0)
        {
            DieWithError("fork() failed");
        }
        else if (pid == 0)
        {                      // Child process
            close(serverSock); // Close server socket in child process

            char menu[1024] = "1. Get directory listing\n2. Select file\n3. Quit\n";
            int choice;

            do
            {
                put(clientSock, menu, strlen(menu)); // Send menu to client
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                get(clientSock, buffer, sizeof(buffer)); // Receive choice from client
                choice = atoi(buffer);

                switch (choice)
                {
                case 1:
                { // Get directory listing
                    DIR *dir;
                    struct dirent *entry;
                    char listing[1024] = "";
                    if ((dir = opendir(".")) != NULL)
                    {
                        while ((entry = readdir(dir)) != NULL)
                        {
                            strcat(listing, entry->d_name);
                            strcat(listing, "\n");
                        }
                        closedir(dir);
                        put(clientSock, listing, strlen(listing)); // Send directory listing to client
                    }
                    else
                    {
                        put(clientSock, "Error reading directory\n", strlen("Error reading directory\n")); // Send error message to client
                    }
                    break;
                }
                case 2:
                { // Select file
                    char file[1024] = "";
                    put(clientSock, "Enter file name: ", strlen("Enter file name: "));
                    get(clientSock, file, sizeof(file)); // Receive file name from client

                    FILE *fp;
                    char buffer[1024];
                    size_t nread;

                    if ((fp = fopen(file, "rb")) != NULL)
                    {
                        while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0)
                        {
                            put(clientSock, buffer, nread); // Send file to client
                        }
                        fclose(fp);
                    }
                    else
                    {
                        put(clientSock, "File not found\n", strlen("File not found\n")); // Send error message to client
                    }
                    break;
                }
                case 3: // Quit
                    break;
                default:                                                             // Invalid choice
                    put(clientSock, "Invalid choice\n", strlen("Invalid choice\n")); // Send error message to client
                    break;
                }
            } while (choice != 3);

            close(clientSock); // Close client socket in child process
            exit(0);           // Terminate child process
        }
        else
        {                      // Parent process
            close(clientSock); // Close client socket in parent process
        }
    }

    return 0;
}
