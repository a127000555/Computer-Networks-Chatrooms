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
#include <iomanip>

#include "json.hpp"
#include "base64.h"

#define MAX_FD 1024
#define MAX_DATALEN 8192 
#define USERNAME_LEN 100
#define PASSWORD_LEN 100
#define FILE_LEN 100
#define MSG_LEN 1024
#define CMD_LEN 10
#define ID_LEN 9

#define IDLE 0
#define MAIN 1
#define LIST 2
#define CHOOSE 3
#define ROOM 4

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
  // printf("\n=after status=%d\n", STATUS);

  switch(STATUS) {
    case IDLE:
      printf("\n========PiePieChat========\n\n");
      printf("[?] s - sign up\n");
      printf("[?] l - login\n");
      printf("[?] g - good bye\n");
      printf("[+] Command?\n[>]: ");
      break;
    case MAIN:
      printf("\n========PiePieChat========\n\n");
      printf("[?] m - messaging\n");
      printf("[?] t - show lists\n");
      printf("[?] x - edit friend/black list -> not\n");
      printf("[?] e - logout -> maybe don't have to close process\n");
      printf("[+] Command?\n[>]: ");
      break;
    case LIST:
      printf("\n==========Lists===========\n\n");
      printf("[?] a - show users\n"); 
      printf("[?] d - show friends\n");
      printf("[?] b - show black list\n");
      printf("[?] m - messaging\n");
      printf("[?] q - quit\n");
      printf("[+] Command?\n[>]: ");
      break;
    case CHOOSE: // choose people to chat
      printf("\n===========Who============\n\n");
      printf("[?] \"id\" - chat with who\n");
      printf("[?] q - quit\n");
      printf("[+] ID or Quit?\n[>]: ");
      // printf("[+] Chat with who?\n[>]: ");
      break;
    case ROOM:
      printf("\n===========Chat===========\n\n");
      printf("[?] \"msg\" - sending messaging\n");
      printf("[?] u - upload file\n");
      printf("[?] f - fetch file\n");
      printf("[?] r - refresh\n");
      printf("[?] q - quit\n");
      printf("[+] MSG or Command?\n[>]: ");
      break;
    default:
      printf("System Error [+]: status%d\n", STATUS);
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
  char json_content_req[MAX_DATALEN];
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
  std::cout << "[+] " << state << std::endl;
}

int login(int fd, const char* usr, const char* pwd)
{
  char json_content_req[MAX_DATALEN];
  sprintf(json_content_req,"{\"username\":\"%s\",\"password\":\"%s\"}",usr,pwd);
  unsigned long long msg_len_req = strlen(json_content_req);
  char uni_pkt_req[10]="l";
  *(unsigned long long *)(uni_pkt_req+1) = msg_len_req;
  send(fd,uni_pkt_req,9,0);
  send(fd,json_content_req, msg_len_req,0);

  char uni_pkt_res[10];
  ssize_t sz = recv(fd,uni_pkt_res,9,0);
  if (sz == 0){
    return -1;
  }
    // fprintf(stderr,"%s\n",uni_pkt_res);
  unsigned long long mes_len_res = *((unsigned long long *)(uni_pkt_res+1));
  char json_content_res[mes_len_res]={'\0'};
  sz = recv(fd,json_content_res,mes_len_res,0);
  json_content_res[sz] = 0;
  // fprintf(stderr,"mes_len_res= %ld,json_content_res: %s\n",sz,json_content_res);
  json j = json::parse(json_content_res);
  int status_code = j["status_code"].get<int>();
  std::string state = j["state"].get<std::string>();
  std::cout << "[+] " << state << std::endl;;

  return status_code;
}

void get_list(int fd, char target) {
  char json_content_req[MAX_DATALEN];
  sprintf(json_content_req,"{\"target\":\"%c\"}",target);
  unsigned long long msg_len_req = strlen(json_content_req);
  char uni_pkt_req[10]="a";
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
  // fprintf(stderr,"mes_len_res= %ld,json_content_res: %s\n",sz,json_content_res);
  json j = json::parse(json_content_res);
  std::cout << j << std::endl;

  int id = 0;
  printf("\n=========List=========\n");
  for(auto x : j["data"]){
    // std::cout << x << std::endl;
    id = x[0].get<int>();
    std::string name = x[1].get<std::string>();
    std::cout << std::left << std::setw(20) <<  name << std::left << std::setw(5) << id << std::endl;
  }
  printf("=======List END=======\n");

}

