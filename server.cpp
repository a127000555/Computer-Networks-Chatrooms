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
std::map<int, struct user_info*> all_users;
std::map<std::string, int> username_to_id;
char client_status[MAX_FD];	// U : unconnected. C : connected. L : Login
char client_fd_to_id[MAX_FD]; 
int sockfd,retval;
fd_set readset , retset;

void connect_new_client(){
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int new_client_fd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);
	client_status[new_client_fd] = 'I';
	printf("\t\t\t[system] fd(%d) set to I (new connection)\n",new_client_fd);
	FD_SET(new_client_fd,&readset);
}
void initiailization(){
	DIR *dir = opendir("./usr");
	struct dirent *entry;
	while( (entry = readdir(dir)) != NULL ){
		// id, name, len(friend), len(black)
		if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0 ){
			FILE *fin = fopen((std::string("./usr/")+ entry->d_name).c_str() ,"r");
			int id,f_len,b_len;
			char name[MAX_NAME_LEN+1];
			char passwd[MAX_NAME_LEN+1];
			retval = fscanf(fin,"%d %d %d",&id,&f_len,&b_len);
			retval = fscanf(fin,"%s %s",name,passwd);
			printf("load information: %s\n",name);
			all_users[id] = new struct user_info(id,std::string(name),std::string(passwd));
			username_to_id[name] = id;
			fclose(fin);
		}
	}
}
void response_only_msg(int fd,int status_code,const char* msg){
	char response_msg_buffer[8192];
	sprintf(response_msg_buffer,"{\"status_code\":%d,\"state\":\"%s\",\"data\":{}}",status_code,msg);
	unsigned long long msg_len = strlen(response_msg_buffer);
	char response[10]="!";
	*(unsigned long long *)(response+1) = msg_len;
	send(fd,response,9,0);
	send(fd,response_msg_buffer,msg_len,0);
}
void sign_up(int fd,int len){
	char json_content[len]={'\0'};
	ssize_t sz = recv(fd,json_content,len,0);
	json_content[sz] = 0;
	fprintf(stderr,"len= %ld,json_content: %s\n",sz,json_content);
	json j = json::parse(json_content);
	std::string username = j["username"].get<std::string>();
	std::string password = j["password"].get<std::string>();
	if(username_to_id.find(username) == username_to_id.end()){
		int id = username_to_id.size();
		while(all_users.find(id) != all_users.end())
			id+=1;
		std::cout << "new registration!" << std::endl;
		std::cout << "username: "<< username ;
		std::cout << "password: "<< password ;
		std::cout << "id: "<< id << std::endl;
		all_users[id] = new struct user_info(id,username,password);	
		username_to_id[username] = id;	
		FILE *fout = fopen((std::string("./usr/")+ std::to_string(id) + ".txt").c_str() ,"w");
		retval = fprintf(fout,"%d 0 0\n",id);
		retval = fprintf(fout,"%s %s\n",username.c_str(),password.c_str());
		fclose(fout);
		response_only_msg(fd,200,"OK: Sign up successfully!");
	
	}else{
		std::cout << "duplicated registration!" << std::endl;
		std::cout << "username: "<< username ;
		std::cout << "password: "<< password << std::endl;	
		response_only_msg(fd,406,"Not Acceptable: Username has been registered!");
	}
}
void send_talking_list(int fd,struct user_info *u){
	std::vector<json> a,f,b;
	for (const auto& kv : all_users) {
		json this_data = json::array({kv.second->user_id,kv.second->user_name});
		a.push_back(this_data);
    	if(u->friend_list.find(kv.second->user_name) != u->friend_list.end())
			f.push_back(this_data);
    	if(u->black_list.find(kv.second->user_name) != u->black_list.end())
			b.push_back(this_data);
    }
	json j_data={
		{"status_code", 200},
        {"state","fetch list successfully."},
        {"data",{
        		{"user_list",a},
        		{"friend_list",f},
        		{"black_list",b}} 
		}
	};
	std::string content = j_data.dump();
	char response[10]="!";
	*(unsigned long long *)(response+1) = content.length();
	send(fd,response,9,0);
	send(fd,content.c_str(),content.length(),0);
}
void login(int fd,int len){
	char json_content[len]={'\0'};
	ssize_t sz = recv(fd,json_content,len,0);
	json_content[sz] = 0;
	fprintf(stderr,"len= %ld,json_content: %s\n",sz,json_content);
	json j = json::parse(json_content);
	std::string username = j["username"].get<std::string>();
	std::string password = j["password"].get<std::string>();
	if(username_to_id.find(username) == username_to_id.end()){
		response_only_msg(fd,406,"Not Acceptable: User not exist.");
	}else{
		struct user_info *u = all_users.find(username_to_id.find(username)->second)->second;
		std::string p1 = u->user_password;
		if(p1 == password){
			response_only_msg(fd,200,"OK: Login successfully.");
			printf("\t\t\t[system] fd(%d) set to L (Logined)\n",fd);
			client_status[fd] = 'L';
			client_fd_to_id[fd] = u->user_id;
			send_talking_list(fd,u);
		}else{
			response_only_msg(fd,403,"Forbidden: Password not correct.");
		}
	}
}
std::string record_init_and_return_filename(int id1, int id2){
	if(id1 > id2)
		std::swap(id1,id2);
	std::string filename = std::string("./history/") + std::to_string(id1) +"_" + std::to_string(id2) +".txt";
	if( access( filename.c_str(), F_OK ) == -1){
		FILE *fout = fopen(filename.c_str(),"w");
		fprintf(fout, "%d %d 0 0\n",id1,id2);
		fflush(fout);
		fclose(fout);
	}
	return filename;
}
void messaging(int fd,int len){
	char json_content[len]={'\0'};
	ssize_t sz = recv(fd,json_content,len,0);
	json_content[sz] = 0;
	fprintf(stderr,"len= %ld,json_content: %s\n",sz,json_content);
	json j = json::parse(json_content);
    int target = j["target"].get<int>();
    std::string type = j["type"].get<std::string>();
    std::string message = j["message"].get<std::string>();
    if(client_status[fd] != 'L'){
		response_only_msg(fd,403,"Forbidden: Not loggined.");	
    }else{
		std::string filename = record_init_and_return_filename(client_fd_to_id[fd],target);
		FILE *fout = fopen(filename.c_str(),"a");
		fprintf(fout,"%d %d %s\n",client_fd_to_id[fd],strcmp(type.c_str(),"message")!=0,message.c_str());
		fflush(fout);
		fclose(fout);
		response_only_msg(fd,200,"OK: Record successfully.");	
	}
}
void refresh(int fd,int len){
	char json_content[len]={'\0'};
	ssize_t sz = recv(fd,json_content,len,0);
	json_content[sz] = 0;
	fprintf(stderr,"len= %ld,json_content: %s\n",sz,json_content);
	json j = json::parse(json_content);
    int target = j["target"].get<int>();
    int start_from= j["start_from"].get<int>();
    int end_to= j["end_to"].get<int>();
    if(client_status[fd] != 'L'){
		response_only_msg(fd,403,"Forbidden: Not loggined.");	
    }else{
		std::string filename = record_init_and_return_filename(client_fd_to_id[fd],target);
		FILE *fin = fopen(filename.c_str(),"r");
		int id1,id2,id1_read,id2_read;
		retval = fscanf(fin,"%d %d %d %d\n",&id1,&id2,&id1_read,&id2_read);
		int counter = 0, who, type;
		char msg_buf[MAX_DATALEN+1];
		std::map<int,json> history;
		while(fscanf(fin,"%d %d %s\n",&who,&type,msg_buf)!=EOF){
			json j;
			j["message"] = msg_buf;
			j["type"] = type == 0 ? "message" : "data";
			j["who"] = who;
			history[counter] = j;
			counter += 1;
		}
	
		json j_data={
			{"status_code", 200},
	        {"state","fetch history successfully."},
	        {"data",history}
		};
	
		std::string content = j_data.dump();
		char response[10]="!";
		*(unsigned long long *)(response+1) = content.length();
		send(fd,response,9,0);
		send(fd,content.c_str(),content.length(),0);
		fclose(fin);
	}
}

