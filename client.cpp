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
#include "base64.h"

#define MAX_FD 1024
#define MAX_DATALEN 8192 
#define MAX_PARALLEL 4
#define MAX_NAME_LEN 1024
#define USERNAME_LEN 100
#define PASSWORD_LEN 100
#define MSG_LEN 1000
#define CMD_LEN 10
#define ID_LEN 9

#define IDLE 0
#define MAIN 1
#define LIST 2
#define ROOM 3

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

int STATUS=IDLE;   //status
int CHATTING_TO; //chat to who

void print_command_message()
{
  printf("\n\n========PiePieChat========\n\n");
  switch(STATUS) {
    case IDLE:
      printf("[?] s - sign up\n");
      printf("[?] l - login\n");
      printf("[?] g - good bye\n");
      printf("command?\n[>]: ");
      break;
    case MAIN:
      printf("[?]  - messaging\n");
      printf("[?] x - edit friend/black list\n")
      printf("[?] \n");
      printf("[?] e - logout");
      printf("command?\n[>]: ");
      break;
    case LIST:
      printf("[?] a - show users\n");
      printf("[?] f - show friends\n");
      printf("[?] b - show black list\n");
      printf("[?]  - cha\n", );
      printf("command?\n[>]: ");
      break;
    case ROOM:
      printf("[?] r - refresh\n");
      break;
    default:
      printf("System Error [+]: status%d\n", STATUS);
      break;
  }
  
}

void state_machine(char input) {
  switch(STATUS) {
    case IDLE:
      if (input=='l') STATUS = MAIN;
      if (input=='s') STATUS = MAIN;
      break;
    case MAIN:
      if (input=='m') STATUS = LIST
      break;
    case LIST:
      break;
    case ROOM:
      break;
    default:
      printf("System Error [+]: state%d\n", STATUS);
      break;
  }
}

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


void sign_up(int fd,const char* usr, const char* pwd)
{
  char json_content_req[8192];
  sprintf(json_content_req,"{\"username\":\"%s\",\"password\":\"%s\"}",usr,pwd);
  unsigned long long msg_len_req = strlen(json_content_req);
  char uni_pkt_req[10]="s";
  *(unsigned long long *)(uni_pkt_req+1) = msg_len_req;
  send(fd,uni_pkt_req,9,0);
  send(fd,json_content_req, msg_len_req,0);

  char uni_pkt_res[10];
  ssize_t sz = recv(fd,uni_pkt_res,9,0);
  if (sz == 0){
    return;
  }
  // fprintf(stderr,"%s\n",uni_pkt_res);
  unsigned long long mes_len_res = *((unsigned long long *)(uni_pkt_res+1));
  printf("mes_len_res = %llu\n", mes_len_res);

  char json_content_res[mes_len_res]={'\0'};
  sz = recv(fd,json_content_res,mes_len_res,0);
  json_content_res[sz] = 0;
  // fprintf(stderr,"mes_len_res= %ld,json_content_res: %s\n",sz,json_content_res);
  json j = json::parse(json_content_res);
  int status_code = j["status_code"].get<int>();
  std::string state = j["state"].get<std::string>();
  std::cout << "[+] " << state;

  if (status_code == 200) {
    state_machine('s');
  }
}

