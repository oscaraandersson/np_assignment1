#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG
#define BUFFER_SIZE 1024


// Included to get the support library
#include "calcLib.h"

int check_supported_version(const char *buffer, const char *version) {
  // Use strstr to search for the version string within the buffer
  if (strstr(buffer, version) != NULL) {
    // Found the supported version in the buffer
    return 1;
  } else {
    // Version not found
    return 0;
  }
}

// Set a timeout for the socket (e.g., 5 seconds)
void set_socket_timeout(int socket_fd) {
  struct timeval timeout;
  timeout.tv_sec = 5;  // 5 seconds
  timeout.tv_usec = 0;

  // Set the timeout for both send and receive operations
  if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("Failed to set socket receive timeout");
  }
  if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("Failed to set socket send timeout");
  }
}



double get_float_result(char* buffer) {
  double f1, f2, fresult;
  char command[10];
  sscanf(buffer, "%s" "%lg %lg", command, &f1, &f2);
  if(strcmp(command,"fadd")==0){
    fresult=f1+f2;
  } else if (strcmp(command, "fsub")==0){
    fresult=f1-f2;
  } else if (strcmp(command, "fmul")==0){
    fresult=f1*f2;
  } else if (strcmp(command, "fdiv")==0){
    fresult=f1/f2;
  }
  return fresult;
}

int get_int_result(char* buffer) {
  int i1, i2, iresult;
  char command[10];
  sscanf(buffer, "%s" "%d %d", command, &i1, &i2);
  if(strcmp(command,"add")==0){
    iresult=i1+i2;
  } else if (strcmp(command, "sub")==0){
    iresult=i1-i2;
  } else if (strcmp(command, "mul")==0){
    iresult=i1*i2;
  } else if (strcmp(command, "div")==0){
    iresult=i1/i2;
  }
  return iresult;
}

// test cases
// ./client ec2-13-53-76-30.eu-north-1.compute.amazonaws.com:5000
// ./client ::ffff:d35:4c1e:5000
// ./client 0000:0000:0000:0000:0000:ffff:0d35:4c1e:5000
// ./client 13.53.76.30:5000

int main(int argc, char *argv[]){

  if (argc != 2) {
    printf("Usage: %s <DNS|IPv4|IPv6>:<PORT>\n", argv[0]);
    return 0;
  }
  // Find the last colon in the input to split address and port
  char *input = argv[1];
  char *last_colon = strrchr(input, ':');

  if (last_colon == NULL) {
    printf("Invalid input. Use <DNS|IPv4|IPv6>:<PORT>\n");
    return 0;
  }

  // Split the string into address and port
  *last_colon = '\0';
  char *dest_host = input;
  char *dest_port = last_colon + 1;

  #ifdef DEBUG
  printf("Host %s, and port %d.\n",dest_host, atoi(dest_port));
  #endif

  // Set up for getaddrinfo
  // https://stackoverflow.com/questions/755308/whats-the-hints-mean-for-the-addrinfo-name-in-socket-programming
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;        // Allow for both IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

  // The getaddrinfo can automatically resolve the format of the address and do DNS lookup
  int status;
  if ((status = getaddrinfo(dest_host, dest_port, &hints, &res)) != 0) {
    printf("ERROR\n");
    fprintf(stderr, "RESOLVE ISSUE\n");
    return 0;
  }

  // Create the socket
  int s;
  s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (s < 0) {
    perror("failed to create socket");
    freeaddrinfo(res);
    return 0;
  }

  set_socket_timeout(s);

  // Connect to the server
  if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
    printf("ERROR: CANT CONNECT TO %s", dest_host);
    close(s);
    freeaddrinfo(res);
    return 0;
  }


  // create a buffer
  char buf[1024] = {0};
  // read the available version
  // Read data from server (with limited buffer size)
  read(s, buf, sizeof(buf));
  // printf("%s", buf);

  // Read the supported versions and check if we have support
  // otherwise print ERROR missmatch protocol
  // Each supported version is on a new line

  // go through the buffer and check if we have support
  const char *supported_versions[] = {
    "TEXT TCP 1.0",
    "TEXT TCP 1.1"
  };
  int num_versions = sizeof(supported_versions) / sizeof(supported_versions[0]);

  // Loop through all your supported versions
  int found = 0;
  for (int i = 0; i < num_versions; i++) {
    if (check_supported_version(buf, supported_versions[i])) {
      found = 1;
      break;  // Stop if we find a supported version
    }
  }

  if (!found) {
    printf("ERROR\n");
    fprintf(stderr, "No supported version found\n");
    return 0;
  }


  // use send or write?
  // - write is simpler but if you want flags use send.
  char ack[] = "OK\n";
  write(s, ack, strlen(ack));

  // clear the buffer
  memset(buf, 0, sizeof(buf));

  read(s, buf, sizeof(buf));
  printf("%s", buf);

  // parse the math problem in the buffer
  if (buf[0] == 'f') {
    double result = get_float_result(buf);
    #ifdef DEBUG
    printf("Calculated the result to: %8.8g\n", result);
    #endif
    char result_str[1024];
    sprintf(result_str, "%f8.8g\n", result);
    write(s, result_str, strlen(result_str));
  } else {
    int result = get_int_result(buf);
    #ifdef DEBUG
    printf("Calculated the result to: %d\n", result);
    #endif
    char result_str[1024];
    sprintf(result_str, "%d\n", result);
    write(s, result_str, strlen(result_str));
  }

  // read the message
  memset(buf, 0, sizeof(buf));
  read(s, buf, sizeof(buf));
  printf("%s", buf);

  // close the socket
  close(s);
  return 0;
}
