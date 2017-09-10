#include "shell.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <queue>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace std;

queue<struct child_info> fin_children;
list<struct child_info> child_list; //List of current children
struct child_info * fg_child;

void change_status(int pid, const string stat){
	list<child_info>::iterator iter = child_list.begin();
	bool found = false;
	while(!found && iter != child_list.end()){
		if(iter->pid == pid){
			found = true;
			iter->status = stat;
			break;
		}
	}
}

void handler(int s){
	pid_t pid;
	int status;
	bool found;
	list<struct child_info>::iterator iter;
	int count = 0;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0 && count < 10){
		if(fg_child == nullptr || fg_child->pid == pid){
			found = false;
			iter = child_list.begin();
			while(!found && iter != child_list.end()){
				if(iter->pid == pid){
					found=true;
					fin_children.push(*iter);
					child_list.erase(iter);
				}else{
					iter++;
				}
			}
		}else{
			fg_child = nullptr;
		}
		count++;
	}
	fflush(stdout);
}

void bg_handler(int s){

	printf("\n");
	int rc = -1;
	rc = kill(fg_child->pid, 20);
	change_status(fg_child->pid, "Suspended");
	if(rc == -1)
		printf("ERROR\n");

	fg_child = nullptr;
}

extern char** environ;

Shell::Shell(char ** env)
	:
	//userID(getpwuid(getuid())->pw_name)
	userID("jh6w"),
	env_ptr(env),
	curr_id(0)
{
	commands.emplace("exit", &Shell::stop_shell);
	commands.emplace("set", &Shell::setVar);
	commands.emplace("unset", &Shell::unsetVar);
	commands.emplace("prt", &Shell::prtWord);
	commands.emplace("envset", &Shell::setEnv);
	commands.emplace("envunset", &Shell::unsetEnv);
	commands.emplace("envprt", &Shell::envPrt);
	commands.emplace("witch", &Shell::witch);
	commands.emplace("pwd", &Shell::pwd);
	commands.emplace("cd", &Shell::cd);
	commands.emplace("lim", &Shell::limit);
	commands.emplace("jobs", &Shell::jobs);
	commands.emplace("fg", &Shell::fg);
	commands.emplace("bg", &Shell::bg);
	commands.emplace("kill", &Shell::kill_id);
	commands.emplace("shmalloc", &Shell::shmalloc);
	commands.emplace("shmdel", &Shell::shmdel);
	clearenv();
	setenv("AOSPATH", "/bin:/usr/bin", 0);
	setenv("AOSCWD", get_current_dir_name(), 0);
	getrlimit(RLIMIT_CPU, &cpu_lim);
	getrlimit(RLIMIT_AS, &mem_lim);
}

void Shell::loop()
{
	int rc1, rc2, pipe_pos, s;
	char buf[100];
	string line, cmd;
	size_t pos;
	istringstream ss;
	bool sync;
	struct child_info temp;

	signal(SIGCHLD, handler);
	signal(20, bg_handler);

	while(keepLooping) {
		while(fin_children.empty() == false){
			temp = fin_children.front();	
			printf("Finished executing: pid: %d  id: %d  prog: %s\n",
				temp.pid, temp.id, temp.p_name.c_str());
			fin_children.pop();
		}
		sync = true;
		if (isatty(STDIN_FILENO)){
			printf("%s_sh> ", userID.c_str());
		}
		if(getline(cin, line).eof()){
			if(isatty(STDIN_FILENO))
				cout << endl;
			break;
		}
		//Get rid of comments
	 	pos = line.find("#");
		line = line.substr(0,pos);
		if (!line.empty()){
			interpolate(line);
			pipe_pos = line.find('|');
			if(pipe_pos != string::npos){
				pipe(pipe_fds);				
				if(line.find_last_not_of(whitespace) == '&'){
					sync = false;
					line = line.substr(0, line.find_last_not_of(whitespace));
				}

				rc1 = fork();

				if(rc1 == -1){
					printf("fork failed\n");
					exit(-1);
				}
				if(rc1 == 0){	//child 1
					close(pipe_fds[0]);

					close(1);
					dup(pipe_fds[1]);
					close(pipe_fds[1]);

					line = line.substr(line.find_first_not_of(whitespace), pipe_pos);

					cmd = line.substr(0, line.find(' ', 0));
					callCommand(cmd, line.substr(cmd.length()));

					close(1);	
					exit(0);

				}

				rc2 = fork();

				if(rc2 == -1){
					printf("fork failed\n");
					exit(-1);
				}

				if(rc2 == 0){	//child 2
					close(pipe_fds[1]);

					close(0);
					dup(pipe_fds[0]);
					close(pipe_fds[0]);

					line = line.substr(pipe_pos+1);
					line = line.substr(line.find_first_not_of(whitespace));

					cmd = line.substr(0, line.find(' ', 0));
					callCommand(cmd, line.substr(cmd.length()));
					
					exit(0);

				}
				
				
				close(pipe_fds[0]);
				close(pipe_fds[1]);

				if(sync){
					waitpid(rc2, &s, 0); 
				}

			}else{

				line = line.substr(line.find_first_not_of(whitespace));
				cmd = line.substr(0, line.find(' ',0));
				callCommand(cmd, line.substr(cmd.length()));				
			}	
		}
	}
	
}