void login(int fd, const char* usr, const char* pwd)
{
  char json_content_req[8192];
  sprintf(json_content_req,"{\"username\":\"%s\",\"password\":\"%s\"}",usr,pwd);
  unsigned long long msg_len_req = strlen(json_content_req);
  char uni_pkt_req[10]="l";
  *(unsigned long long *)(uni_pkt_req+1) = msg_len_req;
  send(fd,uni_pkt_req,9,0);
  send(fd,json_content_req, msg_len_req,0);

  char uni_pkt_res[10];
  ssize_t sz = recv(fd,uni_pkt_res,9,0);
  if (sz == 0){
    return;
  }
    // fprintf(stderr,"%s\n",uni_pkt_res);
  unsigned long long mes_len_res = *((unsigned long long *)(uni_pkt_res+1));
  char json_content_res[mes_len_res]={'\0'};
  sz = recv(fd,json_content_res,mes_len_res,0);
  json_content_res[sz] = 0;
  fprintf(stderr,"mes_len_res= %ld,json_content_res: %s\n",sz,json_content_res);
  json j = json::parse(json_content_res);
  int status_code = j["status_code"].get<int>();
  std::string state = j["state"].get<std::string>();

  std::cout << "[+] " << state;

  if (status_code == 200) {
    char uni_pkt_res2[10];
    sz = recv(fd,uni_pkt_res2,9,0);
    if (sz != 0){
      // fprintf(stderr,"%s\n",uni_pkt_res2);
      unsigned long long mes_len_res2 = *((unsigned long long *)(uni_pkt_res2+1));
      char json_content_res2[mes_len_res2]={'\0'};
      sz = recv(fd,json_content_res2,mes_len_res2,0);
      json_content_res2[sz] = 0;
      fprintf(stderr,"mes_len_res2= %ld,json_content_res2: %s\n",sz,json_content_res2);
      json user_info = json::parse(json_content_res2);
      // std::string state = user_info["data"]["user_list"].get<std::string>();
      // std::cout << "[+] " << state;
    }

    state_machine('l');
  }
}

void messaging(int fd, int target, const char* message)
{
  // char json_content_req[8192];
  std::string message_str(message);
  // std::cout << message_str << std::endl;
  std::string encoded_message_str = base64_encode(reinterpret_cast<const unsigned char*>(message_str.c_str()), message_str.length());
  // std::cout << encoded_message_str << std::endl;
  json j_req;
  j_req["target"] = target; j_req["type"]="message"; j_req["message"]=encoded_message_str;
  std::string json_content_req = j_req.dump();

  unsigned long long msg_len_req = json_content_req.length();
  char uni_pkt_req[10]="m";
  *(unsigned long long *)(uni_pkt_req+1) = msg_len_req;
  send(fd,uni_pkt_req,9,0);
  send(fd,json_content_req.c_str(), msg_len_req, 0);

  char uni_pkt_res[10];
  ssize_t sz = recv(fd,uni_pkt_res,9,0);
  if (sz == 0){
    return;
  }
  // fprintf(stderr,"%s\n",uni_pkt_res);
  unsigned long long mes_len_res = *((unsigned long long *)(uni_pkt_res+1));
  printf("mes_len_res = %llu\n", mes_len_res);

  char json_content_res[mes_len_res]={'\0'};
  sz = recv(fd,json_content_res,mes_len_res,0);
  json_content_res[sz] = 0;
  // fprintf(stderr,"mes_len_res= %ld,json_content_res: %s\n",sz,json_content_res);
  json j = json::parse(json_content_res);
  std::string state = j["state"].get<std::string>();
  std::cout << "[+] " << state;

}

void refresh(int fd, int target, int start, int end) 
{
  char json_content_req[8192];
  sprintf(json_content_req,"{\"target\":%d,\"start_from\":%d,\"end_to\":%d}",target,start,end);
  unsigned long long msg_len_req = strlen(json_content_req);
  char uni_pkt_req[10]="r";
  *(unsigned long long *)(uni_pkt_req+1) = msg_len_req;
  send(fd,uni_pkt_req,9,0);
  send(fd,json_content_req, msg_len_req,0);

  char uni_pkt_res[10];
  ssize_t sz = recv(fd,uni_pkt_res,9,0);
  if (sz == 0){
    return;
  }
  // fprintf(stderr,"%s\n",uni_pkt_res);
  unsigned long long mes_len_res = *((unsigned long long *)(uni_pkt_res+1));
  printf("mes_len_res = %llu\n", mes_len_res);

  char json_content_res[mes_len_res]={'\0'};
  sz = recv(fd,json_content_res,mes_len_res,0);
  json_content_res[sz] = 0;
  // fprintf(stderr,"mes_len_res= %ld,json_content_res: %s\n",sz,json_content_res);
  json j = json::parse(json_content_res);
  std::string state = j["state"].get<std::string>();
  std::cout << "[+] " << state << std::endl;
  // std::cout << j["data"][0];
  int line_num = 0;
  for(auto x : j["data"]){
    // std::cout << x << std::endl;
    line_num = x[0].get<int>();
    std::string message = x[1]["message"].get<std::string>();
    std::string decoded_message = base64_decode(message);
    int who = x[1]["who"].get<int>();
    std::cout << "[" << line_num << "]user" << who << ": " << decoded_message << std::endl;
  }
}

