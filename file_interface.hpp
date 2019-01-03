struct user_info{
	int user_id;
	std::string user_name;
	std::string user_password;
	std::set<int> friend_list;
	std::set<int> black_list;
	user_info(
			int _id,
			std::string _user,
			std::string _user_password){
		user_id = _id;
		user_name = _user;
		user_password = _user_password;
		friend_list.clear();
		black_list.clear();
	}
	user_info(
			int id,
			std::string user,
			std::string user_password,
			std::set<int> friend_list,
			std::set<int> black_list){
		this->user_id = id;
		this->user_name = user;
		this->user_password = user_password;
		this->friend_list.clear();
		this->black_list.clear();
		for(int x : friend_list)
			this->friend_list.insert(x);
		for(int x : black_list){
			this->black_list.insert(x);
		}

	}
};
struct user_info *user_refresh(int usr_id){
	// id, name, len(friend), len(black)

	char file_name[2048];
	sprintf(file_name,"./usr/%d.txt",usr_id); 
	FILE *fin = fopen( file_name, "r");

	int f_len,b_len,retval;
	char name[MAX_NAME_LEN+1];
	char passwd[MAX_NAME_LEN+1];
	std::set<int> friend_list;
	std::set<int> black_list;
	retval = fscanf(fin,"%d %d %d",&usr_id,&f_len,&b_len);
	retval = fscanf(fin,"%s %s",name,passwd);
	for(int i=0;i<f_len;i++){
		int id2;
		retval = fscanf(fin,"%d",&id2);
		friend_list.insert(id2);
	}
	for(int i=0;i<b_len;i++){
		int id2;
		retval = fscanf(fin,"%d",&id2);
		black_list.insert(id2);
	}
	fclose(fin);

	printf("load information: %s\n",name);
	return new struct user_info(
		usr_id,
		std::string(name),
		std::string(passwd),
		friend_list,
		black_list);

}
void user_save(struct user_info* info){
	// id, name, len(friend), len(black)
	int retval;
	FILE *fout = fopen((std::string("./usr/")+ std::to_string(info->user_id) + ".txt").c_str() ,"w");
	retval = fprintf(fout,"%d %lu %lu\n",info->user_id,info->friend_list.size(),info->black_list.size());
	retval = fprintf(fout,"%s %s\n",info->user_name.c_str(),info->user_password.c_str());
	for(int i : info->friend_list)
		retval = fprintf(fout,"%d\n",i);
	for(int i : info->black_list)
		retval = fprintf(fout,"%d\n",i);
	fclose(fout);
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