void Shell::loop(string filename)
{
	ifstream myIn;
	string line, cmd;
	size_t pos;
	istringstream ss;

	myIn.open(filename);

	while(getline(myIn, line) && keepLooping) {

		//Get rid of comments
	 	pos = line.find("#");
		line = line.substr(0,pos);
		if (!line.empty()){
			interpolate(line);
			cmd = line.substr(0, line.find(' ',0));
			callCommand(cmd, line.substr(cmd.length()));				
		}
	}
	
	myIn.close();
}

void Shell::stop_shell(string arg)
{
	keepLooping = false;
}

void Shell::setVar(string arg)
{
	string varName, varVal;
	istringstream ss(arg);
	ss >> varName >> varVal;
	shellVars[varName] = varVal;
	
}

void Shell::unsetVar(string arg)
{
	string varName;
	istringstream ss(arg);
	ss >> varName;
	auto iter = shellVars.find(varName);
	if(iter != shellVars.end())
		shellVars.erase(iter);
	else
		printf("%s does not exist\n", varName.c_str());
}

void Shell::prtWord(string arg)
{
	string wordOrVar;
	istringstream ss(arg);
	while( ss >> wordOrVar ) {
			printf("%s ", wordOrVar.c_str());
	}
	printf("\n");
}

void Shell::setEnv(string arg)
{
	string varName, varVal;
	istringstream ss(arg);
	ss >> varName >> varVal;
	setenv(varName.c_str(), varVal.c_str(), 1);
}

void Shell::unsetEnv(string arg)
{
	string varName;
	istringstream ss(arg);
	ss >> varName;
	unsetenv(varName.c_str());
}

void Shell::envPrt(string arg)
{
	for (int i = 0; environ[i] != NULL; i++){
		char * value = environ[i];
		printf("%s\n", value);
	}
}

void Shell::witch(string arg)
{
	istringstream ss(arg);
	DIR *directory;
	string aospath = string(getenv("AOSPATH"));
	int start = 0,
	    pos = aospath.find(":");

	string dirName,
		cmdName;
	    
	bool found = false,
	     allDirs = false;

	ss >> cmdName;

	if(commands.find(cmdName) != commands.end()){
		cout << "Built-in command" << endl;
	}else{
		while (!found && !allDirs){
			
			dirName = aospath.substr(start, pos);
			directory = opendir(dirName.c_str());
			if(directory){
				struct dirent *entry;
				while((entry = readdir(directory)) != NULL){
					if(entry->d_name == cmdName){
						cout << dirName << "/" << entry->d_name << endl;
						found = true;
					}
				}
			}
			if (pos != string::npos){
				start = start + pos + 1;
			    	pos = (aospath.substr(start).find(":"));
			}else{
				allDirs = true;
			}
		}	
	}
}
	
void Shell::pwd(string arg)
{
	printf("%s\n", getenv("AOSCWD"));
}

void Shell::cd(string arg)
{
	istringstream ss(arg);
	arg = arg.substr(arg.find_first_not_of(whitespace));
	string path;
	if(arg.find_first_not_of(whitespace) != string::npos){
		ss >> path;
		if(chdir(path.c_str()) == 0)
			setenv("AOSCWD", get_current_dir_name(), 1);
		else
			printf("%s doesn't exist\n", path.c_str());
	}
}

void Shell::limit(string arg){
	int mem_MB;
	if(arg.find_first_not_of(whitespace) == string::npos){
		//Print limit
		if(mem_lim.rlim_max != -1){
			mem_MB = mem_lim.rlim_max/(1024*1024);
		}else{
			mem_MB = -1;
		}
		printf("Limits\nCPU: %d seconds\nMem: %d MB\n",
			cpu_lim.rlim_max, mem_MB);
	}else{
		//Store Limit
		istringstream ss(arg);
		ss >> cpu_lim.rlim_max >> mem_MB;
		cpu_lim.rlim_cur = cpu_lim.rlim_max;
		mem_lim.rlim_max = mem_lim.rlim_cur = mem_MB * 1024 * 1024;
	}
}