int main(int argc, char const *argv[])
{
  if(argc != 2){
    return -1 , fprintf(stderr, "usage : piepie-client [host]:[port]");
  }
  fprintf(stderr, "Pie pie is so pie.\n");

  int sockfd,retval;
  char const *split = ":";
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
  int target_num; 

  // Chat room while
  while (true) {
    print_command_message();

    vallen = getline(&command, &cmd_len, stdin) - 1;
    command[vallen] = '\0';
    // printf("%d\n", vallen);
    // fprintf(stderr, "%s\n", command);
    
    if (strcmp (command, "s") == 0) {
      printf("\n========Sign up========\n\n");
      printf("[+] username?\n[>]: ");
      vallen=getline(&username, &username_len, stdin)-1;username[vallen] = '\0';

      printf("[+] password?\n[>]: ");
      vallen=getline(&password, &password_len, stdin)-1;password[vallen] = '\0';

      printf(" usr:%s pwd: %s\n", username, password);
      sign_up(sockfd, username, password);
    } else if (strcmp(command,"l") == 0) {
      printf("\n========Login========\n\n");
      printf("[+] username?\n[>]: ");
      vallen=getline(&username, &username_len, stdin)-1;username[vallen] = '\0';

      printf("[+] password?\n[>]: ");
      vallen=getline(&password, &password_len, stdin)-1;password[vallen] = '\0';

      printf("[+] usr:%s pws: %s\n", username, password);
      login(sockfd, username, password);
    } else if (strcmp(command,"a") == 0) {
      printf("[+] Show user\n");

    } else if (strcmp(command,"m") == 0) {
      printf("\n========Messaging========\n\n");
      state_machine("m");
    }
      printf("[+] to which id?\n[>]: ");
      vallen=getline(&target, &target_len, stdin)-1;target[vallen]='\0';
      printf("[+] Say something\n[>]: ");
      vallen=getline(&msg, &msg_len, stdin)-1;msg[vallen] = '\0';
      target_num = atoi(target);

      printf("[+] id:%d msg: %s\n", target_num, msg);
      if (target_num > 0) {
        messaging(sockfd, target_num, msg);
      } else {
        fprintf(stderr, "[+] Target not found!\n");
      }

    } else if (strcmp(command,"r") == 0) {
      printf("\n========Refresh========\n\n");
      printf("[+] to which id?\n[>]: ");
      vallen=getline(&target, &target_len, stdin)-1;target[vallen]='\0';
      target_num = atoi(target);

      printf("[+] id:%d\n", target_num);
      if (target_num > 0) {
        refresh(sockfd, target_num, 0, 0);
      } else {
        fprintf(stderr, "[+] Target not found!\n");
      }
      
    } else if (strcmp(command, "q") == 0) {
      state_machine('q')
    } else if (strcmp(command, "e") == 0) {
      close(sockfd);
      printf("[+] ByeBye~~\n");
      break;
    } else {
      printf("[+] Command Not Found!\n");
    }

    // printf("\033[%d;%dH", 0, 0);
    // for (int i = 0; i < 100; i++) {
    //   printf("\t\t\t\t\t\t\t\t\t\t\n");  
    // }
    // printf("\033[%d;%dH", 0, 0);
  }

  return 0;
}
