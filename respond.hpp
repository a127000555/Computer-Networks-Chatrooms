void response_client(int fd,int status_code, const char* msg, json data){

	json j_data={
		{"status_code", status_code},
		{"state",msg},
		{"data",data}
	};
	std::string content = j_data.dump();

	char response[10]="!";
	*(unsigned long long *)(response+1) = content.length();
	send(fd,response,9,0);
	send(fd,content.c_str(),content.length(),0);
}
void connect_new_client(){
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int new_client_fd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);
	client_status[new_client_fd] = 'I';
	printf("\t\t\t[system] fd(%d) set to I (new connection)\n",new_client_fd);
	FD_SET(new_client_fd,&readset);
}
void finalize_client(int fd){
	close(fd);
	FD_CLR(fd, &readset);
	client_status[fd] = 'U';
	printf("\t\t\t[system] fd(%d) set to U (unconnection)\n",fd);

}