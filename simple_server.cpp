/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include "http_server.hh"
#include <queue>

#define qsize 5
#define threads 10

void error(char *msg) {
  perror(msg);
  exit(1);
}

//Queue for storing connections socket
queue<int> opensocks;
//Mutex for locking and unlocking access to queue
pthread_mutex_t m;
//Condition variables for sleep and wakeup
pthread_cond_t c, cmain;


//Worker Threads Function
void* thread_func(void* arg)
{
  while(1){
    int newsockfd=0;
    pthread_mutex_lock(&m);                   //Mutex Lock
    while(opensocks.size()==0)                //If queue gets empty worker threads sleep
      pthread_cond_wait(&c, &m);              
    newsockfd = opensocks.front();            //Else take a thread to provide response
    opensocks.pop();
    if(opensocks.size()==qsize-1)
      pthread_cond_signal(&cmain);            //Signal to wakeup 
    pthread_mutex_unlock(&m);                 //Mutex Unlock

    int n=1;
    char buffer[10000];
    /* read message from client */
    bzero(buffer, 10000);
    n = read(newsockfd, buffer, 10000);
    if (n <= 0){
      printf("ERROR reading from socket");
      close(newsockfd);
      continue;
    }

    /* send reply to client */
    HTTP_Response *res = handle_request(string(buffer));
    string ans = res->get_string();
    n = write(newsockfd, ans.c_str(), ans.size());
    // printf("tid = %ld\n", syscall(SYS_gettid));
    if (n < 0)
      printf("ERROR writing to socket");
    close(newsockfd);
  }
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  /* listen for incoming connection requests */

  listen(sockfd, 10);
  clilen = sizeof(cli_addr);
  pthread_t tid;
  for(int i=0; i<threads; i++){
    pthread_create(&tid, NULL, thread_func, NULL);
  }

  while(1){
    /* accept a new request, create a newsockfd */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0){
      printf("ERROR on accept");
      continue;
    }
    pthread_mutex_lock(&m);                   //Mutex Lock
    if(opensocks.size()==qsize){              
      printf("FULL");
      //Acceptor Sleeps
      pthread_cond_wait(&cmain, &m);          //Main Thread Sleeps when queue is full
    }
    opensocks.push(newsockfd);                //Else pushes new socketfd
    pthread_cond_signal(&c);                  //Wakeup signal for thread
    pthread_mutex_unlock(&m);                 //Mutex Unlock
  }
  close(sockfd);
  return 0;
}
