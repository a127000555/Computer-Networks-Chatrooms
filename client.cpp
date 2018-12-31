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
#include <netdb.h>

#include "json.hpp"

#define MAX_FD 1024
#define MAX_DATALEN 8192 
#define MAX_PARALLEL 4
#define MAX_NAME_LEN 1024
#define USERNAME_LEN 100
#define PASSWORD_LEN 100
#define MSG_LEN 1000
#define CMD_LEN 10
#define ID_LEN 9
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
    return -1 , fprintf(stderr, "usage : piepie-client [host]:[port]");
  }
  fprintf(stderr, "Pie pie is so pie.\n");

  // init socket
  // -----input:argv[1] -----
  // -----output:sockfd -----
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  char *hostname = (char*)malloc(sizeof(char) * 200);
  char *port = (char*)malloc(sizeof(char) * 10);
  int len = strlen(argv[1]);
  char *start = (char *)malloc(sizeof(char) * (len+1));
  strcpy(start, argv[1]);
  start = strtok(start, split);
  strcpy(hostname, start);
  start = strtok(NULL, split);
  strcpy(port, start);
  htoip(hostname);
  printf("Server is on %s:%s\n", hostname, port);

  struct sockaddr_in serverInfo;
  bzero(&serverInfo, sizeof(serverInfo));
  serverInfo.sin_family = PF_INET;
  serverInfo.sin_addr.s_addr = inet_addr(hostname);
  serverInfo.sin_port = htons(atoi(port));
  retval = connect(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
  assert(retval >= 0);
  fprintf(stderr, "Connection Success!\n");

  // Some variable for chat room command
  size_t cmd_len = CMD_LEN;
  size_t target_len = ID_LEN;
  size_t username_len = USERNAME_LEN;
  size_t password_len = PASSWORD_LEN;
  size_t msg_len = MSG_LEN; 
  size_t vallen;
  char* command = (char*)malloc(sizeof(char) * CMD_LEN);
  char* username = (char*)malloc(sizeof(char) * USERNAME_LEN);
  char* password = (char*)malloc(sizeof(char) * PASSWORD_LEN);
  char* target = (char*)malloc(sizeof(char) * ID_LEN);
  char* msg = (char*)malloc(sizeof(char) * MSG_LEN);
  
  // Chat room while
  while (true) {
    printf("[?] s - sign up\n");
    printf("[?] l - login\n");
    printf("[?] a - show users\n");
    printf("[?] m - messaging\n");
    printf("[?] r - refresh\n");
    printf("command?\n[>]: ");

    vallen = getline(&command, &cmd_len, stdin) - 1;
    command[vallen] = '\0';
    // printf("%d\n", vallen);
    // fprintf(stderr, "%s\n", command);
    
    if (strcmp (command, "s") == 0) {
      printf("\t[system] Sign up\n");
      printf("username?\n[>]: ");
      vallen=getline(&username, &username_len, stdin)-1;username[vallen] = '\0';

      printf("password?\n[>]: ");
      vallen=getline(&password, &password_len, stdin)-1;password[vallen] = '\0';

      printf("\t[system] usr:%s pwd: %s\n", username, password);
    } else if (strcmp(command,"l") == 0) {
      printf("\t[system] Login\n");
      printf("username?\n[>]: ");
      vallen=getline(&username, &username_len, stdin)-1;username[vallen] = '\0';
      printf("%zd", vallen);

      printf("password?\n[>]: ");
      vallen=getline(&password, &password_len, stdin)-1;password[vallen] = '\0';

      printf("\t[system] usr:%s pws: %s\n", username, password);


    } else if (strcmp(command,"a") == 0) {
      printf("\t[system] Show user\n");

    } else if (strcmp(command,"m") == 0) {
      printf("\t[system] Messaging\n");
      printf("to which id?\n[>]: ");
      vallen=getline(&target, &target_len, stdin)-1;target[vallen]='\0';
      printf("Say something\n[>]: ");
      vallen=getline(&msg, &msg_len, stdin)-1;msg[vallen] = '\0';

      printf("\t[system] id:%s msg: %s\n", target, msg);

    } else if (strcmp(command,"r\n") == 0) {
      printf("\t[system] Refresh\n");
      printf("to which id?\n[>]: ");
      vallen=getline(&target, &target_len, stdin)-1;target[vallen]='\0';

      printf("\t[system] id:%s\n", target);
    } else {
      printf("\t[system] Command Not Found\n");
    }

    // printf("\033[%d;%dH", 0, 0);
    printf("\n================\n");
  }

  return 0;
}
