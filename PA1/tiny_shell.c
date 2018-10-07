#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> //for fork
#include <unistd.h> //for fork and vfork
#include <sys/wait.h> //for waitpid
#include <time.h> //for timing

#define STACK_SIZE (1024 * 1024) //random size 

int recentPID = 0;

void intHandler(int SIG) {
	if (recentPID == 0) {
		//ignore this call and continue normally
		exit(0);
	} else {
		//kill the top process
		kill(recentPID, SIGKILL);
		recentPID = 0;
	}
}

void parseCommand(char* line, char** args) {
	char* deliminators = " \n\t";
	char* token;
	int tokenIndex = 0;
	//get the first token from the string
	token = strtok(line, deliminators);
	//while not at the end of the line
	while (token != NULL) {
		//store the token in the array
		args[tokenIndex++] = token;
		//man page says to pass NULL when the same string should be compared
		token = strtok(NULL, deliminators);
	}
	//make sure to null terminate
	args[tokenIndex] = '\0';
	//dont need to return args because it was passed by reference
}

static int childFunc(void* arg) { //startup function for cloned child
	printf("child process arg = %s\n", (char *)arg);
	// you are in the child
	//store the pid so it can be killed with ctrl c
	recentPID = getpid();
	if (execvp(args[0], args) < 0) {
		perror("ERROR: failure in execvp");
	}
	//from handout
	// if (close(*((int *) arg)) == -1) {
	// 	errExit("close");
	// }
	return 0; //child terminates now
}

int clone_function(void* arg) {
	char *stack; //points to bottom
	char *stackTop; //points to top

	int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM | CLONE_DETACHED;

	// printf("%d\n", flags);

	char *str = "Hello World\n";

	pid_t pid; //for waitpid
	int status; //for waitpid


	stack = (char *) malloc(STACK_SIZE);

	if (stack == NULL) {
		fprintf(stderr,  "ERROR with malloc\n");
		exit(1);
	}
	stackTop = stack + STACK_SIZE - 1; // assuming stack grows down, -1 to prevent overflow
	printf("parent pid = %d\n", getpid());
	// the clone command uses childfunc as the starting function, the child terminates when the childfunc terminates
	//if ((pid = clone(childFunc, stackTop, flags | SIGCHLD, (void *) &fd)) == -1) {
	if ((pid = clone(childFunc, stackTop, flags , str)) == -1) {
		fprintf(stderr,  "ERROR with clone\n");
		free(stack);
		exit(1);
	}

	printf("child pid = %d\n", pid);
	waitpid(pid, &status, 0);
	printf("done\n");
	sleep(1);
	return 0;
}

/*my_system is a functions that create a child process and
runs the command you passed as the argument*/
int my_systemF(char** args) {
	pid_t pid; 	//this is the child's PID in the parent process (or -1 if fail and no child created)
	//this is 0 in the child process
	int status;

	char* args[512];
	//parse
	parseCommand(line, args);
	//hijack process to exit on exit and change directory on cd
	//since these are not executable commands in the regular sense
	if (strcmp(*args, "exit") == 0) {
		exit(0);
	}
	// TODO: this is the wrong way to do this (works but doesnt have the expected behaviour with clone)
	else if (strcmp(*args, "cd") == 0) {
		//allow soft fails through return statements instead of exit
		if (args[1] == NULL) {
			printf("%s\n", "ERROR: missing cd arg");
			return 1;
		} else {
			if (chdir(args[1]) != 0) {
				perror("ERROR: could not change directory");
				return 1;
			}
			//cd is the command, dont attempt to exec after
			return 0;
		}
	}

	//printf("Fork line: %s\n", *args);

	if ((pid = fork()) == -1) { //maybe less than 0
		perror("ERROR: Failed to fork child");
		exit(1);
	} else if (pid == 0) {
		// you are in the child
		//store the pid so it can be killed with ctrl c
		recentPID = getpid();
		if (execvp(args[0], args) < 0) {
			perror("ERROR: failure in execvp");
		}

		// old way before i parsed arguments
		// implementation used in system()
		// pretty sure this makes another shell inside the child
		// if(execl("/bin/sh", "sh", "-c", &line, (char*) 0)<0){
		// 	printf("%s\n", "ERROR: failed to execute command");
		// 	exit(1);
		// }

	} else {
		// you're in parent
		// wait for child

		//if >0 wait for child pid, if 0 wait for any child of the caller (parent)
		waitpid(pid, &status, 0);
	}


	return 0;
}

