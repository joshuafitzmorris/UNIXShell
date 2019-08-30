// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

char history[HISTORY_DEPTH][COMMAND_LENGTH] = {0};
int history_increment = 0;


/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
int read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ( (length < 0) && (errno !=EINTR) ){
      perror("Unable to read command. Terminating.\n");
      exit(-1);  /* terminate with error */
  }
	else if((length < 0) && (errno == EINTR)){
		// Ctrl -c pressed
		memset(buff,0,COMMAND_LENGTH);
		for (int i = 0; i < NUM_TOKENS - 1; i++){
			tokens[i] = NULL;
		}
		return -1;
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return 0;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
	return 0;
}




void addToHistory(char history[HISTORY_DEPTH][COMMAND_LENGTH], char command[], int history_increment){

	// First create the string we wish to pass into history array
	if (command[0] == '\0'){
		return;
	}
	char res[COMMAND_LENGTH];
	memset(res,0,COMMAND_LENGTH);
	sprintf(res,"%d",history_increment);
	strcat(res, "\t");
	strcat(res,command);
	int length = strlen(res);
	if (res[length] != '\0'){
		strcat(res,"\0");
	}

	// If history array in not full, add it into the available slot
	if (history_increment <= 10){
		
		strcat(history[history_increment-1],res);
		
	}
	// Else, history array is full and we need to shift the array to make room for the new command
	// and remove the previosuly last used command
	else
	{
		for (int i = 0; i < 9; i++)
		{
			strcpy(history[i], history[i+1]);
		}
		strcpy(history[9], res);
	}
	

}

void displayHistory(char history[HISTORY_DEPTH][COMMAND_LENGTH], int history_increment,bool cntrl){
	if((cntrl)){
		write(STDOUT_FILENO,"\n",strlen("\n"));
	}
	int increment = history_increment;
	if (increment > 10){
		increment = 10;
	}
	for (int i = 0; i < increment; i++){
		write(STDOUT_FILENO, history[i], strlen(history[i]));
		write(STDOUT_FILENO,"\n", strlen("\n"));
	}
}

void handle_SIGINT(){

	displayHistory(history,history_increment,true);

}

