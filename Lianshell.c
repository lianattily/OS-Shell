/*********************************************************************************
 * STUDENT NAME: LIAN ATTILY
 * COURSE: EECS3221 Operating Systems
 * PROJECT 1 - SIMPLE C SHELL 
*********************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h> //for strtok
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_LINE 80 /* The maximum length command */
#define DEL " "
#define BUFFER_SIZE 10 
/*************************GLOBAL VARIABLES***********************************************/
int input_redirection_flag = 0;
int output_redirection_flag = 0;    
int piping_flag = 0;        //| flag
int amp_flag=0; //& flag
int hist_flag=0;
char* input_FILE=NULL;
char* output_FILE=NULL;
char* piping_args[MAX_LINE];
int pipefd[2];
char *history[BUFFER_SIZE]; //history array to store history commands
int count=0;
char **cmdpnt;
/*************************FUNCTION PROTOTYPES***********************************************/
void displayHistory();
void piping_handle(char* args[], char* piping_args[], int pipefd[]);
void execute(char *arg, char **args);
char ** process_line(char *temp[], char line[]);
char* returninstruction(int num);
int pipe_and_redirection_checking(char* temp[]);
void check_line(char *temp[]);
void pos(char *args[], char *temp[], char* piping_args[]);
int hist(char inputBuffer[], char *args[]);
void remove_endln(char line[]);
void read_line(char line[]);
int read_parse_line(char *args[], char line[], char* piping_args[]);

/*************************FUNCTION IMPLEMENTATIONS***********************************************/
void displayHistory()
{
        printf("%s \n", history[count]);
} 

void piping_handle(char* args[], char* piping_args[], int pipefd[]){
    int pid, status;
    pid=fork();
    if(pid==0){ //in grandchild
        dup2(pipefd[1], 1);
		close(pipefd[0]); //parent doesnt need this end of the pipe
        close(pipefd[1]);
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    }
    int pid2=fork();
	if(pid2==0){
        dup2(pipefd[0], 0);
		close(pipefd[1]);   //child doesnt need this end of pipe
        close(pipefd[0]);
        execvp(piping_args[0] , piping_args);
        perror(piping_args[0]);
		exit(EXIT_FAILURE);
    }
	close(pipefd[0]);
    close(pipefd[1]);
}

void execute(char *arg, char **args){

	    //Exit shell 
        if(strcmp(args[0], "exit")==0){exit(0);}

        //execute cd command
        if (strcmp (args[0],"cd") == 0) {
            if (args[1] == NULL)
                chdir (getenv ("HOME"));
            else
            if(chdir(args[1])!=0) printf("%s: No such file or directory\n", args[1]); 
            return ;
        }          
    //fork() process for the rest of the commands 
    pid_t child_pid=fork();
    if(child_pid==0){   //CHILD PROCESS
        if(input_redirection_flag==1 && input_FILE!=NULL){
            dup2(open(input_FILE, O_RDWR|O_CREAT,0777),0);
        }
        if(output_redirection_flag==1 && output_FILE!=NULL){
            dup2(open(output_FILE, O_RDWR|O_CREAT,0777),1);
        }
        if(piping_flag==1){
            piping_handle(args, piping_args, pipefd);
            exit(0);
        }
        if (execvp(args[0], args) == -1 && strcmp(args[0], "!!") != 0)  perror(args[0]);//printf("Unknown command - could not execute %s\n", args[0]);
        
    }
    else if(child_pid>0) {  //PARENT PROCESS
         if(amp_flag==1) {printf("CONCURRENT PROCESS WITH CHILD ID [1]: %d\n",child_pid); amp_flag=0; }
         else waitpid(child_pid,NULL,0);
        input_redirection_flag=0;
        output_redirection_flag=0;
		piping_flag=0;
        input_FILE=NULL;
        output_FILE=NULL;
        amp_flag=0;
        close(pipefd[0]); close(pipefd[1]);
        }
	else{
		printf("Fork failed: %d\n",child_pid);
	}

}

char ** process_line(char *temp[], char line[]){

    int i=0;
    temp[i]=strtok(line, " ");

    while(temp[i]!=NULL){
        i++;
        temp[i]=strtok(NULL, " ");
    }
   // return 1;
	return temp;
}

char* returninstruction(int num){
    char *temp = malloc((strlen(history[num]))*sizeof(char));
    strcpy(temp,history[num]);
    return temp;

}

