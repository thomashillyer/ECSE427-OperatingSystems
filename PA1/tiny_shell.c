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

static int child_function(void* arg) { //startup function for cloned child
	//printf is not thread safe
	char ** newargs = (char **) arg;
	// write(1, newargs[0], strlen(newargs[0])); //seems to also cause a seg fault, no longer causing seg fault?
	// sleep(2);
	// you are in the child

	//hijack process to exit on exit and change directory on cd
	//since these are not executable commands in the regular sense
	if (strcmp(*newargs, "exit") == 0) {
		_exit(0);
	}
	else if (strcmp(*newargs, "cd") == 0) {
		//allow soft fails through return statements instead of exit
		if (newargs[1] == NULL) {
			printf("%s\n", "ERROR: missing cd arg");
			return 1;
		} else {
			if (chdir(newargs[1]) != 0) {
				perror("ERROR: could not change directory");
				return 1;
			}
			//cd is the command, dont attempt to exec after
			return 0;
		}
	}
	//store the pid so it can be killed with ctrl c
	// recentPID = getpid();
	if (execvp(newargs[0], newargs) < 0) {
		perror("ERROR: failure in execvp");
		_exit(1);
	}
	//from handout
	// if (close(*((int *) arg)) == -1) {
	// 	errExit("close");
	// }
	// return 0; //child terminates now
}

int clone_function(void* line) {

	struct timespec start, finish; //start and end structs for timing
	char* args[512];
	//parse
	parseCommand(line, args);

	char *stack; //points to bottom
	char *stackTop; //points to top

	int flags = CLONE_VM | // child runs in same memory space as caller
	            CLONE_FS | //child shares filesystem info (for use with chdir)
	            CLONE_FILES | //child shares file descriptor table
	            CLONE_SIGHAND | //child shares signal handler table
	            CLONE_THREAD | //child is placed in the same thread group as caller (must be with clone_sighand and clone_vm)
	            CLONE_PARENT_SETTID | //store child thread id @ ptid in parent
	            CLONE_CHILD_CLEARTID | //store child thread id @ ctid in child
	            CLONE_SYSVSEM; //child shares sys V semaphore adjustment vals with caller

	pid_t pid; //for waitpid
	int status; //for waitpid


	stack = (char *) malloc(STACK_SIZE); //allocate memory for stack

	if (stack == NULL) {
		//failed to allocate
		fprintf(stderr,  "ERROR with malloc\n");
		exit(1);
	}
	stackTop = stack + STACK_SIZE; // assuming stack grows down, -1 to prevent overflow

	clock_gettime(CLOCK_REALTIME, &start); //record the start time
	// the clone command uses childfunc as the starting function, the child terminates when the childfunc terminates
	if ((pid = clone(child_function, stackTop, flags | SIGCHLD, args)) == -1) {
		fprintf(stderr,  "ERROR with clone\n");
		free(stack);
		exit(1);
	}
	// else {
	// do {
	waitpid(pid, &status, __WCLONE);

	clock_gettime(CLOCK_REALTIME, &finish); //record the end time
	long ns = finish.tv_nsec - start.tv_nsec;
	printf("nanoseconds: %ld\n", ns); // print the time it took
	// } while (!WIFEXITED(status));
	// waitpid(pid, &status, 0);
	return 0;
	// }
	// return 1;
}

/*my_system is a functions that create a child process and
runs the command you passed as the argument*/
int my_systemF(char* line) {
	struct timespec start, finish; //start and end structs for timing

	pid_t pid; 	//this is the child's PID in the parent process (or -1 if fail and no child created)
	//this is 0 in the child process
	int status;

	char* args[512];
	//parse
	parseCommand(line, args);

	//hijack process to exit on exit and change directory on cd
	//since these are not executable commands in the regular sense
	if (strcmp(*args, "exit") == 0) {
		exit(0); //not actually in the child so use the regular exit
	}

	clock_gettime(CLOCK_REALTIME, &start); //record the start time
	if ((pid = fork()) == -1) { //maybe less than 0
		perror("ERROR: Failed to fork child");
		exit(1);
	} else if (pid == 0) {
		// you are in the child

		//store the pid so it can be killed with ctrl c
		recentPID = getpid();
		if (execvp(args[0], args) < 0) {
			perror("ERROR: failure in execvp");
			_exit(1);
		}

	} else {
		// you're in parent
		// wait for child

		//if >0 wait for child pid, if 0 wait for any child of the caller (parent)
		waitpid(pid, &status, 0);

		clock_gettime(CLOCK_REALTIME, &finish); //record the end time
		long ns = finish.tv_nsec - start.tv_nsec;
		printf("nanoseconds: %ld\n", ns); // print the time it took
	}


	return 1;
}