/* 
Pre-condition: User enters !! or !n where n is an integer

Function: retrieve's the command from history (if it exists) and copies it into the buffer to be read and tokened and then
		  ran as a command in the shell
		  If the number passed doesn't correspond to any history function, print error and do nothing
*/
void retrieveCommand(char history[HISTORY_DEPTH][COMMAND_LENGTH], char input_buffer[COMMAND_LENGTH],
					int command_number,int history_increment, bool *addtoHistory)
{
	int tab_increment = 0;
	memset(input_buffer,0,COMMAND_LENGTH);
	// ERROR check: Nonsensical input
	if((command_number > history_increment) ||(history_increment==0)){ // Command number cant be greater than # of commands
		write(STDOUT_FILENO,"SHELL: Unknown history commmand.", strlen("SHELL: Unknown history commmand."));
		write(STDOUT_FILENO, "\n", strlen("\n"));
		*addtoHistory = false;
		return;
	}

	// Command entered: "!!" => run last command in history
	if(command_number == history_increment){ 


		if((history_increment > 0) && (history_increment <11)){ // If >= 10 commands run latest history increment
			
			int length = strlen(history[history_increment-1])-1;

			if(history_increment == 10){
				tab_increment = 2;
			}
			else{
				tab_increment = 1;
			}
			

			// Get substring of history command
			int c = 0;
			while (c < length){
				input_buffer[c] = history[history_increment-1][tab_increment+c+1];
				c++;
			}
			input_buffer[c] = '\0';
			write(STDOUT_FILENO,input_buffer,strlen(input_buffer));
			write(STDOUT_FILENO,"\n", strlen("\n"));
			return;
		}


		else{ // If history has more than 10 commands, run last command in history array
			int length = strlen(history[9])-1;
			while(history[9][tab_increment] != '\t'){ //Tab position
				tab_increment++;
			}
			int c = 0;
			while (c < length){
				input_buffer[c] = history[9][tab_increment+c+1];
				c++;
			}
			input_buffer[c] = '\0';
			write(STDOUT_FILENO,input_buffer,strlen(input_buffer));
			write(STDOUT_FILENO,"\n", strlen("\n"));
			return;
		}
	}


	// Command entered: '!N', where N is an integer
	// 2 cases arise from this:
	else{

		int difference = (history_increment - command_number);

		// Case 1: Command_number is too small, so the command is not in the history array
		if (difference> 9){
			write(STDOUT_FILENO,"SHELL: Unknown history commmand.", strlen("SHELL: Unknown history commmand."));
			write(STDOUT_FILENO, "\n", strlen("\n"));
			*addtoHistory = false;
			return;
		}


		// Case 2: Command_number is large enough to be in the history array
		else{
			int increment = 0;
			if(history_increment>10){
				increment=10;
			}
			else{
				increment = history_increment;
			}
			int index = increment - difference - 1;

			int length = strlen(history[index])-1;
			while(history[index][tab_increment] != '\t'){ // Tab position
				tab_increment++;
			}
			int c = 0;
			while(c<length){
				input_buffer[c] = history[index][c + tab_increment + 1];
				c++;
			}
			input_buffer[c] = '\0';
			write(STDOUT_FILENO,input_buffer,strlen(input_buffer));
			write(STDOUT_FILENO,"\n", strlen("\n"));
			return;
		}
	}
	
	
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS] = {NULL};
	char working_directory[COMMAND_LENGTH];
	bool is_internal = true; // Does the command need to fork a child process?

	struct sigaction handler;
	handler.sa_handler= handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT,&handler,NULL);

	bool addtoHistory = true;

	while (true) { 

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		is_internal = true;
		addtoHistory = true;
		getcwd(working_directory, sizeof(working_directory));
		strcat(working_directory, "> ");
		write(STDOUT_FILENO, working_directory, strlen(working_directory));
		memset(input_buffer,0,COMMAND_LENGTH);

		for (int i = 0; i < NUM_TOKENS-1; i++){
			tokens[i] = NULL;
		}
		_Bool in_background = false;
		int sig = read_command(input_buffer, tokens, &in_background);
		if (sig == -1){
			continue;
		}
		if (input_buffer[0] == '\0'){
			continue;
		}
		else if (input_buffer[0] == ' '){
			continue;
		}
		

		if(input_buffer[0]== '!'){

			// Run last command
			if (input_buffer[1] == '!'){
				retrieveCommand(history,input_buffer,history_increment,history_increment,&addtoHistory);
				int token_count = tokenize_command(input_buffer,tokens);

					// Extract if running in background:
				if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
					in_background = true;
					tokens[token_count - 1] = 0;
				}
			}


			// Tried to pass a number or nonsense
			else{
				int i = 1;
				char num[COMMAND_LENGTH];
				while(input_buffer[i] != '\0'){
					i++;
				}
				memcpy(num,&input_buffer[1],i);
				num[i] = '\0';

				int command_number = atoi(num);


				// Either passed 0 or a non int, both of which result in an error
				if(command_number == 0){ 
					write(STDOUT_FILENO,"SHELL: Unknown history commmand.", strlen("SHELL: Unknown history commmand."));
					write(STDOUT_FILENO, "\n", strlen("\n"));
					addtoHistory = false;
					continue;					
				}
				else if (command_number > history_increment){
					write(STDOUT_FILENO,"SHELL: Unknown history commmand.", strlen("SHELL: Unknown history commmand."));
					write(STDOUT_FILENO, "\n", strlen("\n"));
					addtoHistory = false;
					continue;	
				}
				else{
					retrieveCommand(history,input_buffer,command_number,history_increment, &addtoHistory);
					int token_count = tokenize_command(input_buffer,tokens);

						// Extract if running in background:
					if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
						in_background = true;
						tokens[token_count - 1] = 0;
					}
				}
			}
		}

		// DEBUG: Dump out arguments:
		for (int i = 0; tokens[i] != NULL; i++) {

			if(strcmp(tokens[i],"type") == 0){
				write(STDOUT_FILENO,"Command not found\n",strlen("Command not found\n"));
				is_internal = false;
				history_increment++;
				addToHistory(history,"type",history_increment);

				break;
			}

			if(strcmp(tokens[i],"exit") == 0){
				exit(0);
			}
			if(strcmp(tokens[i],"pwd") == 0){
				write(STDOUT_FILENO, getcwd(working_directory, sizeof(working_directory)),
						strlen(getcwd(working_directory, sizeof(working_directory))));
				write(STDOUT_FILENO,"\n",strlen("\n"));
						is_internal = false;
						history_increment++;
						addToHistory(history,"pwd",history_increment);
						break;
			}
			if(strcmp(tokens[i],"cd") == 0){

				const char *directory = tokens[i+1];
				char full[COMMAND_LENGTH] = "cd ";
				int ret;
				ret = chdir(directory);
				is_internal = false;
				history_increment++;
				for(int j = 1; tokens[j]!=NULL; j++){
					strcat(full,tokens[j]);
					strcat(full," ");
				}
				addToHistory(history, full,history_increment);
				if(ret == -1){
					write(STDOUT_FILENO, "Invalid directory.\n", strlen("Invalid directory.\n"));
				}
				break;

			}
			if(strcmp(tokens[i],"history") == 0){
				history_increment++;
				is_internal = false;
				addToHistory(history,"history",history_increment);
				displayHistory(history, history_increment,false);
			}

		}

		if (is_internal == false){
			continue;
		}




		if (addtoHistory){
			history_increment++;
			int i = 0;
			int j = 0;
			char cmd[COMMAND_LENGTH] = "";
			for(i = 0; tokens[i] != NULL; i++){
				while((*tokens)[j] != '\0'){
					cmd[j] = (*tokens)[j];
					j++;
				}
				cmd[j] = ' ';
				j++;
			}
			if (in_background == true){
				cmd[j] = '&';
				j+=2;
			}
			cmd[j-1] = '\0';
			addToHistory(history,cmd,history_increment);

		}

        pid_t pid;

        pid = fork();

        if (pid < 0){ // If fork fails
            write(STDOUT_FILENO, "Fork failed\n", strlen("Fork failed\n"));
            exit(1);
			}


        if (pid == 0){ // If process is child process, execute command
            execvp(tokens[0], tokens); // child process should never get by this command if command is successful
            write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
			write(STDOUT_FILENO,": Unknown command.\n", strlen(": Unknown command.\n"));
        }
        else{
            if (in_background == false){
				wait(NULL);				
			}
            else{
                continue;
            }
        }
		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */

	}
	return 0;
}

