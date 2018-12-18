#include <queue>
#include <vector>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <iostream>
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
#include "myparse.hpp"

#define MAX_FD 1024
#define MAX_DATALEN 8192 
#define MAX_PARALLEL 4
#define my_mmap(size) mmap(NULL, size , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS , -1 , 0)
using json = nlohmann::json;

User client_data[MAX_FD];
json client_json_data[MAX_FD];
char *client_status = (char *)my_mmap(MAX_FD);	// U : unconnected. I : idle M : matching T : talking C: child process 
std::string client_buffer_string[MAX_FD];
int client_match[MAX_FD];
int client_itself[MAX_FD];
std::vector<int> waiting;
std::queue<int> thread_queue;
int sockfd,retval;
fd_set readset , retset;

const char header[]="struct User{char name[33];unsigned int age;char gender[7];char introduction[1025];};";
const char try_match[]="{\"cmd\":\"try_match\"}\n";
const char other_side_quit[] ="{\"cmd\":\"other_side_quit\"}\n";
const char quit[] ="{\"cmd\":\"quit\"}\n";

void invoke_thread();
void pop_up(int);
void connect_new_client(){
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int new_client_fd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);
	client_status[new_client_fd] = 'I';
	printf("\t\t\t[system] fd(%d) set to I (new connection)\n",new_client_fd);
	FD_SET(new_client_fd,&readset);
}
void create_so(int fd,std::string filter_function){
	const char *c_filter = filter_function.c_str();
	char file_name[MAX_DATALEN],so_name[MAX_DATALEN];
	sprintf(file_name,"client%d.c",fd);
	sprintf(so_name,"client%d.so",fd);
	FILE *obj = fopen( file_name , "w" );
	fprintf(obj,"%s%s",header,c_filter);
	fclose(obj);
	retval = system((std::string() + "gcc -fPIC -O2 -std=c11 -shared -o " + so_name +" " + file_name + " 2> /dev/null ").c_str());
	// if compile error, then so file is not exist. Thus, match_ok will fail -> turns out always false.
	remove(file_name);
}
void send_portfolio(int fd_data,int fd_target){
	char data[MAX_DATALEN]={0};
	j2s_portfolio(data,client_json_data[fd_data]);
	send(fd_target,data,strlen(data),0);
	printf("\t\t\t[system] fd(%d) set to T (talking)\n",fd_target);
}
bool match_ok(int wait_fd,int test_fd,bool *result){
	*result = false;
	pid_t pid = fork();
	if(pid==0){ // child process
		char so_name[MAX_DATALEN];
		sprintf(so_name,"./client%d.so",wait_fd);
		void* handle = dlopen(so_name, RTLD_LAZY);
		bool (*check_fn)(struct User) = (bool (*)(struct User)) dlsym(handle, "filter_function");
		*result = check_fn(client_data[test_fd]);
		dlclose(handle);
		_exit(0);
	}else{
		int status;
		while(!waitpid(pid,&status,WNOHANG))
			if(client_status[wait_fd] != 'M' || client_status[wait_fd] != 'M')
				kill(pid,SIGKILL);
		return *result;
	}
}
bool try_to_match(int fd1,int fd2){
	bool *match_blocks = (bool *)my_mmap(sizeof(bool)*2);
	pid_t pid = fork();
	if(pid==0){	//child process
		match_ok(fd2,fd1,match_blocks);
		_exit(0);
	}
	int status;
	match_ok(fd1,fd2,match_blocks+1);
	waitpid(pid,&status,0);
	bool result = match_blocks[0] && match_blocks[1];
	printf("\t\t\ttry to match %d <-> %d : %d|%d\n",fd1,fd2,match_blocks[0],match_blocks[1]);
	munmap(match_blocks,sizeof(bool)*2);
	return result;
}
void connect_two_client(int fd1,int fd2){
	client_match[fd1] = fd2;
	client_match[fd2] = fd1;
	client_status[fd1] = client_status[fd2] = 'T';
	send_portfolio(fd1,fd2);
	send_portfolio(fd2,fd1);	
}
// !!! this function is for thread !!!
int *final_matching_idx =  (int *)my_mmap(sizeof(int));
int matching_idx = 0;
std::vector<int> static_waiting;
void *matching(void *data){
	// initialize
	static_waiting = waiting;
	matching_idx = 0;
	int fd_new = *(int *)data , status;
	std::vector<pid_t> working_children;
	*final_matching_idx = -1;
	while( (*final_matching_idx) == -1 && matching_idx < static_waiting.size()){
		// I need some fork
		pid_t now_fork;
		if( (now_fork = fork()) == 0){	//child
			int fd_old = static_waiting[matching_idx];
			if(try_to_match(fd_old,fd_new) && ( (*final_matching_idx) == -1 || (*final_matching_idx) > matching_idx)){
				(*final_matching_idx) = matching_idx;
			}
			_exit(0);
		}
		working_children.push_back(now_fork);
		matching_idx ++;
		if(working_children.size() == MAX_PARALLEL){ 
			pid_t find_pid = wait(&status);
			std::vector<int>::iterator it = std::find (working_children.begin(),working_children.end(), find_pid);
  			if (it != working_children.end())
  				working_children.erase(it);
		}
	}
	while(!working_children.empty()){	// wait till all child died.
		pid_t find_pid = wait(&status);
		std::vector<int>::iterator it = std::find (working_children.begin(),working_children.end(), find_pid);
		if (it != working_children.end())
			working_children.erase(it);
	}
	if( (*final_matching_idx) == -1){	// no one can pair
		if(client_status[fd_new]=='M')
			waiting.push_back(fd_new); 
	}else{
		printf("\t\t\t\033[31mconnecting : %d <-> %d\033[0m\n",static_waiting[*final_matching_idx],fd_new);
		connect_two_client(static_waiting[*final_matching_idx],fd_new);
		pop_up(static_waiting[*final_matching_idx]);
	}
	thread_queue.pop();
	static_waiting = waiting;
	invoke_thread();
	}
