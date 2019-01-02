#include <dirent.h>
#include <unistd.h> 
#include <arpa/inet.h>

#include <bits/stdc++.h>

#include "json.hpp"
#include "const.hpp"
#include "global_variables.hpp"
#include "file_interface.hpp"
#include "respond.hpp"


void initiailization(){
	DIR *dir = opendir("./usr");
	struct dirent *entry;
	while( (entry = readdir(dir)) != NULL ){
		if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0 ){
			int id = atoi(entry->d_name);
			all_users[id] = user_refresh(id);
			username_to_id[all_users[id]->user_name] = id;
		}
	}
}
void sign_up(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;

	std::string username = j["username"].get<std::string>();
	std::string password = j["password"].get<std::string>();
	if(username_to_id.find(username) == username_to_id.end()){
		int id = username_to_id.size();
		while(all_users.find(id) != all_users.end())
			id+=1;
		std::cout << "new registration!" << std::endl;
		std::cout << "username: "<< username << std::endl;
		std::cout << "password: "<< password << std::endl;
		std::cout << "id: "<< id << std::endl;
		struct user_info *u = new struct user_info(id,username,password);
		user_save(u);
		all_users[id] = u;
		username_to_id[username] = id;
		response_client(fd,200,"OK: Sign up successfully!",{});
	
	}else{
		std::cout << "duplicated registration!" << std::endl;
		std::cout << "username: "<< username ;
		std::cout << "password: "<< password << std::endl;	
		response_client(fd,406,"Not Acceptable: Username has been registered!",{});
	}
}
void send_talking_list(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	struct user_info *u = all_users.find(client_fd_to_id[fd])->second;
	std::string target = j["target"].get<std::string>();
	std::vector<json> out;
	for (const auto& kv : all_users) {
		json this_data = json::array({kv.second->user_id,kv.second->user_name});
		if(target == "a")
			out.push_back(this_data);
		if(target == "f" && u->friend_list.find(kv.second->user_name) != u->friend_list.end())
			out.push_back(this_data);
		if(target == "b"  && u->black_list.find(kv.second->user_name) != u->black_list.end())
			out.push_back(this_data);
	}
	response_client(fd,200,"fetch list successfully.",out);
}
void login(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	std::string username = j["username"].get<std::string>();
	std::string password = j["password"].get<std::string>();
	if(client_status[fd] == 'L'){
		response_client(fd,403,"Forbidden: You've logined.",{});
	}else if(username_to_id.find(username) == username_to_id.end()){
		response_client(fd,406,"Not Acceptable: User not exist.",{});
	}else{
		struct user_info *u = all_users.find(username_to_id.find(username)->second)->second;
		std::string p1 = u->user_password;
		if(p1 == password){
			response_client(fd,200,"OK: Login successfully.",{});
			printf("\t\t\t[system] fd(%d) set to L (Logined)\n",fd);
			client_status[fd] = 'L';
			client_fd_to_id[fd] = u->user_id;
		}else{
			printf("Well, I exepected %s",p1.c_str());
			response_client(fd,403,"Forbidden: Password not correct.",{});
		}
	}
}
void messaging(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	int target = j["target"].get<int>();
	std::string type = j["type"].get<std::string>();
	std::string message = j["message"].get<std::string>();
	if(client_status[fd] != 'L'){
		response_client(fd,403,"Forbidden: Not loggined.",{});	
	}else{
		std::string filename = record_init_and_return_filename(client_fd_to_id[fd],target);
		FILE *fout = fopen(filename.c_str(),"a");
		fprintf(fout,"%d %d %s\n",client_fd_to_id[fd],strcmp(type.c_str(),"message")!=0,message.c_str());
		fflush(fout);
		fclose(fout);
		response_client(fd,200,"OK: Record successfully.",{});	
	}
}
void refresh(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	int target = j["target"].get<int>();
	int start_from= j["start_from"].get<int>();
	int end_to= j["end_to"].get<int>();
	if(client_status[fd] != 'L'){
		response_client(fd,403,"Forbidden: Not loggined.",{});	
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
		response_client(fd,200,"fetch history successfully.",history);
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
					unsigned long long len = *((unsigned long long *)(uni_pkt+1));
					switch(uni_pkt[0]){
						case 's':	sign_up(fd,len);			break;
						case 'l':	login(fd,len);				break;
						case 'a':	send_talking_list(fd,len);	break;
						case 'm':	messaging(fd,len);			break;
						case 'r':	refresh(fd,len);			break;
					}
					printf("\t\t\t[system] fd(%d) send strange packet\n",fd);
					printf("len = %llu\n",len);
				}else{ // sz = 0 : client closed.
					finalize_client(fd);
				}
			}
		}
	}
	return 0;
}