/*my_system is a functions that create a child process and
runs the command you passed as the argument*/
int my_systemV(char** args) {
	pid_t pid; 	//this is the child's PID in the parent process (or -1 if fail and no child created)
	//this is 0 in the child process
	int status;
	struct timespec start, finish;
	char* envp[] = {"key=value", '\0'};

	char* args[512];
	//parse
	parseCommand(line, args);
	//hijack process to exit on exit and change directory on cd
	//since these are not executable commands in the regular sense
	if (strcmp(*args, "exit") == 0) {
		exit(0);
	}
	// TODO: this is the wrong way to do this (works but doesnt have the expected behaviour with clone)
	else if (strcmp(*args, "cd") == 0) {
		//allow soft fails through return statements instead of exit
		if (args[1] == NULL) {
			printf("%s\n", "ERROR: missing cd arg");
			return 1;
		} else {
			if (chdir(args[1]) != 0) {
				perror("ERROR: could not change directory");
				return 1;
			}
			//cd is the command, dont attempt to exec after
			return 0;
		}
	}

	//prepend "/bin/" so that commands can be found
	char* base;
	char* newCmd;
	base = "/bin/\0";
	int newLen = strlen(base) + strlen(*args) + 1;
	newCmd = malloc (newLen * sizeof(char));
	// printf("%s\n", base);
	// printf("%s\n", *args);
	strcat(newCmd, base);
	strcat(newCmd, *args);
	// printf("%s\n", newCmd);
	*args = newCmd;


	clock_gettime(CLOCK_REALTIME, &start);
	if ((pid = vfork()) == -1) {
		perror("ERROR: Failed to fork child");
		exit(1);
	} else if (pid == 0) {
		// you are in the child
		//store the pid so it can be killed with ctrl c
		recentPID = getpid();
		if (execve(args[0], args, envp) < 0) {
			perror("ERROR: failure in execve");
		}
	} else {
		// you're in parent
		// you dont need to wait for child
		// if there is no wait here then the parent returns and will print the prompt before the command has printed
		waitpid(pid, &status, 0);

		clock_gettime(CLOCK_REALTIME, &finish);
		long ns = finish.tv_nsec - start.tv_nsec;
		if (start.tv_nsec > finish.tv_nsec)
		{	// clock underflow
			ns += 1000000000;
		}
		printf("nanoseconds: %ld\n", ns);
		return 0;
	}
	return 1;
}

int my_system(char* line) {
	//signal handler to hijack ctrl+c
	signal(SIGINT, intHandler);
	// char* args[512];
	// //parse
	// parseCommand(line, args);
	// //hijack process to exit on exit and change directory on cd
	// //since these are not executable commands in the regular sense
	// if (strcmp(*args, "exit") == 0) {
	// 	exit(0);
	// }
	// // TODO: this is the wrong way to do this (works but doesnt have the expected behaviour with clone)
	// else if (strcmp(*args, "cd") == 0) {
	// 	//allow soft fails through return statements instead of exit
	// 	if (args[1] == NULL) {
	// 		printf("%s\n", "ERROR: missing cd arg");
	// 		return 1;
	// 	} else {
	// 		if (chdir(args[1]) != 0) {
	// 			perror("ERROR: could not change directory");
	// 			return 1;
	// 		}
	// 		//cd is the command, dont attempt to exec after
	// 		return 0;
	// 	}
	// }
#ifdef FORK
	my_systemF(args);
#elif VFORK
	my_systemV(args);
#elif CLONE
	clone_function(args); //something
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

	while (1) {
		printf("%s", "tshell$ ");
		//line = get_a_line();
		//In the event of an error, errno is set to indicate the cause.
		if (getline(&line, &n, stdin) == -1) {
			// printf("%s\n", "No line");
			//exit or print error
			perror("ERROR: ");// not sure what to do here
			exit(1); //should probably be exit(1)
		}
		if (strlen(line) > sizeof(char)) {
			my_system(line);
		}
		//else {
		//	exit(0);
		//}
	}
}

