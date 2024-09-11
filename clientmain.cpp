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


// Included to get the support library
#include "calcLib.h"

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

int main(int argc, char *argv[]){

  if (argc != 2) {
    printf("Usage: %s <DNS|IPv4|IPv6>:<PORT>\n", argv[0]);
    return -1;
  }
  // Find the last colon in the input to split address and port
  char *input = argv[1];
  char *last_colon = strrchr(input, ':');

  if (last_colon == NULL) {
      printf("Invalid input. Use <DNS|IPv4|IPv6>:<PORT>\n");
      return -1;
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
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      return -1;
  }

  // Create the socket
  int s;
  s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (s < 0) {
      perror("failed to create socket");
      freeaddrinfo(res);
      return -1;
  }

  // Connect to the server
  if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
    perror("failed to connect");
    close(s);
    freeaddrinfo(res);
    return -1;
  }


  // create a buffer
  char buf[1024] = {0};
  // read the available version
  read(s, buf, sizeof(buf));
  printf("%s", buf);

  // use send or write
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
    printf("result: %8.8g\n", result);
    char result_str[1024];
    sprintf(result_str, "%f8.8g\n", result);
    write(s, result_str, strlen(result_str));
  } else {
    int result = get_int_result(buf);
    printf("result: %d\n", result);
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
