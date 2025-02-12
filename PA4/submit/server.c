#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include "db.h"
#define MAXLINE 128
#define MAX_KEYLEN 1024
int max;
int client_num=0;
//db_t* DB;

void handler(int sig){

	//free(fd_list);
	db_close(DB);
	printf("\nDB closed\n");
	exit(0);
}
void *pthread_main(void *cfd){
	int n; 
	char buf[MAXLINE];
	int connfd = *((int*)cfd);
	//printf("connfd:%d\n",connfd);
	pthread_detach(pthread_self());
	free(cfd);
	int i=0;
	if(client_num >= max){ // overcrowd client num
		if(write(connfd,"Too many clients\n",17));
		close(connfd);
		//pthread_exit(NULL);
		return NULL;
	}else {
		client_num++;
	}
	int connect=0;
	while((n=read(connfd,buf+i,1) >0))
	{	
		if(buf[i++] != '\n') continue; // until get \n
		if(connect == 0){
			if(!strcmp(buf,"CONNECT\n")){
				connect=1;
				if(write(connfd,"CONNECT_OK\n",11));
				memset(buf,'\0',sizeof(buf));
				i=0;
				continue;
			} else {					
				if(write(connfd,"UNDEFINED PROTOCOL\n",19)); //client 
				memset(buf,'\0',sizeof(buf));
				i=0;
				continue;
			}
		} else {
			if(!strncmp(buf,"DISCONNECT\n",11)){ // disconnect
				if(write(connfd,"BYE\n",4));
				memset(buf,'\0',sizeof(buf));
				close(connfd);
				client_num--;
				return NULL;
				//pthread_exit(NULL);
			} else {
				// db_get, db_put
				// dealing PUT [a] [12], GET [a]
				char* val;
				val = (char*)calloc(sizeof(char),MAXLINE);
				//char val[MAXLINE];
				char key[MAX_KEYLEN];
				int key_len, val_len;
				// parse "buf" GET [KEY]
				char *p1 = strchr(buf,'[');
				if((p1!= NULL)&&(p1+1) != NULL) p1 +=1;
				else {
					if(write(connfd,"UNDEFINED PROTOCOL\n",19)); //client 
					memset(buf,'\0',sizeof(buf));
					i=0;
					continue;					
				}
				char *p2 = strchr(p1,'[');
				char *end = strchr(buf,']');
				if(end == NULL){
					if(write(connfd,"UNDEFINED PROTOCOL\n",19)); //client 
					memset(buf,'\0',sizeof(buf));
					i=0;
					continue;						
				}
				end[0] = '\0';
				strcpy(key,p1);
				if(p2 != NULL){
					p2 +=1;
					end = strchr(p2,']');
					if(end == NULL){
						if(write(connfd,"UNDEFINED PROTOCOL\n",19)); //client 
						memset(buf,'\0',sizeof(buf));
						i=0;
						continue;						
					}
					end[0] = '\0';
					memcpy(val,p2,end-p2+1);
				} 

				char send_data[MAX_KEYLEN*2];
				key_len = strlen(key);
				//DB_GET = read
				if(!strncmp(buf,"GET",3)){ // db_get - read

					val = db_get(DB, key, key_len, &val_len);
					if (val == NULL) {
						sprintf(send_data,"GETINV\n");
						if(write(connfd,send_data,strlen(send_data)));
					} else {
						sprintf(send_data,"GETOK [%s] [%d]\n", key, *((int*)val));
						if(write(connfd,send_data,strlen(send_data)));
						free(val);
					}				
				} else if(!strncmp(buf,"PUT",3)){ // db_put - write
					if(p2 == NULL){
						if(write(connfd,"UNDEFINED PROTOCOL\n",19)); //client 
						memset(buf,'\0',sizeof(buf));
						i=0;
						continue;	
					}
					db_put(DB, key, key_len, val , sizeof(int));
					sprintf(send_data,"PUTOK\n");
						if(write(connfd,send_data,strlen(send_data)));
					free(val);
					//if(write(connfd,"PUTOK\n",6);
				} else {					
					if(write(connfd,"UNDEFINED PROTOCOL\n",19)); //client 
					memset(buf,'\0',sizeof(buf));
					i=0;
					continue;
				}
				i=0;
				memset(send_data,'\0',sizeof(send_data));
				memset(buf,'\0',sizeof(buf));
			}
			i=0;
			memset(buf,'\0',sizeof(buf));
		}
	}
	close(connfd);
	client_num--;
	return NULL;
}
int main(int argc, char** argv){ // ./server 8888 6 128
	signal(SIGINT,handler);
	signal(20,handler);
	//signal(SIGPIPE,SIG_IGN);
	max = atoi(argv[2]); // 6
	Lsize = atoi(argv[3]);
	//int* fd_list = (int*)calloc(sizeof(int),max);
	int* connfd;
	int listenfd;
	socklen_t caddrlen ;
	//struct hostent *h;
	struct sockaddr_in saddr, caddr ;
	int port =atoi ( argv [1]); // 8888
	//time_t t;
	//pthread_t client; // client number
	pthread_t client[max+1];
	if (( listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf ("socket() failed.\n");
		exit(1);
	}
	bzero ((char *)& saddr, sizeof ( saddr ));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr =	htonl (INADDR_ANY);
	saddr.sin_port =htons (port);
	
	if (bind( listenfd , ( struct sockaddr *)& saddr, sizeof ( saddr )) < 0) {
		printf ("bind() failed.\n");
		exit(2);
	}


	if (listen( listenfd , 5) < 0) {
		printf ("listen() failed.\n");
		exit(3);
	}	
	DB = db_open(Lsize);	
	if (DB == NULL) {
		printf("DB not opened\n");
		return -1;
	}
	printf("DB opened\n");
	while(1){
		caddrlen = sizeof(caddr);
		connfd = (int*)malloc(sizeof(int));
		if (( *connfd = accept( listenfd , ( struct sockaddr *)& caddr , & caddrlen )) < 0) {
			printf ("accept() failed.\n");
			continue;
		}
		pthread_create(&client[client_num], NULL, pthread_main, connfd);
	}
	db_close(DB);
	printf("\n2 DB closed\n");
}