int main(int argc, char **argv){
	if(argc!=2)
		return -1 , puts("usage : piepie-server [port]");
	puts("Pie pie is so pie.");
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
		client_status[i]='U';
	initiailization();
	// start working
	while (1){
		memcpy(&retset,&readset,sizeof(readset));
		retval = select(MAX_FD, &retset, NULL, NULL, NULL);
		for(int fd = 0;fd<MAX_FD;fd++){
			if(!FD_ISSET(fd,&retset))	// nothing happened to this fd.
				continue;
			if(fd == sockfd){	// new connection.
				connect_new_client();
			}else{
				char uni_pkt[10];
				ssize_t sz = recv(fd,uni_pkt,9,0);
				if (sz != 0){
					fprintf(stderr,"%s\n",uni_pkt);
					unsigned long long len = *((unsigned long long *)(uni_pkt+1));
					switch(uni_pkt[0]){
						case 's':
							sign_up(fd,len);
							break;
						case 'l':
							login(fd,len);
							break;
						case 'm':
							messaging(fd,len);
							break;
						case 'r':
							refresh(fd,len);
							break;
						default:
							break;

					}
					printf("\t\t\t[system] fd(%d) send strange packet\n",fd);
					printf("len = %llu\n",len);
				}else{ // sz = 0 : client closed.
					close(fd);
					FD_CLR(fd, &readset);
					client_status[fd] = 'U';
					printf("\t\t\t[system] fd(%d) set to U (unconnection)\n",fd);
				}
			}
		}
	}
	return 0;
}
