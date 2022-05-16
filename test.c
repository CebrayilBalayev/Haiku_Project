#include <stdio.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <CUnit/CUnit.h>
#include <err.h>
#include <CUnit/Basic.h>

#define WESTERN_SIZE 9 // 9 western haiku txt files
#define JAPANESE_SIZE 6 // 6 japanese haiku txt files

int count[3] = {0,0,0};
int japan = 1;
int western = 2;

typedef struct message{
  long mtype; // for Haiku category
  char text[256]; // storing Haiku here
} Message ;

char **western_haikus; 
char **japanese_haikus;

//signal handlers
void signal_handler(int signal);
void close_server(int signal);

//thread functions
void *fWriter ( void * arg );
void *fReader ( void * arg );

//haiku functions
void write_haiku(int category);
void read_haiku(int category);

//queue functions
int create_queue ( void );
int access_queue ( void );
void write_value (int id , int category, char* text);
Message read_value (int id, int category);
void remove_queue ( int id );

//shared memory functions
int create_segment ( void );
int access_segment ( void );
void send_server_pid (int id , pid_t server_pid);
void remove_segment ( int id );

//additional useful functions
char **fillarray(char *dir_name,int size);
void send_error(char *error);
void swap(int *a,int *b);
int random0n(int n);

//additional functions for tests
void read_haiku_for_test(int category);//same just without printf
int get_server_pid (int id);

//testing functions
void run_tests();
void test_write_and_read();
void test_read_write_queue();
void test_pid();
void test_nbfiles();
void test_fillarray();
void test_swap();
void test_access_queue();
void test_random0n();

int main() {
  srand(time(NULL));

  western_haikus=fillarray("Western",WESTERN_SIZE);
  japanese_haikus=fillarray("Japan",JAPANESE_SIZE);

  int segment_id = create_segment();
  pid_t server_pid = getpid();
  send_server_pid(segment_id,server_pid);

  int queue_id=create_queue();

  run_tests();
}

void signal_handler(int signal){
  pthread_t threads[3];
  int e;
  int categories[3]={japan,western,(signal==2)?japan:western};
  for(int i=0;i<3;i++)
  if(i>1){
      if ((e = pthread_create(& threads[i], 
      NULL , fReader, &categories[i])) != 0)
      send_error (" pthread_create ") ;}
  else {
      if((e = pthread_create(& threads[i], 
      NULL , fWriter, &categories[i])) != 0)
      send_error (" pthread_create ") ;}
}

void close_server(int signal){
  int segment_id = access_segment();
  remove_segment(segment_id);
  int queue_id=access_queue();
  remove_queue(queue_id);
  exit(0);
}

void *fWriter ( void * arg ){
  int i = * ( int *) arg ;
  if(count[i]<1){
    if(i==japan) count[i] = JAPANESE_SIZE;
    else if(i==western) count[i] = WESTERN_SIZE;
    write_haiku(i);
  }
  pthread_exit (NULL) ;
}

void *fReader ( void * arg ){
  int i = * ( int *) arg ;
  printf("\033[47m\033[31mreading category %s \033[0m\n",(i==1)?"Japan":"Western");
  read_haiku(i);
  pthread_exit (NULL) ;
}

void write_haiku(int category){
  int id=access_queue();

  int randomIndex;
  int *arrayOfIndexes;
  int size;

  if(category == western) size = WESTERN_SIZE;
  else if(category == japan) size = JAPANESE_SIZE;

  arrayOfIndexes=(int *) malloc(size * sizeof(int));
  for(int i=0;i<size;i++) arrayOfIndexes[i]=i;

  for(int i=0; i < size ;i++){
    randomIndex=random0n( size-i );
    write_value(id , category,
    (category == western) ? western_haikus[ arrayOfIndexes[randomIndex] ]
    : japanese_haikus[ arrayOfIndexes[randomIndex] ] );
    swap(&arrayOfIndexes[randomIndex] , &arrayOfIndexes[size -i-1] );
  }
}

void read_haiku(int category){
  int id=access_queue();
  Message m;
  count[category]--;
  m=read_value (id, category);
  printf("%s\n",m.text);
}

int create_queue ( void ){
  key_t k ;int id ;
  k = ftok ("/etc/passwd", 'L') ;
  if (k == -1)
  send_error (" ftok ") ;
  id = msgget (k, IPC_CREAT | 0666) ;
  if(id == -1)
  send_error (" msgget ");
  return id ;
}