std::string endecrypt(std::string msg)
{
  for(char& c : msg) {
    c = (c+64)%128;
  }
  return msg;
}

void messaging(int fd, int target, char* message)
{
  std::string message_str(message);
  //encryption
  message_str = endecrypt(message_str);
  
  //base64 encode
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
  std::cout << "[+] " << state << std::endl;;
}

void refresh(int fd, int target, int start, int end) 
{
  char json_content_req[MAX_DATALEN];
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

    //base64 decode
    std::string decoded_message = base64_decode(message);
    //decrypt
    decoded_message = endecrypt(decoded_message);

    int who = x[1]["who"].get<int>();
    std::cout << "[" << line_num << "]user" << who << ": " << decoded_message << std::endl;
  }
}

int upload(int fd,  const char* filename)
{
  return 0;
}

void input(char *buf, size_t len) {
  int val = getline(&buf, &len, stdin)-1;
  buf[val] = '\0';
}

int main(int argc, char const *argv[])
{
  if(argc != 2){
    return -1 , fprintf(stderr, "usage : piepie-client [host]:[port]");
  }
  fprintf(stderr, "Pie pie is so pie.\n");

  
  // init parsing argv
  // -----input:argv[1] -----
  // -----output:hostname, port -----
  char const *split = ":";
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

  while(1){
    // fork for auto reconnect
    pid_t pid = fork();
    if (pid > 0) {
      // parent process
      // fprintf(stderr, "parent\n");
      // init socket
      int monitor_sockfd = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in serverInfo;
      bzero(&serverInfo, sizeof(serverInfo));
      serverInfo.sin_family = PF_INET;
      serverInfo.sin_addr.s_addr = inet_addr(hostname);
      serverInfo.sin_port = htons(atoi(port));
      int retval;
      while (1){
        retval = connect(monitor_sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
        if (retval >= 0) break;
      }
      assert(retval >= 0);

      // 宣告 select() 使用的資料結構
      fd_set readset;
      fd_set working_readset;
      FD_ZERO(&readset);
      FD_SET(monitor_sockfd, &readset);
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 10;
      int end_whole = 0;
      // int end_process=0;
      while(1) {
        int *status;
        pid_t p = waitpid(pid, status, WNOHANG);
        if (p != 0) {
          end_whole = 1;
          break;
        }
        // 宣告select要用的working_readset
        FD_ZERO(&working_readset);
        memcpy(&working_readset, &readset, sizeof(fd_set));

        retval = select(MAX_FD, &working_readset, NULL, NULL, &timeout);
        if (retval < 0) { // 發生錯誤
          fprintf(stderr, "select() went wrong");
          return 0;
        } else if (!FD_ISSET(monitor_sockfd, &working_readset)){
          continue;
        } else {
          fprintf(stderr, "\n[!] Server is down!\n");
          kill(pid, SIGKILL);
          break;
        }
      }
      if (end_whole) break; 

    } else if (pid == 0) {
      // child process
      // fprintf(stderr, "child\n");
      // init socket
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in serverInfo;
      bzero(&serverInfo, sizeof(serverInfo));
      serverInfo.sin_family = PF_INET;
      serverInfo.sin_addr.s_addr = inet_addr(hostname);
      serverInfo.sin_port = htons(atoi(port));
      int retval;
      while (1) {
        retval = connect(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
        if (retval >= 0) break;
      }
      assert(retval >= 0);

      fprintf(stderr, "[!] Connection Success!\n");

      // Some variable for chat room command
      char* command = (char*)malloc(sizeof(char) * CMD_LEN);
      char* username = (char*)malloc(sizeof(char) * USERNAME_LEN);
      char* password = (char*)malloc(sizeof(char) * PASSWORD_LEN);
      char* filename = (char*)malloc(sizeof(char) * FILE_LEN);
      char* target = (char*)malloc(sizeof(char) * ID_LEN);
      char* msg = (char*)malloc(sizeof(char) * MSG_LEN);
      int target_num;
      
      // Flag for ending while loop
      int goodbye = 0;

      // Chat room while
      while (!goodbye) {
        print_command_message();

        switch(STATUS) {
        case IDLE:
          input(command, CMD_LEN);
          if (strcmp (command, "s") == 0) {
            printf("\n=========Sign Up==========\n\n");
            printf("[+] username?\n[>]: ");
            input(username, USERNAME_LEN);

            printf("[+] password?\n[>]: ");
            input(password, PASSWORD_LEN);

            printf(" usr:%s pwd: %s\n", username, password);
            sign_up(sockfd, username, password);
          } else if (strcmp(command,"l") == 0) {
            printf("\n==========Login===========\n\n");
            printf("[+] username?\n[>]: ");
            input(username, USERNAME_LEN);

            printf("[+] password?\n[>]: ");
            input(password, PASSWORD_LEN);

            printf(" usr:%s pws: %s\n", username, password);
            int status_code = login(sockfd, username, password);

            if (status_code == 200) {
              STATUS = MAIN;
            }
          } else if (strcmp(command,"g") == 0) {
            printf("\n========PiePieChat========\n");
            close(sockfd);
            goodbye = 1;
            printf("[+] ByeBye~~\n");
          } else {
            printf("[+] Command not found!\n");
          }
          break;
        case MAIN:
          input(command, CMD_LEN);
          if (strcmp(command,"m") == 0) {
            STATUS = CHOOSE;
          } else if (strcmp(command, "x") == 0) {
            //edit list
          } else if (strcmp(command,"t") == 0) {
            STATUS = LIST;
          } else if (strcmp(command,"e") == 0) { 
            STATUS = IDLE;
            close(sockfd);
            goodbye = 1;
            printf("[+] ByeBye~~\n");
          } else {
            printf("[+] Command not found!\n");
          }
          break;
        case LIST:
          input(command, CMD_LEN);
          if (strcmp(command,"m") == 0) {
            STATUS = CHOOSE;
          } else if (strcmp(command,"a") == 0) {
            //show friends
            printf("[+] Show user\n");
            get_list(sockfd, 'a');
          } else if (strcmp(command,"d") == 0) {
            //show friends
            get_list(sockfd, 'f');
          } else if (strcmp(command,"b") == 0) {
            //show blacklist
            get_list(sockfd, 'b');
          } else if (strcmp(command, "q") == 0) {
            STATUS = MAIN;
          } else {
            printf("[+] Command not found!\n");
          }
          break;
        case CHOOSE:
          input(command, ID_LEN);
          if (strcmp(command,"q") == 0) {
            STATUS = MAIN;
          } else {
            target_num = atoi(command);
            printf("[+] id:%d\n", target_num);
            if (target_num > 0) {
              CHATTING_TO = target_num;
              STATUS = ROOM;
              refresh(sockfd, CHATTING_TO, 0, 0);
            } else {
              fprintf(stderr, "[+] Target not found!\n");
            }
          }
          break;
        case ROOM:
          input(msg, MSG_LEN);
          if (strcmp(msg,"r") == 0) {
            printf("\n=========Sign Up==========\n\n");
            printf("\n=========Refresh==========\n\n");
            refresh(sockfd, CHATTING_TO, 0, 0);
          } else if (strcmp(msg,"u") == 0) {
            //upload file
            printf("\n=========Upload============\n\n");
            printf("[+] filename?\n[>]: ");
            input(filename, FILE_LEN);

            printf(" filename: %s\n", filename);
            int status_code = upload(sockfd, filename);
          } else if (strcmp(msg,"f") == 0) {
            //fetch file
          } else if (strcmp(msg,"q") == 0) {
            CHATTING_TO = 0;
            STATUS = MAIN;
          } else {
            printf("[+] id:%d msg: %s\n", target_num, msg);
            messaging(sockfd, CHATTING_TO, msg);
            refresh(sockfd, CHATTING_TO, 0, 0);
          }
          break;
        default:
          printf("System Error [+]: state%d\n", STATUS);
          break;
        }

        // printf("\033[%d;%dH", 0, 0);
        // for (int i = 0; i < 100; i++) {
        //   printf("\t\t\t\t\t\t\t\t\t\t\n");  
        // }
        // printf("\033[%d;%dH", 0, 0);
      }
      break;
    } else {
      fprintf(stderr, "Error!\n");
    }
  }
  return 0;
}
