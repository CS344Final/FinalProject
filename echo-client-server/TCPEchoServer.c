#include <stdio.h>
#include "Practical.h"
#include <unistd.h>

int main(int argc, char *argv[]) {

  if (argc != 2) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)", "<Server Port/Service>");

  char *service = argv[1]; // First arg:  local port

  // Create socket for incoming connections
  int servSock = SetupTCPServerSocket(service);
  if (servSock < 0)
    DieWithUserMessage("SetupTCPServerSocket() failed", service);

  //buffer 
  char buff[MAX];
  int n;

  for (;;) { // Run forever
    // New connection creates a connected client socket
    int clntSock = AcceptTCPConnection(servSock);

    HandleTCPClient(clntSock); // Process client
    
    //erases memory from previous commands
    //buffer 
    bzero(buff, MAX);
   
    // read the message from client and copy it in buffer
    read(connfd, buff, sizeof(buff));

    // print buffer which contains the client contents
    printf("From client: %s\t To client : ", buff);
    bzero(buff, MAX);
    n = 0;
    
    // copy server message in the buffer
    while ((buff[n++] = getchar()) != '\n')
      ;
   
    // and send that buffer to client
    write(connfd, buff, sizeof(buff));
   
    // if msg contains "Exit" then server exit and chat ended.
    if (strncmp("exit", buff, 4) == 0) {
      printf("Exiting Server...\n");
      close(clntSock);
      break;   
    // below line closes connection with server
    
  }
  // NOT REACHED
  close(servSock);
}
