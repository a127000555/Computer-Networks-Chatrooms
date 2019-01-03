std::map<int, struct user_info*> all_users;
std::map<std::string, int> username_to_id;
char client_status[MAX_FD];	
// U : unconnected. C : connected. L : Login
int client_fd_to_id[MAX_FD]; 
int sockfd,retval;
fd_set readset , retset;
jmp_buf jmpbuffer;