int access_queue ( void ){
  key_t k ;int id ;
  k = ftok ("/etc/passwd", 'L') ;
  if (k == -1) send_error (" ftok ");
  id = msgget (k, 0) ;
  if (id == -1) send_error (" msgget ") ;
  return id ;
}

void write_value (int id , int category, char* text){
  Message m;
  m.mtype = category ;
  strcpy(m.text,text);
  int r = msgsnd (id , &m, sizeof m - sizeof m.mtype , 0) ;
  if (r == -1) send_error (" msgsnd ") ;
}

Message read_value (int id, int category){
  Message m ;
  int r = msgrcv (id , &m, sizeof m - sizeof m.mtype , category, 0) ;
  if (r == -1) send_error (" msgrcv ") ;
  return m ;
}

void remove_queue ( int id ){
  int r = msgctl (id , IPC_RMID , NULL ) ;
  if (r == -1) send_error (" msgctl ");
}

int create_segment ( void ){
  key_t k ;
  int id ;
  k = ftok ("/etc/passwd", 'M') ;
  if (k == -1) send_error (" ftok ") ;
  id = shmget (k, 4096 , IPC_CREAT | 0666) ;
  if (id == -1) send_error (" shmget ");
  return id ;
}

void send_server_pid (int id , pid_t server_pid){
  int *addr ;
  addr = shmat (id , NULL , 0) ;
  if ( addr == ( void *) -1 ) send_error (" shmat write ") ;
  addr[0] = server_pid;
  if ( shmdt ( addr ) == -1) send_error (" shmdt ") ;
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

void remove_segment ( int id ){
  int r ;
  r = shmctl (id , IPC_RMID , NULL ) ;
  if (r == -1) send_error (" shmctl ") ;
}

char **fillarray(char *dir_name,int size){
  char **array;
  array=(char **)calloc(size,sizeof(char *));
  for(int i=0;i<size;i++)array[i]=(char *)calloc(110,sizeof(char));

  int arraysize=0;
  struct dirent *filesInDirectory;
  DIR *directory = opendir(dir_name);

  while((filesInDirectory = readdir(directory)) != NULL){
    if(filesInDirectory->d_name[0]=='.')continue;
    //printf("reading => %s\n", filesInDirectory->d_name);
    char name[13];
    strcpy(name, dir_name);
    strcat(name, "/");
    strcat(name, filesInDirectory->d_name);

    int file = open(name, O_RDONLY);
    char c;
    int strsize=0;
    while(read(file, &c, 1)){
      array[arraysize][strsize]=c;
      strsize++;
    }
    arraysize++;
    close(file);
  }
  return array;
}

void send_error(char *error){
  perror(error);
}

int random0n(int n){
  return random()%n;
}

void swap(int *a,int *b){
  int tmp=*a;
  *a=*b;
  *b=tmp;
}

void read_haiku_for_test(int category){//without printing
  int id=access_queue();
  Message m;
  count[category]--;
  m=read_value (id, category);
}

int get_server_pid (int id){//just for tests
  int *addr ;
  addr = shmat (id , NULL , 0) ;
  int pid = addr[0];
  if ( addr == ( void *) -1) send_error (" shmat read ") ;
  if ( shmdt ( addr ) == -1) send_error (" shmdt ") ;
  return pid;
}

//-------------------------------------------------------------------------------

void run_tests() {
  //Foldertestsuit interprocomsuite additioalsuite
  if (CU_initialize_registry() != CUE_SUCCESS)
    printf("%s\n","can't initialize test registry" );
  
  // CU_pSuite DirectorySuite = NULL;
  // CU_pSuite Inretproscomsuit = NULL;
  // CU_pSuite Addfuncsuit = NULL;


  CU_pSuite DirectorySuite = CU_add_suite("DirAccesibily", NULL, NULL);
  CU_pSuite Inretproscomsuit = CU_add_suite("ProcessCommunication", NULL, NULL);;
  CU_pSuite Addfuncsuit = CU_add_suite("HelperFunctions", NULL, NULL);;

  if (CU_get_error() != CUE_SUCCESS) printf("%s\n","error_1" );

  /*-----------------------------------------------------------------*/
  CU_add_test(Addfuncsuit, "test of random0n function", test_random0n);
  CU_add_test(Addfuncsuit, "test of swap function", test_swap);
  /*----------------------------------------------------------------*/
  CU_add_test(Inretproscomsuit, "test of pid function", test_pid);
  CU_add_test(Inretproscomsuit, "test of access queue function", test_access_queue);
  CU_add_test(Inretproscomsuit, "test of pid function", test_read_write_queue);
  /*----------------------------------------------------------------*/
  CU_add_test(DirectorySuite, "test of test_fillarray function", test_fillarray);
  CU_add_test(DirectorySuite, "test of test_readwrite function", test_write_and_read);
  CU_add_test(DirectorySuite, "test of test_nbfiles_infolder function", test_nbfiles);
  /*----------------------------------------------------------------*/



/*
  CU_pSuite random0nSuite = CU_add_suite("random0n", NULL, NULL);
  CU_pSuite swapSuite = CU_add_suite("swap", NULL, NULL);
  CU_pSuite access_queueSuite = CU_add_suite("access_queue", NULL, NULL);
  CU_pSuite fillarraySuite = CU_add_suite("fillarray", NULL, NULL);
  CU_pSuite DirectorySuite = CU_add_suite("foldercheck", NULL, NULL);
  CU_pSuite readwriteSuite = CU_add_suite("readwrite", NULL, NULL);
  CU_pSuite queueSuite = CU_add_suite("queuecheck", NULL, NULL);
  CU_pSuite pidSuite = CU_add_suite("pid", NULL, NULL);
*/
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
}

void test_write_and_read(){
  pthread_t thread;
  int category = japan;
  pthread_create(&thread, NULL , fWriter, &category);
  pthread_join(thread,NULL);
  CU_ASSERT(count[japan]==JAPANESE_SIZE);
  
  for(int i=0;i<JAPANESE_SIZE;i++) read_haiku_for_test(japan);
  CU_ASSERT(count[japan]==0);

  category = western;
  pthread_create(&thread, NULL , fWriter, &category);
  pthread_join(thread,NULL);
  CU_ASSERT(count[western]==WESTERN_SIZE);
  
  for(int i=0;i<WESTERN_SIZE;i++) read_haiku_for_test(western);
  CU_ASSERT(count[western]==0);
}

void test_read_write_queue(){
  int priority = 5;//never used inside server
  char *check = "Test 1 2 3";
  int queue_id = access_queue();

  write_value(queue_id, priority, check);
  Message mes;
  mes = read_value (queue_id, priority);
  CU_ASSERT_TRUE(strcmp(check,mes.text)==0);
}

void test_pid(){
  pid_t actual_pid = getpid();

  int segment_id = access_segment();
  pid_t pidWrittenToSegment = get_server_pid(segment_id);

  CU_ASSERT(actual_pid==pidWrittenToSegment);
}

void test_nbfiles(){
  struct dirent *filesInDirectory;
  DIR *directory = opendir("Japan");
  int count = 0;
  while((filesInDirectory = readdir(directory)) != NULL){
    if(filesInDirectory->d_name[0]=='.')continue;
    count++;
  }
  CU_ASSERT(count==JAPANESE_SIZE);

  directory = opendir("Western");
  count = 0;
  while((filesInDirectory = readdir(directory)) != NULL){
    if(filesInDirectory->d_name[0]=='.')continue;
    count++;
  }
  CU_ASSERT(count==WESTERN_SIZE);
}

void test_fillarray() {
  //we check using local arrays as there might be some side effects because of global arrays
  char **western = fillarray("Western", 9);
  char **japan = fillarray("Japan", 6);
  for(int i=0; i<9;i++) CU_ASSERT_TRUE(strcmp(western_haikus[i], western[i])==0);  
  for(int i=0;i<6;i++) CU_ASSERT_TRUE(strcmp(japanese_haikus[i], japan[i])==0);    
}

void test_swap() {
  int a = 5;
  int b = 2;

  swap(&a, &b);

  CU_ASSERT_EQUAL(a, 2);
  CU_ASSERT_EQUAL(b, 5);  
}

void test_access_queue() {
  int id = access_queue();
  CU_ASSERT(id >= 0);
}

void test_random0n() {
  int result;
  for(int i=0;i<14;i++) {
    result = random0n(100);
    CU_ASSERT_TRUE(result>=0 && result<100);
  }
}