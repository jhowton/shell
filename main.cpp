#include "shell.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char * argv[], char ** envp) {
	Shell shell(envp);
	if(argc > 1){
		shell.loop(argv[1]);
	}else{
		shell.loop();
	}
	return 0;
}
