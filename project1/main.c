/* 
	Amauri Lopez
	CSC345-02
	Project1
	main.c
	*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <sys/types.h>
#include <termios.h>
#define MAX_LINE 80 /* The Maximum Length Command */

static struct termios save_term_mode;
struct termios new_term_mode;

char** history[64];
char input[MAX_LINE];
char *ip;

int historyPosition = 0;
int historyRightBound = -1;
int historyLeftBound = 0;
int historyCount = 0;

int justHitArrow = 0;
int backspaceCount = 0;
int concurrent = 0;

void set_noncanonical(){
    tcgetattr(0, &save_term_mode);
    new_term_mode = save_term_mode;
    new_term_mode.c_lflag &=~(ECHO | ICANON);
    tcsetattr(0, TCSANOW, &new_term_mode);
}

void set_canonical(){
    tcsetattr(0, TCSANOW, &save_term_mode);
}

void printHistory(){

}

char* stringCopy(const char* input){
	const size_t stringSize = (strlen(input) + 1) * sizeof(char);
	char* buffer = malloc(stringSize);

	if(buffer == NULL) { return NULL;}

	return (char *)memcpy(buffer, input, stringSize);
}

char** readArguments(char** args){

	ip = &input[0];

	while(1){

		char keystroke = getchar();

		/* If Enter key */
		if(keystroke == '\n'){
			/* Add a null terminating character */
			*ip = '\0';

			/* Add current line to the history */
			ip = input;

			historyRightBound++;

			/*Reset the history position to always point 1 space outside the array */
			historyPosition = historyRightBound + 1;

			history[historyRightBound] = (char **)stringCopy(ip);
			historyCount++;
			printf("\n");

			justHitArrow = 0;

			backspaceCount = 0;

			break;

		} else if(keystroke == 127){ /* backspace */
			if(backspaceCount == 0){
				/* Do nothing if nothing was typed yet*/
			}
			else{
				/* Set the index to delete as the last character */
				int idxToDel = strlen(input) - 1;
				printf("\b \b");

				/*Delete the character */
				memmove(&input[idxToDel], &input[idxToDel + 1], strlen(input) - idxToDel);
				//input[strlen(input) - 1] = "\0";
				justHitArrow = 0;

				backspaceCount--;
			}
		} else if(keystroke == '\033'){
			keystroke = getchar(); /* [ */
			keystroke = getchar(); /* Up or down */
			if(keystroke == 'A'){
				if(historyPosition == historyLeftBound){ /*If nothing in history */
					/* Do Nothing */
				} 
				else{
					if(justHitArrow){
						/* Delete current history print */
						int i = 0;
						while(i < strlen((char *)history[historyPosition])){
							printf("\b \b");
							i++;
						}
					}
					/* Move over the history position */
					historyPosition--;

					/* Replace string with history */
					ip = input;
					ip = stringCopy((char *)history[historyPosition]);
					printf("%s", ip);
					justHitArrow = 1;

					/*Update backspaces available */
					backspaceCount = strlen(input);

					/* Update input buffer */
					char* tempptr = input;
					strcpy(tempptr, ip);
				}
				
			}else if(keystroke == 'B'){
				/* If Down arrow */
				if(historyPosition == historyRightBound + 1){
					/* Do Nothing */
				}else {
					if(justHitArrow){
						/* Delete current history print */
						int i = 0;
						while(i < strlen((char *)history[historyPosition])){
							printf("\b \b");
							i++;
						}
					}

					historyPosition++;

					/* Replace input buffer with history */
					ip = input;

					if(!(historyPosition == historyRightBound + 1)){
						ip = stringCopy((char *)history[historyPosition]);
						printf("%s", ip);
						justHitArrow = 1;

						/*Update backspaces available */
						backspaceCount = strlen(input);

						/* Update input buffer */
						char* tempptr = input;
						strcpy(tempptr, ip);
					}
				}
			}
		} else{
			/* If any other character, copy to char array */
			*ip = keystroke;
			printf("%c", *ip);
			ip++;
			justHitArrow = 0;

			/*Update backspaces available */
			backspaceCount++;
		}
	}

	/* Split Input string by delimeter ' ' */
	char* split = strtok(input, " \t\r\n\a");
	int n_spaces = 0, i;

	/* split string and append tokens to args */
	while(split) {

		/*realloc to increase size of array containing the elements */
	    args = realloc(args, sizeof (char*) * ++n_spaces);

	    /* memory allocation failed */
	    if (args == NULL)
	        exit (-1); 

	    args[n_spaces-1] = stringCopy(split);

	    split = strtok(NULL, " \t\r\n\a");
	}

	/* realloc one extra element for the last NULL */
	args = realloc (args, sizeof (char*) * (n_spaces+1));
	args[n_spaces] = 0;

	/* Clear input buffer */
	for(i = 0; i<strlen(input); i++){
		input[i] = 0;
	}

	return args;
}

