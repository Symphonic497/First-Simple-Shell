#include "sh.h"
#include <signal.h>
#include <stdio.h>

static void sig_handler(int); 

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */
	if ((signal(SIGINT, sig_handler)) == SIG_ERR){
		printf("Signal Processing Error");
	}
	signal(SIGTERM,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_DFL);

  return sh(argc, argv, envp);
}

void sig_handler(int sig)
{
  /* define your signal handler */
	printf("\nRecieved Signal: %s\n", strsignal(sig));
}

