#include <stdlib.h>
#include <stdio.h>
#include "sock352.h"
#include <sys/socket.h>
#include <errno.h>

int main(int argc, char ** argv) {








	 int sockfd,n;
	   struct sockaddr_in servaddr,cliaddr;
	   socklen_t len;
	   char mesg[1000];

	   sockfd=socket(AF_INET,SOCK_DGRAM,0);

	   bzero(&servaddr,sizeof(servaddr));
	   servaddr.sin_family = AF_INET;
	   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	   servaddr.sin_port=htons(32000);
	   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));





		struct timeval tv;

		tv.tv_sec = 2; /* 30 Secs Timeout */
		tv.tv_usec = 200000;  // Not init'ing this can cause strange errors

		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));



	   for (;;)
	   {
	      len = sizeof(cliaddr);
	      n = recvfrom(sockfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);
	      puts("recv\n");
	      n = read(sockfd,mesg,1000);
	      printf("read, %d\n", n);
	      sendto(sockfd,mesg,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	      printf("-------------------------------------------------------\n");
	      mesg[n] = 0;
	      printf("Received the following:\n");
	      printf("%s",mesg);
	      printf("-------------------------------------------------------\n");
	   }
	 return 0;



}