void runCommand(char **args){
	char* cd = "cd";
	char* historycmd = "history";
	char* execlastline = "!!";
	char* charptr;
	char* batch;

	int i, n;

	/* Fork to create child process */
	pid_t pid;
	pid = fork();

	charptr = args[0];

	/* Check if & at the end of the first arg */
	if((*(charptr + strlen(args[0])) - 1) == '&'){
		concurrent = 1;
	}else{
		concurrent = 0;
	}

	if(strcmp(args[0], historycmd) == 0){
		if(historyRightBound == 0){
			printf("No History");
		}
		if(historyCount <= 10){
			for(i = 0; i<historyCount; i++){
				printf("%d %s\n", historyCount - i, (char *)history[i]);
			}	
		}else{
			for(i = historyCount - 11; i<historyCount - 1; i++){
				printf("%d %s\n", (historyCount - 1) - i, (char *)history[i]);
			}	
		}
	}

	/* If its a batch file to run, run it */
	if(*charptr == '.' && *(charptr + 1) == '/'){
		batch = charptr + 2;
		runBatch(batch);
		return;
	}

	/* Child Process */
	if(pid == 0){ 

		/* Check for cd' first */
		if(strcmp(args[0], cd) == 0){
			chdir(args[1]);
		}
		else if(strcmp(args[0], execlastline) == 0){
			if(historyRightBound == 0){
				printf("No History");
				return;
			}
			execvp((char *)history[historyCount - 1], args);
		}
		else if(*charptr == '!'){
			n = atoi(*(charptr + 1));

			execvp((char *)history[n - 1], args);
		}
		else{
			execvp(args[0], args);
		}	
	}

	/* Parent Process */
	else if(pid>0){
		if(concurrent == 0){
			wait(NULL);
		}
	}

}

void runBatch(const char* filename){
	char shellLine[MAX_LINE/2 + 1];
	char** argOut = NULL;
	char* ptrToToken;
	int execFlag = 1;

	FILE* file;
	file = fopen(filename, "r");

	/*Check if file exists */
	if(file == NULL){
		printf("File %s does not exist\n", filename);
		return;
	}

	/* Start to parse each line in file */
	while(fgets(shellLine, MAX_LINE/2 + 1, file) != NULL){
		size_t len = strlen(shellLine);

		/* Find new line character and make it a null terminating */
		if(shellLine[len - 1] == '\n'){
			shellLine[len - 1] == '\0';
		}else{shellLine[len] == '\0';}

		/* Split Input string by delimeter ' ' */
		ptrToToken = strtok(shellLine, " \t\r\n\a");
		int n_spaces = 0, i;

		/* split string and append tokens to argOut */
		while(ptrToToken) {

			/*realloc to increase size of array containing the elements */
		    argOut = realloc(argOut, sizeof (char*) * ++n_spaces);

		    /* memory allocation failed */
		    if (argOut == NULL)
		        exit (-1); 

		    if(*ptrToToken == '#'){
		    	execFlag = 0;
		    	break;
		    }

		    argOut[n_spaces-1] = stringCopy(ptrToToken);

		    ptrToToken = strtok(NULL, " \t\r\n\a");
		}

		/* realloc one extra element for the last NULL */
		argOut = realloc (argOut, sizeof (char*) * (n_spaces+1));
		argOut[n_spaces] = 0;

		for(i = 0; i<n_spaces + 1; i++){
			printf("argOut[%d] = %s\n", i, argOut[i]);
		}

		if(execFlag){
			runCommand(argOut);
		}

	}
	fclose(file);
}

int main(void){
	/*Command line arguments*/
	char** args = NULL; 

	/*Flag to determine when to exit program*/
	int should_run = 1; 

	while(should_run){
		printf("osh>");

		fflush(stdout);

		/* Change to non-canonical mode */
		set_noncanonical();
		args = readArguments(args);

		/* Change back to canonical before running command */
		set_canonical();

		/* If 'exit' then exit program, if not then run */
		if(strcmp(args[0], "exit") == 0){
			printf("\n");
			should_run = 0;
			//exit(0);
		} else{runCommand(args);}
	}
}