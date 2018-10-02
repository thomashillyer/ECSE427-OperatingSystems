#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_a_line();
int length();


int main() {
	//pointer to the line
	char* line = NULL;
	// size_t that will be updated to the number of bytes read
	size_t n;

	while(1) {
		printf("%s","tshell$ ");
		//line = get_a_line();
		//In the event of an error, errno is set to indicate the cause.
		if(getline(&line, &n, stdin) == -1) {
			// printf("%s\n", "No line");
			//exit or print error
			perror("ERROR: ");// not sure what to do here
			exit(0);
		}
		if (strlen(line) > sizeof(char)) {
			system(line);	
		} else {
			exit(0);
		}
	}
}

// int length(&line) {
// 	return ;
// }

//can assume constant length of 512 for input lines 
// but shouldnt affect my code

//i dont need to implement, i can just use getline() from stdio.h
//this vaguely works
int get_a_line(char **lineptr, size_t *n, FILE *stream) {
	static char line[256];
	unsigned int len;
	char *ptr;

	fgets(line, 256, stream);

	printf("%s\n", line);

	//not mine
	ptr = strchr(line,'\n');   
   	if (ptr)
      *ptr = '\0';

	len = strlen(line);

	if ((len+1) < 256)
	{
	  ptr = realloc(*lineptr, 256);
	  if (ptr == NULL)
	     return(-1);
	  *lineptr = ptr;
	  *n = 256;
	}
	//not mine end

	strcpy(*lineptr, line);

	return len;
}
