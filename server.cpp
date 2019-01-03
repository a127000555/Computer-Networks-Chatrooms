#include <dirent.h>
#include <unistd.h> 

#include <sys/stat.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>

#include "json.hpp"
#include "picosha2.h"
#include "const.hpp"
#include "global_variables.hpp"
#include "file_interface.hpp"
#include "respond.hpp"
std::string string_to_sha256(std::string src_str){
	std::vector<unsigned char> hash(picosha2::k_digest_size);
	picosha2::hash256(src_str.begin(), src_str.end(), hash.begin(), hash.end());
	return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}
void signalHandler( int signum ) {
	longjmp(jmpbuffer, 0);
}
void initiailization(){
	DIR *dir = opendir("./history");
	if(!dir)
		mkdir("./history",0755);
	dir = opendir("./file_save");
	if(!dir)
		mkdir("./file_save",0755);
	dir = opendir("./usr");
	if(!dir){
		mkdir("./usr",0755);
		initiailization();
	}else{
		struct dirent *entry;
		while( (entry = readdir(dir)) != NULL ){
			if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0 ){
				int id = atoi(entry->d_name);
				all_users[id] = user_refresh(id);
				username_to_id[all_users[id]->user_name] = id;
			}
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
		password = string_to_sha256(password);
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
	if(client_status[fd] != 'L'){
		response_client(fd,403,"Forbidden: Not loggined.",{});	
	}else{
		struct user_info *u = all_users.find(client_fd_to_id[fd])->second;
		std::string target = j["target"].get<std::string>();
		std::vector<json> out;
		for(int i:u->black_list){
			printf("black : %d\n",i);
		}
		for (const auto& kv : all_users) {
			json this_data = json::array({kv.second->user_id,kv.second->user_name});
			if(target == "a")
				out.push_back(this_data);
			if(target == "f" && u->friend_list.find(kv.second->user_id) != u->friend_list.end())
				out.push_back(this_data);
			if(target == "b"  && u->black_list.find(kv.second->user_id) != u->black_list.end())
				out.push_back(this_data);
		}
		response_client(fd,200,"fetch list successfully.",out);
	}
}
void edit_list(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	if(client_status[fd] == 'I'){
		response_client(fd,403,"Forbidden: Not loggined.",{});		
	}else{
		int id = client_fd_to_id[fd];
		struct user_info* u = all_users[id];
		if(u){
			std::string target_list = j["target_list"].get<std::string>();
			std::string op = j["op"].get<std::string>();
			int target_id = j["target_id"].get<int>();
			if(all_users[target_id]){
				if(op == "add" && target_list == "friend")
					u->friend_list.insert(target_id);
				else if(op == "add" && target_list == "black")
					u->black_list.insert(target_id);
				else if(op == "delete" && target_list == "friend")
					u->friend_list.erase(target_id);
				else if(op == "delete" && target_list == "black")
					u->black_list.erase(target_id);
				else{
					response_client(fd,406,"Not Acceptable: What are you type? bitch.",{});
					return ;
				}
				user_save(u);
				response_client(fd,200,"Wow, I complete the mission:).",{});
			}else{
				response_client(fd,406,"Not Acceptable: Target id not exists.",{});
			}
		}else{
			response_client(fd,406,"Not Acceptable: User not exist or not login.",{});
		}
	}
}
void login(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	std::string username = j["username"].get<std::string>();
	std::string password = j["password"].get<std::string>();	
	password = string_to_sha256(password);
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
		if(all_users[client_fd_to_id[fd]]->black_list.find(target) != all_users[client_fd_to_id[fd]]->black_list.end()){
			response_client(fd,403,"Forbidden: This man is in black list, please forgive him/her :).",{});
			return;
		}
		std::string filename = record_init_and_return_filename(client_fd_to_id[fd],target);
		FILE *fout = fopen(filename.c_str(),"a");
		fprintf(fout,"%d %d %s\n",client_fd_to_id[fd],strcmp(type.c_str(),"message")!=0,message.c_str());
		fclose(fout);
		response_client(fd,200,"OK: Record successfully.",{});	
	}
}
void id_to_username(int fd,int len){
json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	int target = j["target"].get<int>();	
	if(all_users[target])
		response_client(fd,200,"OK: Record successfully.",all_users[target]->user_name);
	else
		response_client(fd,404,"User not found.",{});
}
void upload_file(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	int target = j["target"].get<int>();
	std::string filename = j["filename"].get<std::string>();
	std::string data = j["data"].get<std::string>();
	std::string hash = string_to_sha256(data);
	if(client_status[fd] != 'L'){
		response_client(fd,403,"Forbidden: Not loggined.",{});	
	}else{
		int file_id = abs(rand()*rand());
		std::string filepath = "./file_save/" + filename + "_" + std::to_string(file_id) + "_" + hash;
		FILE *fout = fopen(filepath.c_str(),"w");
		printf("write to %s",filepath.c_str());
		fprintf(fout,"%s",data.c_str());
		fclose(fout);
		printf("write end ");
		
		std::string filename2 = record_init_and_return_filename(client_fd_to_id[fd],target);
		fout = fopen(filename2.c_str(),"a");
		fprintf(fout,"%d %d %s\n",client_fd_to_id[fd],1,(filename + "_" + std::to_string(file_id)+ "_" + hash).c_str());
		fclose(fout);
		response_client(fd,200,"OK: Upload successfully.",{});	
	}
}
void download_file(int fd,int len){
	json j = recv_client_data(fd,len);
	if(client_status[fd] == 'U') return;
	
	int target = j["target"].get<int>();
	
	std::string filename = j["filename"].get<std::string>();
	std::string filepath = "./file_save/" + filename;
	if(client_status[fd] != 'L'){
		response_client(fd,403,"Forbidden: Not loggined.",{});	
	}else{
		FILE *fin = fopen(filepath.c_str(),"r");
		if(fin){
			char *large_buffer = (char*) malloc(1<<20);
			retval = fscanf(fin,"%s",large_buffer);
			puts(large_buffer);
			fclose(fin);
			response_client(fd,200,"OK: Download successfully.",std::string(large_buffer));	
			free(large_buffer);	
		}else{
			response_client(fd,404,"File Not Found.",{});	

		}
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
		if(all_users[client_fd_to_id[fd]]->black_list.find(target) != all_users[client_fd_to_id[fd]]->black_list.end()){
			response_client(fd,403,"Forbidden: This man is in black list, please forgive him/her :).",{});
			return;
		}
		std::string filename = record_init_and_return_filename(client_fd_to_id[fd],target);
		FILE *fin = fopen(filename.c_str(),"r");
		int id1,id2,id1_read,id2_read;
		retval = fscanf(fin,"%d %d %d %d\n",&id1,&id2,&id1_read,&id2_read);
		int counter = 0, who, type;
		char msg_buf[MAX_DATALEN+1];
		std::vector<json> raw_history;
		while(fscanf(fin,"%d %d %s\n",&who,&type,msg_buf)!=EOF){
			json j;
			j["message"] = msg_buf;
			j["type"] = type == 0 ? "message" : "data";
			j["who"] = who;
			raw_history.push_back(j);
			counter += 1;
		}
		std::map<int,json> history;
		if(start_from < 0)
			start_from = counter + start_from;
		if(end_to < 0)
			end_to = counter + end_to;
		if(end_to >= counter)
			end_to = counter-1;
		if(start_from < 0)
			start_from = 0;
		for(;start_from<=end_to && start_from>=0 ;start_from++){
			history[start_from] = raw_history[start_from];
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
	for(int i=1;i<=20;i++){
		if(i != SIGINT){
			signal(i, signalHandler); 
		}
	}
	int jmpVal = setjmp(jmpbuffer);
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
					try{
						unsigned long long len = *((unsigned long long *)(uni_pkt+1));
						switch(uni_pkt[0]){
							case 'l':	login(fd,len);				break;
							case 's':	sign_up(fd,len);			break;
							case 'r':	refresh(fd,len);			break;
							case 'm':	messaging(fd,len);			break;
							case 'x':	edit_list(fd,len);			break;
							case 'u':	upload_file(fd,len);		break;
							case 'd':	download_file(fd,len);		break;
							case 'a':	send_talking_list(fd,len);	break;
							case 't':	id_to_username(fd,len);		break;
						}
						printf("\t\t\t[system] fd(%d) send strange packet\n",fd);
						printf("len = %llu\n",len);
					} catch(json::parse_error){
						response_client(fd,406,"Not Acceptable: Are you're string is json parsable?",{});
						puts("wow parse error detected.");
					}catch(json::type_error){
						response_client(fd,406,"Not Acceptable: Are you're json file is meet our protocol?",{});
					}
				}else{ // sz = 0 : client closed.
					finalize_client(fd);
				}
			}
		}
	}
	return 0;
}