// !!! this function is for thread !!!
void invoke_thread(){
	pthread_t tid;
	if(!thread_queue.empty()){
		pthread_create(&tid, NULL, matching, (void *)&client_itself[thread_queue.front()]);
	}
}

void pop_up(int fd){
	for(int i=0;i<waiting.size();i++){		// pop up from the waiting queue.
		if(waiting[i]==fd){
			waiting.erase(waiting.begin()+i);
			return ;
		}
	}
	for(int i=0;i<static_waiting.size();i++){		// pop up from the waiting queue.
		if(static_waiting[i]==fd){
			if(i>matching_idx)
				waiting.erase(waiting.begin()+i);
			return ;
		}
	}
}
int main(int argc, char **argv){
	if(argc!=2)
		return -1 , puts("usage : inf-bonbon-server [port]");
	// initiailization bind to server
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	struct sockaddr_in addr={PF_INET,htons(atoi(argv[1])),{htonl(INADDR_ANY)}};
	retval = bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));	
	if(retval){
		puts("socket fail");
		return 0;
	}
	retval = listen(sockfd, MAX_FD);
	FD_SET(sockfd,&readset);
	// initialize varabiles
	for(int i=0;i<MAX_FD;i++)
		client_status[i]='U',client_itself[i]=i;
	// start working
	while (1){
		memcpy(&retset,&readset,sizeof(readset));
		retval = select(MAX_FD, &retset, NULL, NULL, NULL);
		for(int fd = 0;fd<MAX_FD;fd++){
			//for(int x:waiting)printf("%d ",x);
			//for(int i=0;i<20;i++)printf(" ");printf("\r");
			if(!FD_ISSET(fd,&retset))	// nothing happened to this fd.
				continue;
			if(fd == sockfd){	// new connection.
				connect_new_client();
			}else{
				char ttmp[MAX_DATALEN];
				ssize_t sz = recv(fd,ttmp,MAX_DATALEN,0);
				for(int i=0;i<sz;i++){
					client_buffer_string[fd].push_back(ttmp[i]);
				}
				if (sz == 0){	// client closed.
					close(fd);
					FD_CLR(fd, &readset);
					if(client_status[fd]=='T'){	// quit when talking
						send(client_match[fd],other_side_quit,strlen(other_side_quit),0);
						client_status[client_match[fd]] = 'I'; //true other side to state idle.
						printf("\t\t\t[system] fd(%d) set to I (idle)\n",client_match[fd]);	
					}
					char so_name[MAX_DATALEN];
					sprintf(so_name,"client%d.so",fd);
					remove(so_name);
					client_status[fd]='U'; 
					printf("\t\t\t[system] fd(%d) set to U\n",fd);
					pop_up(fd);
				}else{
					std::size_t idx;
					while((idx = client_buffer_string[fd].find_first_of("\n")) != std::string::npos){
						std::string clause = client_buffer_string[fd].substr(0,idx+1);
						client_buffer_string[fd].erase(0,idx+1);
						json j;
						//std::cout << fd << "transmit an message, "<<" cmd : " << json::parse(clause)["cmd"].get<std::string>()<<std::endl;
						switch(client_status[fd]){
							case 'I':; 		// (idle->matching) recv & send try-match;
								send(fd,try_match,strlen(try_match),0);
								client_json_data[fd] = json::parse(clause);
								j2u_information(&client_data[fd],client_json_data[fd]);
								create_so(fd,client_json_data[fd].at("filter_function").get<std::string>());
								client_status[fd] = 'M'; 	// matching.
								printf("\t\t\t[system] fd(%d) set to M (matching)\n",fd);
								pthread_t tid;
								thread_queue.push(fd);
								if(thread_queue.size() == 1)
									invoke_thread();	
									
								break;
							case 'T':;		// (talking -> talking/idle)
								j = json::parse(clause);
								if(j["cmd"].get<std::string>().compare(std::string("send_message"))==0){	// talking
									char temp[MAX_DATALEN]={0};
									memcpy(temp,clause.c_str(),clause.length());
									send(fd,temp,strlen(temp),0);
									memset(temp,0,sizeof(temp));
									send_to_recv(temp,j);
									send(client_match[fd],temp,strlen(temp),0);
									break;
								}else{	// talking -> idle
									send(client_match[fd],other_side_quit,strlen(other_side_quit),0);
									client_status[client_match[fd]] = 'I'; //true other side to state idle.
									printf("\t\t\t[system] fd(%d) set to I (idle)\n",client_match[fd]);
								}
							case 'M':;		// (matching -> idle)
								send(fd,quit,strlen(quit),0);	// send "cmd:quit" back.
								client_status[fd] = 'I'; 		// turn other side to state idle.
								pop_up(fd);
								printf("\t\t\t[system] fd(%d) set to I (idle)\n",fd);
								break;
						}
					}
				}
			}
		}
	}
	return 0;
}
