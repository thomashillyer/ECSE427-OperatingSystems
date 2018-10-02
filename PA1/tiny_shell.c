#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> //for fork
#include <unistd.h> //for fork and vfork
#include <sys/wait.h> //for waitpid

int clone_function(void* arg);

/*my_system is a functions that create a child process and 
runs the command you passed as the argument*/
int my_systemF(char* line) {
	pid_t pid; 	//this is the child's PID in the parent process (or -1 if fail and no child created)
				//this is 0 in the child process
	int status;

	printf("Fork line: %s\n", line);

	if((pid = fork()) == -1){ //maybe less than 0
		printf("%s\n", "Failed to create child");
		exit(1);
	} else if(pid == 0){
		// you are in the child
		//do the thing

		//implementation used in system()
		if(execl("/bin/sh", "sh", "-c", &line, (char*) 0)<0){
			printf("%s\n", "ERROR: failed to execute command");
			exit(1);
		}

	} else {
		// you're in parent
		// wait for child

		//if >0 wait for child pid, if 0 wait for any child of the caller (parent)
		waitpid(pid, &status, 0);
	}


	return 0;
}

int my_system(char* line){
	#ifdef FORK
		my_systemF(line);
	#elif VFORK
		my_systemV(line);
	#elif CLONE
		my_systemC(line); //something
	#elif PIPE
		//something
	#else
		printf("Regular System call:\n");
		system(line);
	#endif
}

int main() {
	//pointer to the line
	char* line = NULL;
	// size_t that will be updated to the buffer size
	size_t n;

	while(1) {
		printf("%s","tshell$ ");
		//line = get_a_line();
		//In the event of an error, errno is set to indicate the cause.
		if(getline(&line, &n, stdin) == -1) {
			// printf("%s\n", "No line");
			//exit or print error
			perror("ERROR: ");// not sure what to do here
			exit(0); //should probably be exit(1)
		}
		if (strlen(line) > sizeof(char)) {
			my_system(line);	
		} else {
			exit(0);
		}
	}
}

