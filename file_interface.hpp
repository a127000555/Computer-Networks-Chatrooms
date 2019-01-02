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
struct user_info *user_refresh(int usr_id){
	// id, name, len(friend), len(black)

	char file_name[2048];
	sprintf(file_name,"./usr/%d.txt",usr_id); 
	FILE *fin = fopen( file_name, "r");

	int f_len,b_len,retval;
	char name[MAX_NAME_LEN+1];
	char passwd[MAX_NAME_LEN+1];
	retval = fscanf(fin,"%d %d %d",&usr_id,&f_len,&b_len);
	retval = fscanf(fin,"%s %s",name,passwd);
	fclose(fin);

	printf("load information: %s\n",name);
	return new struct user_info(usr_id,std::string(name),std::string(passwd));

}
void user_save(struct user_info* info){
	// id, name, len(friend), len(black)
	int retval;
	FILE *fout = fopen((std::string("./usr/")+ std::to_string(info->user_id) + ".txt").c_str() ,"w");
	retval = fprintf(fout,"%d %lu %lu\n",info->user_id,info->friend_list.size(),info->black_list.size());
	retval = fprintf(fout,"%s %s\n",info->user_name.c_str(),info->user_password.c_str());
	for(std::string s : info->friend_list)
		retval = fprintf(fout,"%s\n",s.c_str());
	for(std::string s : info->black_list)
		retval = fprintf(fout,"%s\n",s.c_str());
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