int pipe_and_redirection_checking(char* temp[]){
    int i=0;
    while(temp[i]!=NULL){
        if(strcmp(temp[i], ">")==0){
            output_FILE=temp[i+1];
            output_redirection_flag=1;
            return i;
        }
        if(strcmp(temp[i],"<")==0){
            input_FILE=temp[i+1];
            input_redirection_flag=1;
            return i;
        }
        if(strcmp(temp[i],"|")==0){
            piping_flag=1;
            return i;
        }
        if(strcmp(temp[i],"&")==0){
            amp_flag=1;
            return i;
        }
        i++;
    }
    return i;
}

void check_line(char *temp[]){
    int i=0, pipe_cnt=0, output_redirection_cnt=0,input_redirection_cnt=0, amp_flag=0;
    int total=0;
    if(temp[i]==NULL){
        return ;
    }
    while(temp[i]!=NULL){
        if(strcmp(temp[i], ">")==0){
            output_redirection_cnt++;
        }
        if(strcmp(temp[i],"<")==0){
            input_redirection_cnt++;
        }
        if(strcmp(temp[i], "|")==0){
            pipe_cnt++;
            piping_flag=1;
        }
        if(strcmp(temp[i], "&")==0){
            amp_flag=1;
        }
        i++;
    }
    total=input_redirection_cnt+output_redirection_cnt+pipe_cnt+amp_flag;
    if(total>1){
        printf("ERROR: Cannot support this case\n");
        exit(1);
    }
}

void pos(char *args[], char *temp[], char* piping_args[]){
	int pos;
	pos = pipe_and_redirection_checking(temp);

    int i=0;
    while(i<pos){
        args[i]=temp[i];
        i++;
    }
    args[i]=NULL;
    i++;
    if(piping_flag==1){
        int j=0;
       while(temp[i]!=NULL){ 
           piping_args[j]=temp[i];
           i++;
           j++;
       }
       piping_args[j]=NULL;
    }
}

int hist(char input[], char *args[]){
    int i=0;
	char* temp[MAX_LINE];
	char *cmd;
    if(strcmp(args[0],"!!")==0){	
            if(count>0){
				displayHistory();
				cmd = returninstruction(count);
				cmdpnt = process_line(temp, cmd);
				check_line(temp);
				pos(cmdpnt, temp, piping_args);
				execute(cmdpnt[0],cmdpnt);
				free(cmd);
            }
			else {printf("No commands in history yet\n"); return -1;}
}
else {
	 if(count<BUFFER_SIZE)
            {    count++;
                history[count] = strdup(input);
            }
            else if(count==BUFFER_SIZE)
                {    int x;
                    for(x=1;x<BUFFER_SIZE;x++)
                        history[x-1] = history[x];
                    history[BUFFER_SIZE-1] = strdup(input);
                }
}
}

void remove_endln(char line[]){
    int i=0;
    while(line[i]!='\n')i++;

    line[i]='\0';   //replace '\n' with '\0'
}

void read_line(char line[]){
    char* ret = fgets(line, MAX_LINE, stdin);
    remove_endln(line);
    if(strcmp(line, "exit")==0 || (ret==NULL))exit(0);
}

int read_parse_line(char *args[], char line[], char* piping_args[]){
    char* temp[MAX_LINE];
	char histline[MAX_LINE];
    int i=0;

    read_line(line);
    strcpy(histline, line);
    process_line(temp, line);
    check_line(temp);
    int pos;
	pos = pipe_and_redirection_checking(temp);

	//separating arguments
    while(i<pos){
        args[i]=temp[i];
        i++;
    }
    args[i]=NULL;
    i++;
    if(piping_flag==1){
        int j=0;
       while(temp[i]!=NULL){ 
           piping_args[j]=temp[i];
           i++;
           j++;
       }
       piping_args[j]=NULL;
    }
    hist(histline, temp);
    return 1;
}

int main(void) {
    char *args[MAX_LINE/2 + 1]; /* command line arguments*/  
    char line[MAX_LINE];
    int should_run = 1; /* flag to determine when to exit program */
	if(pipe(pipefd)==-1) {printf("An error occurred while piping\n"); return 1;}
    while (should_run){
        printf("mysh:~$ "); 
        fflush(stdout);
        fflush(stdin);
        read_parse_line(args, line, piping_args);
		if(strcmp(line,"exit")==0) {exit(0);}
		execute(args[0], args);
		
    }
return 0;
}