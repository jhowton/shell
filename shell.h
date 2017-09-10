#ifndef SHELL_H
#define SHELL_H

#include <map>
#include <string>
#include <vector>
#include <csignal>
#include <ctime>
#include <sys/time.h>
#include <sys/resource.h>
#include <list>

using namespace std;

struct child_info{
	int pid;
	int id;
	string status;
	string p_name;

	struct child_info & operator=(const struct child_info &rhs){
		if(this!= &rhs){
			pid = rhs.pid;
			id = rhs.id;
			status = rhs.status;
			p_name = rhs.p_name;
		}

		return *this;
	}
};

//Shell class
class Shell {
public:
	Shell(char ** env); //Constructor
	//Typedef for function pointer with string argument
	typedef void (Shell::*FctnPtr)(string arg);
	void loop();//Method to create command loop w/ stdin
	void loop(string);//Method to create command loop w/ cmd line arg file

private:
	bool keepLooping = true; //Test at each loop start
	string userID; //User ID
	const string whitespace = " \t\n";
	map<string, FctnPtr> commands; //Hash map of commands
	map<string, string> shellVars; //Hash map of shell variables
	vector<char *> arg_list;	//Arguments for exec call
	char ** env_ptr;	//Pointer to enviornment
	int pipe_fds[2];	//Std out pipe
	struct rlimit cpu_lim;	//Rlimit struct for CPU time
	struct rlimit mem_lim;	//Rlimit struct for Mem cap
	int curr_id;	//Current id to assign to child
	void stop_shell(string); //Exit shell - set keepLooping to false
	void setVar(string); //Set shell variable
	void unsetVar(string); //Unset shell variable
	void prtWord(string); //Print word or variable
	void setEnv(string); //Set environment variable
	void unsetEnv(string); //Unset environment variable
	void envPrt(string); //Print entire Environment
	void witch(string); //Which command
	void pwd(string);	//Print working directory
	void cd(string); //Change directory
	void limit(string); //Get and set rlimit
	void jobs(string);	//list jobs
	void fg(string);	//start job in foreground
	void bg(string);	//start job in background
	void kill_id(string);	//Kill specified job
	void shmalloc(string);	//Kill specified job
	void shmdel(string);	//Kill specified job
	void callCommand(string, string); //Call appropriate function given command
	void interpolate(string &);	//Interpolate variables in input string
};

#endif
