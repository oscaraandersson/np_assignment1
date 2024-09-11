#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/socket.h>
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

  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port).
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'.
  */
  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter.

  /* Do magic */
  int port=atoi(Destport);
#ifdef DEBUG
printf("Host %s, and port %d.\n",Desthost,port);
#endif

  int s;
  struct sockaddr_in channel;

  // Create the socket (file descriptor to endpoint)
  // domain: AF_INET IpV4 internet protocols
  //  - what is the difference between AF_INET and PF_INET
  // type: communication semantics SOCK_STREAM that provides a two way communication
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s<0) {
    printf("failed to create socket\n");
  }

  // sockaddr_in doc
  // https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
  memset(&channel, 0, sizeof(channel));
  channel.sin_family = AF_INET;
  channel.sin_port = htons(port);
  // convert ipv4 numbers and does to byteformat in correct byteorder
  //
  // What is the difference between inet_pton and inet_aton. check man
  // - inet_pton can convert both but you have to specify the version
  if (inet_aton(Desthost, &channel.sin_addr) <= 0) {
      printf("failed to convert id address to bytes\n");
  }

  if (connect(s, (struct sockaddr*)&channel, sizeof(channel)) < 0) {
    printf("failed to connect\n");
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
