#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>

int pid;

void send_error(char *error);
int access_segment ( void );
int get_server_pid (int id);
void signal_handler(int signal);

int main(){
	int segment_id = access_segment();
	pid = get_server_pid(segment_id);

	int signals[2] = {SIGINT,SIGQUIT};
	struct sigaction sigact;
  sigemptyset( &sigact.sa_mask );
  sigact.sa_flags = 0;
  sigact.sa_handler = signal_handler;
  sigaction( signals[0], &sigact, NULL );
  sigaction( signals[1], &sigact, NULL );

  while(1) sleep(1);
}

void signal_handler(int signal){
  kill(pid,signal);	
}

int access_segment ( void ){
	key_t k ;
	int id ;
	k = ftok ("/etc/passwd", 'M') ;
	if (k == -1) send_error (" ftok ") ;
	id = shmget (k, 0,0) ;
	if (id == -1) send_error (" shmget ");
	return id ;
}

int get_server_pid (int id){
	int *addr ;
	addr = shmat (id , NULL , 0) ;
	int pid = addr[0];
	if ( addr == ( void *) -1) send_error (" shmat read ") ;
	if ( shmdt ( addr ) == -1) send_error (" shmdt ") ;
	return pid;
}

void send_error(char *error){
	perror(error);
}