void Shell::callCommand(string cmd, string args)
{
	int rc, s;
	string single_arg;
	bool sync = true;
	vector<char *> arg_list;
	stringstream ss;

	if(cmd.find('/') != string::npos){
		if(args[args.find_last_not_of(whitespace)] == '&'){
			sync = false;
			args = args.substr(0, args.find_last_not_of(whitespace));
		}

		rc = fork();
		if(rc == -1){
			printf("fork failed\n");
			exit(-1);
		}

		if(rc == 0){ //child
			setrlimit(RLIMIT_CPU, &cpu_lim);
			setrlimit(RLIMIT_AS, &mem_lim);
			signal(20, SIG_DFL);
			setpgid(0,0);

			ss << cmd;
			ss << " ";
			ss << args;
			while(ss >> single_arg) 
			{
				char *arg = new char[single_arg.size() + 1];
				copy(single_arg.begin(), single_arg.end(), arg);
				arg[single_arg.size()] = '\0';
				arg_list.push_back(arg);
			}
			arg_list.push_back(0);

			execve(arg_list[0], &arg_list[0], environ);

			for(size_t i = 0; i < arg_list.size(); i++)
				delete[] arg_list[i];

			exit(0);	
		}
		if(sync){
			struct child_info temp;
			temp.pid = rc;
			temp.id = curr_id;
			temp.p_name = cmd;
			temp.status = "Running";
			child_list.push_front(temp);
			curr_id = (curr_id + 1) % 100000;

			fg_child = &temp;

			waitpid(rc,&s,WUNTRACED);
		}else{
			struct child_info temp;
			temp.pid = rc;
			temp.id = curr_id;
			temp.p_name = cmd;
			temp.status = "Running";
			child_list.push_front(temp);
			curr_id = (curr_id + 1) % 100000;

			sync = true;
		}
	}else{
		int start = 0, pos;
		auto iter = this->commands.find(cmd);
		if (iter != commands.end())
			(this->*(this->commands)[cmd])(args);
		else{
			bool found = false, allDirs = false;
			string aospath = string(getenv("AOSPATH"));
			pos = (aospath.substr(start).find(":"));
			while(!found && !allDirs){
				string dirName = aospath.substr(start, pos);
				DIR *directory = opendir(dirName.c_str());
				if(directory){
					struct dirent *entry;
					while((entry = readdir(directory)) != NULL && !found){
						if(entry->d_name == cmd){
							callCommand(dirName + '/' + string(entry->d_name), args);
							found = true;
						}
					}
				}
				if (pos != string::npos){
					start = start + pos + 1;
					pos = (aospath.substr(start).find(":"));
				}else{
					allDirs = true;
				}
			}
			if(!found){
				printf("\'%s\' not available in current path\n", cmd.c_str());
			}
		}
	}
}

void Shell::interpolate(string & line){
	size_t pos = line.find("$");
	int start, len;
	bool found;
	string sub;
	while (pos != string::npos){
		found = false;
		start = pos + 1;
		len = line.length() - start;
		for(;!found && (len > 0); len--){
			sub = line.substr(start, len);
			if(shellVars.find(sub) != shellVars.end()){
				line.replace(pos, len + 1, shellVars[sub]);
				found = true;
			}else if(getenv(sub.c_str())){
				line.replace(pos, len + 1, getenv(sub.c_str()));
				found = true;
			}
		}

		if (found){
			pos = line.find("$");
		}else{
			cout << "Variable " << line.substr(pos) << " not found\n";
			break;
		}
	}
}

void Shell::jobs(string args){
	list<struct child_info>::iterator iter = child_list.begin();
	printf("Jobs:\n");
	if(iter == child_list.end()){
		printf("No background/suspended jobs\n");
	}
	while(iter != child_list.end()){
		printf("id: %d  pid: %d  status: \'%s\'  prog: %s\n", iter->id, iter->pid, iter->status.c_str(), iter->p_name.c_str());
		iter++;
	}
}
void Shell::fg(string args){
	int id;
	istringstream s(args);
	s >> id;
	
	list<child_info>::iterator iter = child_list.begin();
	bool found = false;
	while(!found && iter != child_list.end()){
		if(iter->id == id){
			found = true;
			kill(iter->pid, SIGCONT);
			fg_child = &(*iter);
			change_status(iter->pid, "Running");
			waitpid(iter->pid,NULL,WUNTRACED);
			break;
		}
		iter++;
	}

	if(!found)
		printf("Process %d not found\n", id);
}

void Shell::bg(string args){
	int id;
	istringstream s(args);
	s >> id;
	
	list<child_info>::iterator iter = child_list.begin();
	bool found = false;
	while(!found && iter != child_list.end()){
		if(iter->id == id){
			found = true;
			kill(iter->pid, SIGCONT);
			change_status(iter->pid, "Running");
			break;
		}
		iter++;
	}

	if(!found)
		printf("Process %d not found\n", id);

}

void Shell::kill_id(string args){
	int id;
	istringstream s(args);
	s >> id;
	
	list<child_info>::iterator iter = child_list.begin();
	bool found = false;
	while(!found && iter != child_list.end()){
		if(iter->id == id){
			found = true;
			kill(iter->pid, 9);
			break;
		}
		iter++;
	}

	if(!found)
		printf("Process %d not found\n", id);

}

void Shell::shmalloc(string args){
	istringstream s(args);
	string name;
	int size;
	void * shmptr;
	s >> name >> size;
	size *= (1024*1024);

	int shmfd = shm_open(name.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if(shmfd == -1){
		perror("shm_open failed: ");
		exit(-1);
	}

	int rc = ftruncate(shmfd, size);
	if(rc == -1){
		perror("ftruncate failed: ");
		exit(-1);
	}
}

void Shell::shmdel(string args){
	istringstream s(args);
	string name;
	s >> name;
	shm_unlink(name.c_str());
}
