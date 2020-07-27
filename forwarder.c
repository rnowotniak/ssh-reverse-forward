/*
 * Forwarder of local sshd service via reverse connection
 * Robert Nowotniak <rnowotniak@gmail.com>  2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <sys/select.h> 

#define SSHPORT 22
#define DSTPORT 3690

#define BUFSIZE 1024

#define SSHADDR "212.191.89.2"
// #define SSHADDR "127.0.0.1"
#define DSTADDR "91.185.186.43"

int main() {
	struct sockaddr_in ssh1;
	struct sockaddr_in dst1;
	int e;
	char buf[1024];

	int s = socket(PF_INET, SOCK_STREAM, 0);
	if (s < 0 ) {
		perror("socket");
		return -1;
	}
	int d = socket(PF_INET, SOCK_STREAM, 0);
	if (d < 0 ) {
		perror("socket");
		return -1;
	}

	// Connect to ssh server
	ssh1.sin_family = AF_INET;
	ssh1.sin_port = htons(SSHPORT);
	e = inet_pton(AF_INET, SSHADDR, &ssh1.sin_addr);
	e = connect(s, (const struct sockaddr *) &ssh1, sizeof(ssh1) );
	if (e < 0 ) {
		perror("connect to ssh");
		return -1;
	}


	// Connect to listening port to forward ssh there
	dst1.sin_family = AF_INET;
	dst1.sin_port = htons(DSTPORT);
	e = inet_pton(AF_INET, DSTADDR, &dst1.sin_addr);
	e = connect(d, (const struct sockaddr *) &dst1, sizeof(dst1) );
	if (e < 0 ) {
		perror("connect to listening address");
		return -1;
	}

	fd_set readfds, writefds;

	
	for (;;) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);
		FD_SET(d, &readfds);

		select(s<d?d+1:s+1, &readfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0);

		if (FD_ISSET(s, &readfds)) {
			int n = read(s, buf, sizeof(buf));
			printf("read %d from ssh server\n", n); fflush(stdout);
			if (n <= 0) {
				perror("read(s)");
				return 0;
			}
			n = write(d, buf, n);
			fsync(d);
			printf("wrote %d to remote\n", n); fflush(stdout);
		}
		if (FD_ISSET(d, &readfds)) {
			int n = read(d, buf, sizeof(buf));
			printf("read %d from remote\n", n); fflush(stdout);
			if (n <= 0) {
				perror("read(d)");
				return 0;
			}
			n = write(s, buf, n);
			fsync(s);
			printf("wrote %d to remote\n", n); fflush(stdout);
		}
	}

	return 0;
}