/*my_system is a functions that create a child process and
runs the command you passed as the argument*/
int my_systemV(char* line) {
	pid_t pid; 	//this is the child's PID in the parent process (or -1 if fail and no child created)
	//this is 0 in the child process
	int status;
	struct timespec start, finish; //start and end structs for timing
	char* envp[] = {"key=value", '\0'}; //blank test variables

	char* args[512];
	//parse
	parseCommand(line, args);
	//hijack process to exit on exit and change directory on cd
	//since these are not executable commands in the regular sense
	if (strcmp(*args, "exit") == 0) {
		exit(0); //not actually in the child process so use regular exit
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
			_exit(1);
		}
	} else {
		// you're in parent
		// you dont need to wait for child
		// if there is no wait here then the parent returns and will print the prompt before the command has printed
		waitpid(pid, &status, 0);

		clock_gettime(CLOCK_REALTIME, &finish);
		long ns = finish.tv_nsec - start.tv_nsec;
		printf("nanoseconds: %ld\n", ns);
		return 0;
	}
	return 1;
}

int pipe_function(char* line) {
	int fd1[2]; //this is the two ends of the pipe, fd[0] reading, fd[1] writing
	int fd2[2];
	pid_t pid; 	//this is the child's PID in the parent process (or -1 if fail and no child created)
	//this is 0 in the child process
	int status;

	char* args[512];
	//parse
	parseCommand(line, args);

	//hijack process to exit on exit and change directory on cd
	//since these are not executable commands in the regular sense
	if (strcmp(*args, "exit") == 0) {
		exit(0); //not actually in the child so use the regular exit
	}

	//create a pipe
	if (pipe(fd1) == -1) {
		perror("ERROR: pipe1 failed");
		exit(1);
	}
	// if(pipe(fd2)==-1){
	// 	perror("ERROR: pipe2 failed");
	// 	exit(1);
	// }

	if ((pid = fork()) == -1) { //maybe less than 0
		perror("ERROR: Failed to fork child");
		exit(1);
	} else if (pid == 0) {
		// you are in the child
		close(1); //close stdout in child
		dup(fd1[1]); //duplicate fd1[1] in stdout's default spot

		// close(0); //close stdin in child
		// dup(fd2[0]); //duplicate fd2[0] (reading end) in stdin's default spot
		// close(fd1[0]); //close reading end in child

		//store the pid so it can be killed with ctrl c
		recentPID = getpid();
		if (execvp(args[0], args) < 0) {
			perror("ERROR: failure in execvp");
			_exit(1);
		}
		// old way before i parsed arguments
		// implementation used in system()
		// pretty sure this makes another shell inside the child
		// if(execl("/bin/sh", "sh", "-c", &line, (char*) 0)<0){
		// 	printf("%s\n", "ERROR: failed to execute command");
		// 	exit(1);
		// }

	} else {
		close(0); //close stdin
		dup(fd1[0]); //duplicate fd1[0] (reading end) in stdins default spot

		// close(1); //close stdout
		// dup(fd2[1]); //duplicate fd2[1] (writing end) in stdouts default spot
		// close(fd1[1]); //close writing end in parent
		// you're in parent
		// wait for child

		//if >0 wait for child pid, if 0 wait for any child of the caller (parent)
		waitpid(pid, &status, 0);
	}


	return 1;
}


int my_system(char* line) {
	//signal handler to hijack ctrl+c
	signal(SIGINT, intHandler);
#ifdef FORK
	my_systemF(line);
#elif VFORK
	my_systemV(line);
#elif CLONE
	clone_function(line);
#elif PIPE
	// should probably break out a separate function to
	// create the pipe and call the existing fork() implementation
	pipe_function(line);
#else
	struct timespec start, finish; //start and end structs for timing
	clock_gettime(CLOCK_REALTIME, &start); //start
	system(line);
	clock_gettime(CLOCK_REALTIME, &finish); //stop
	long ns = finish.tv_nsec - start.tv_nsec;
	printf("nanoseconds: %ld\n", ns);
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

