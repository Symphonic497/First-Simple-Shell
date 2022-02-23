#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glob.h>
#include <pthread.h>
#include <utmpx.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sh.h"
#include "mp3.h"
#define MAXLINE 128
node_t *head;
static void handler(int);
static void *mythread(void *param)
{
	printf("got in thread!");
	node_t *tmp;
	struct utmpx *up;
	while(1){
		printf("restarting from top\n");
		setutxent();
		tmp = head;
		while(tmp){
			setutxent();
			while(up = getutxent()){
				if(up->ut_type == USER_PROCESS){
					//printf("\ncomparing data: %s, %s, output: %d\n",up->ut_user, tmp->name, strcmp(up->ut_user, tmp->name));
					if(strcmp(up->ut_user,tmp->name) == 0){
						printf("\n%s has logged on %s from %s\n",up->ut_user, up->ut_line, up->ut_host);
					}
				}
			}
			tmp = tmp->next;
		}
		sleep(15);
	}
}

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd, *lwd, *cmd, *ptr, *stat, *glob1;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  int bg = 0; //Background int
  struct passwd *password_entry;
  char **users = calloc(MAXARGS, sizeof(char*));
  char *homedir;
  struct pathelement *pathlist;
  char buf[MAX_CANON];
  DIR *dirwild;
  struct dirent *wilddirent;
  int noclobber = 0;
  int fid, fid2,fid3;
  int flagr;
  pthread_mutex_t lock;
  pthread_t tid1;
  pthread_mutex_init(&lock, NULL);
  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/
     
  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  lwd = owd; //init last work directory
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */
	  printf("%s", prompt);
	  printf("[%s]> ", owd);
	  int i = 0;
	  while(args[i]){
		  args[i] = NULL;
		  i++;
	  }
    /* get command line and process */
	 while((stat = fgets(commandline,MAX_CANON,stdin))!= NULL){
		  if (commandline[strlen(commandline) - 1] == '\n'){
			commandline[strlen(commandline) - 1] = '\0'; /* replace newline with null */
		  }
		  args[0] = command = strtok(commandline, " ");
		  int i = 1;
		  while (p = strtok(NULL, " ")){
			  args[i] = p;
			  i++;
		  }
	 if(!command){
		 break;
	 }
    /* check for each built in command and implement */
	  if(strcmp(command, "exit") == 0){
		  free(prompt);
		  free(commandline);
		  for(int i=0; i < MAXARGS; i++){
			  free(args[i]);
		  }
		  if(tid1){
		  	pthread_cancel(tid1);
		  	if(pthread_join(tid1, NULL) != 0){
			  	printf("pthread join ERROR\n");
		  	}
		  }
		  exit(0);
	  }
	  else if (strcmp(command,"which") == 0){
		  if(!args[1]){
			  printf("Invalid Input\n");
			  break;
		  }
		  printf("[Executing built-in which]\n");
		  int i = 1;
		  while(args[i]){
			  printf("Which: %s\n", args[i]);
			  cmd = which(args[i], pathlist);
			  printf("%s\n",cmd);
			  i++;
		  }
		  free(cmd);
		  break;
	  }
	  else if (strcmp(command,"where") == 0){
		  if(!args[0]){
			  printf("Invalid Input\n");
			  break;
		  }
		  printf("[Executing built-in where]\n");
		  int i = 1;
		  while(args[i]){
			  printf("Where: %s\n",args[i]);
			  cmd = where(args[i],pathlist);
			  printf("%s\n",cmd);
			  i++;
		  }
		  free(cmd);
		  break;
          }
	  else if (strcmp(command,"cd") == 0){
		  printf("[Executing built-in cd]\n");
		  if(args[2]){
			  printf("Too many arguments!\n");
			  break;
		  }
		  if(!args[1]){ //If no args in args[0]
			  chdir(homedir); //chdir homedir
			  lwd = owd; // lwd = last working dir
			  owd = homedir;
		  }
		  else if(strcmp(args[1],"-") == 0){ //change prev
			  chdir(lwd);
			  lwd = owd;
			  owd = getcwd(NULL,0);
		  }
		  else if(chdir(args[1]) == 0){ //change to abs path 
			  lwd = owd; 
			  owd = getcwd(NULL,0);
		  }else{
			  printf("Invalid Argument\n");
		  }
		  break;
          }
	  else if (strcmp(command,"pwd") == 0){
		  printf("[Executing built-in pwd]\n");
		  ptr = getcwd(NULL,0);
		  printf("[%s]\n",ptr);
		  break;
          }
	  else if (strcmp(command,"list") == 0){
		  printf("[Executing built-in list]\n");
		  if(!args[1]){ //no args exist
		  	ptr = getcwd(NULL,0);
		  	list(ptr);
		  }else{
			  int i = 1;
			  while(args[i]){
				  printf("%s:\n", args[i]);
				  list(args[i]);
				  i++;
			  }
		  }
		  break;
          }
	  else if (strcmp(command,"pid") == 0){
		  printf("[Executing built-in pid]\n");
		  int iptr = getpid();
		  printf("Shell PID: %d\n", iptr);
		  break;
          }
	  else if (strcmp(command,"kill") == 0){
		  printf("[Executing built-in kill]\n");
		  if (args[1]){
		  	int k = -1;
		  	if(!args[2]){
				k =  kill(atoi(args[1]),SIGTERM);
		  	}
		  	else if(args[2]){
				k =  kill(atoi(args[2]),abs(atoi(args[1])));
		  	}
		  	if (k < 0){
			  	printf("Kill: Failed: %d\n",k);
		  	}
		  	break;
		  }else{
			  printf("Invalid Arguments\n");
			  break;
		  }
          }
	  else if (strcmp(command,"prompt") == 0){
		  printf("[Executing built-in prompt]\n");
		  if(!args[1]){
			  printf("Input prompt prefix: ");
			   while(fgets(prompt,MAX_CANON,stdin)!= NULL){
				   if (prompt[strlen(prompt) - 1] == '\n'){
					   prompt[strlen(prompt) - 1] = '\0'; /* replace newline with null */
				   } break;
			   }
		  }
		  else{
			  strcpy(prompt,args[1]);
		  }
		  break;
          }
	  else if (strcmp(command,"printenv") == 0){
		  printf("[Executing built in printenv]\n");
		  if(args[3]){
			  printf("Invalid Input\n");
			  break;
		  }
		  if(!args[1]){
			  int i = 1;
			  while(envp[i] != NULL){
				  printf("%s\n", envp[i++]);
			  }
		  } else if(args[1]){
			  char *temp;
			  if ((temp = getenv(args[1])) != NULL){
			  printf("%s\n", temp);
			  }
			  else{
				  fprintf(stderr, "Error: Invalid Input\n");
			  }
		  }
		  break;
          }
	  else if (strcmp(command,"setenv") == 0){ //check HOME and PATH are updated properly
		  printf("[Executing built in setenv]\n");
		  if(args[3]){
			  printf("Invalid Input\n");
			  break;
		  }
		  int s = 0;
		  if(!args[1]){
			  int i = 1;
			  while(envp[i] != NULL){
				  printf("%s\n",envp[i++]);
			  }
			  break;
		  }else if((args[1]) && (!args[2])){
			  //printf("got in 1 args\n");
			  s = setenv(args[1],"",1);
		  }else if(args[2]){
			  //printf("Got in 2 args\n");
			  s = setenv(args[1],args[2],1);
		  }else{
			  fprintf(stderr, "Error: Invalid Input\n");
			  break;
		  }
		  if (s != 0){
			  printf("Error: Invalid Input\n");
			  break;
		  }
		  if(strcmp(args[1],"HOME") == 0){
			  homedir = args[2];
		  }
		  else if (strcmp(args[1], "PATH") == 0){
			  free(pathlist);
			  pathlist = get_path();
		  }
		  break;
          }
	  else if (strcmp(command,"watchuser") == 0){
		  if(args[3]){
			  printf("Error: Invalid Input\n");
			  break;
		  }if(!args[1]){
			  printf("Error: No Input\n");
			  //fflush(stdout);
			  break;
		  }
		  else if (!tid1){
			  insert(args[1], 0);
			  //printf("\nCreating Thread: %d\n", !tid1); for debugging
			  pthread_create(&tid1, NULL, mythread, NULL);
			  break;
		  }
		  if (args[2]){
			  if(strcmp(args[2],"off")!= 0){
				  printf("Error: Invalid Input, Try OFF\n");
			  }else{
				  pthread_mutex_lock(&lock);
				  del(args[1]);
				  pthread_mutex_unlock(&lock);
			  }
		  }else{//Add to list
			  //printf("got in ADD: arg = %s\n", args[1]);
			  pthread_mutex_lock(&lock);
			  insert(args[1], 0);
			  pthread_mutex_unlock(&lock);
			  //printf("got out of ADD");
		  }
		  break;
	  }//watchusr
	  else if(strcmp(command, "noclobber") == 0){
		  printf("[Executing Built-in noclobber]\n");
		  if (args[1]){
			  printf("Error: Invalid Input\n");
			  break;
		  }else if(noclobber == 1){//set Noclobber to 0
			  noclobber = 0;
			  printf("Value of noclobber is %d\n",noclobber);
		  }
		  else{//set Noclobber to 1
			  noclobber = 1;
			  printf("Value of noclobber is %d\n",noclobber);
		  }
		  break;
	  }
	  else{ /*  else  program to exec */
       /* find it */
		  if (access(command, X_OK) == 0){
			  //command is executable, do nothing
		  }
		  else{
			  command =  which(command,pathlist);
		  }
		  if(args[1]){ //if arguments to execute
			  int j = 1;
			  while(args[j]){
				  if(strlen(args[j])> 1){
					  if((strchr(args[j],'?') || strchr(args[j],'*')) > 0){
						  if(strlen(args[j]) > 1){ //if not just a lone wildcard
							  glob_t paths;
							  char **t;
							  glob(args[j], 0, NULL, &paths);
							  int i = j-1;
							  for(t = paths.gl_pathv; *t != NULL; ++t){
								  i++; 
								  args[i] = *t;
							  }
						  }
					  }
				  }else if((strlen(args[1]) == 1) && strchr(args[1],'*') > 0){
					  dirwild = opendir(owd);
					  int i = 1;
					  int cnt = 1;
					  while((wilddirent = readdir(dirwild)) != NULL){
						  if(i>2){
						  	args[cnt] = wilddirent->d_name;
							cnt++;
						  }
						  i++;
					  }
					  break;
				  }if(args[j][0] == '&'){
					  bg = 1; //background process = true
					  args[j] = NULL;
				  }
				  j++;
			  }
			  if(args[2]){
				  if(strcmp(args[1],">") == 0){// when noclobber = 0 Overwrite file
					  if(noclobber == 0){
						  //printf("Opening file > \n");
						  fid = open(args[2], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
						  flagr = 1;
					  }else{
						  if(access(args[2], F_OK) == 0){
							  printf("Cannot Overwrite File: noclobber = 1\n");
							  break;
						  }else{
						  	fid = open(args[2], O_RDWR | O_CREAT, S_IRWXU);
						  	flagr = 1;
						  }
					  }
				  }
				  else if(strcmp(args[1], ">&") == 0){//when noclobber = 0 overwrite file
					  //printf("opening file >&\n");
					  if(noclobber == 0){
                                                  fid = open(args[2], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
						  //printf("opened file");
                                                  flagr = 2;
                                          }else{
                                                  if(access(args[2], F_OK) == 0){
                                                          printf("Cannot Overwrite File: noclobber = 1\n");
							  break;
                                                  }else{
                                                  	fid = open(args[2], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                                                  	flagr = 2;
						  }
					  }

				  }
				  else if(strcmp(args[1], ">>") == 0){//when noclobber = 0 can create file 
					  if(noclobber == 0){
						  fid = open(args[2], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
						  flagr = 1;
					  }else{
						  if(access(args[2], F_OK) == 0){
							  fid = open(args[2], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
							  flagr = 1;
						  }else{
							  printf("Cannot Create File: noclobber = 1\n");
							  break;
						  }
					  }
				  }
				  else if(strcmp(args[1], ">>&") == 0){//when noclobber = 0 can create file
					  if(noclobber == 0){
						  fid = open(args[2], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
						  flagr = 2;
					  }else{
						  if(access(args[2], F_OK) == 0){
							  fid = open(args[2], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
							  flagr = 2;
						  }else{
							  printf("Cannot Create File: noclobber = 1\n");
							  break;
						  }
					  }
				  }
				  else if(strcmp(args[2], "<") == 0){
					  fid = open(args[3], O_RDWR | O_CREAT , S_IRWXU);
					  flagr = 3;
				  }
				  if(flagr == 1){
					//printf("closing stdout\n");
				  	close(1);
				  	fid2 = dup(fid);
				  	args[1] = NULL;
				  	args[2] = NULL;
					//flagr = 0;
					
				  }//if flag
				  else if(flagr == 2){
					  close(1);
					  fid2 = dup(fid);
					  close(2);
					  fid3 = dup(fid2);
					  args[1] = NULL;
					  args[2] = NULL;
				  }
				  else if(flagr == 3){
					  close(0);
					  fid2 = dup(fid);
					  args[2] = NULL;
					  args[3] = NULL;
				  }
			  }
		  }// arg parse
		  if(!command){
			  printf("Invalid Command\n");
			  break;
		  }//check no command
       /* do fork(), execve() and waitpid() */
	    if ((pid = fork()) < 0){
		    printf("fork error");
	    }
	    else if (pid == 0){
		    signal(SIGINT,handler);
		    //printf("Executing Command: %s\n", command); Used when DEBUGGING
		    execve(command,args,envp);
		    fprintf(stderr, "Couldn't execute Command: %s\n", command);
		    exit(127);
	    }
	    if(bg == 1){//Background process
		    //printf("got in BG wait PID\n"); //used when DEBUGGING
		    bg = 0;
		    if ((pid = waitpid(pid, &status, WNOHANG)) < 0){
			    printf("waitpid error");
	    	}
	    }else{ //background = false
		    if (flagr == 1){
		    	fid = open("/dev/tty", O_WRONLY);
		    	close(fid2);
		    	dup(fid);
		    	close(fid);
		    }
		    if (flagr == 2){
			    fid = open("/dev/tty", O_WRONLY);
			    close(fid2);
			    dup(fid);
			    close(fid);
			    
			    fid = open("/dev/tty", O_WRONLY);
			    close(fid3);
			    dup(fid);
			    close(fid);
		    }
		    if (flagr == 3){
			    fid = open("/dev/tty", O_RDONLY);
			    close(fid2);
			    dup(fid);
			    close(fid);
		    }
		    if ((pid = waitpid(pid, &status, 0)) < 0){
			    printf("waitpid error");
		    }
	    }
	    /*if(WEXITSTATUS(status)){
		    int exit_status = WEXITSTATUS(status);
		    printf("Exit Status of Child: %d is %d\n",pid,exit_status);
	    }*/
	    break;
	  }//program to exec
	 }//fgets while
	 if (!stat){
		 printf("\nUse 'exit' to leave shell\n");
	 }
  }//while
  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist )
{
	char *cmd = (char *) malloc(128);
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */
	while(pathlist){
		sprintf(cmd, "%s/%s", pathlist->element,command);
		if(access(cmd, X_OK) == 0){
			return(cmd);
		}
		pathlist = pathlist->next;
	}
	return(NULL);
} /* which() */

char *where(char *command, struct pathelement *pathlist )
{
	char *cmd = (char *) malloc(128);
	char *cmdlist = (char *) malloc(128);
  /* similarly loop through finding all locations of command */
	while(pathlist){
		sprintf(cmd, "%s/%s", pathlist->element,command);
		if(access(cmd, X_OK) == 0){
			strcat(cmdlist,cmd);
		}
		pathlist = pathlist->next;
	}
	free(cmd);
	if(cmdlist){
		return(cmdlist);
	}
	return(NULL);
} /* where() */

void list ( char *dir )
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
	struct dirent *dirent1;
	DIR *dir1;
	dir1 = opendir(dir);
	int i = 0;
	if(dir1 != NULL){
		while ((dirent1 = readdir(dir1)) != NULL){
			if(i>1){
				printf("%s\n",dirent1->d_name);
			}
			i++;
		}
	}else{
		printf("Directory does not exist\n");
	}
} /* list() */

void handler(int siggy){//child signal handler
	signal(SIGINT,SIG_DFL);
	kill(SIGINT,pid);
}
