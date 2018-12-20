#include <set>
#include <queue>
#include <vector>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <iostream>
#include <assert.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "json.hpp"

#define MAX_FD 1024
#define MAX_DATALEN 8192 
#define MAX_PARALLEL 4
#define MAX_NAME_LEN 1024
using json = nlohmann::json;

struct user_info{
  int user_id;
  std::string user_name;
  std::string user_password;
  std::set<std::string> friend_list;
  std::set<std::string> black_list;
  user_info(int _id,std::string _user,std::string _user_password){
    user_id = _id;
    user_name = _user;
    user_password = _user_password;
    friend_list.clear();
    black_list.clear();
  }
};

void htoip(char *hostname) 
{   
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
 
    if ((rv = getaddrinfo(hostname,"http", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }
 
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        h = (struct sockaddr_in *) p->ai_addr;
        strcpy(hostname, inet_ntoa(h->sin_addr));
    }
     
    freeaddrinfo(servinfo);
}

int sockfd,retval;
char const *split = ":";
fd_set readset , retset;

int main(int argc, char const *argv[])
{
  if(argc != 2){
    return -1 , fprintf(stderr, "usage : piepie-client [port]");
  }
  fprintf(stderr, "Pie pie is so pie.");
  // init socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // assert(socket );
  char *hostname = (char*)malloc(sizeof(char) * 200);
  char *port = (char*)malloc(sizeof(char) * 10);
  int len = strlen(argv[2]);
  char *start = (char *)malloc(sizeof(char) * (len+1));
  strcpy(start, argv[2]);
  start = strtok(start, split);
  strcpy(hostname, start);
  start = strtok(NULL, split);
  strcpy(port, start);
  htoip(hostname);
  printf("%s:%s\n", hostname, port);

  struct sockaddr_in serverInfo;
  bzero(&serverInfo, sizeof(serverInfo));
  serverInfo.sin_family = PF_INET;
  serverInfo.sin_addr.s_addr = inet_addr(hostname);
  serverInfo.sin_port = htons(atoi(port));
  retval = connect(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));

  return 0;
}
