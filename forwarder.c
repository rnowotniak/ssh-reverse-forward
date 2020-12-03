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
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <sys/select.h> 
#include <signal.h> 

#define DEBUG 0

#define SSHPORT 22
#define DSTPORT 3690

#define BUFSIZE 1024

#define SSHADDR "212.191.89.2"
// #define SSHADDR "127.0.0.1"
#define DSTADDR "91.185.186.43"


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table() {

    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


void base64_cleanup() {
    free(decoding_table);
}

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}




int main(int argc, char *argv[]) {
	struct sockaddr_in ssh1;
	struct sockaddr_in dst1;
	int e;
	char buf[1024];

	if (argc != 4) {
		printf("Usage: %s <anything> <base64(SshIp:port)> <base64(DstIp:porrt)>\n", argv[0]);
		return 1;
	}

	size_t ssh_len;
	unsigned char *p = base64_decode(argv[2], strlen(argv[2]), &ssh_len);

	size_t dst_len;
	unsigned char *p2 = base64_decode(argv[3], strlen(argv[3]), &dst_len);

	printf("%s\n", p);
	printf("%s\n", p2);


	return 0;

#if DEBUG == 0
	// demonize
	if (fork() > 0) {
		return 0;
	}
	setsid();
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	if (fork() > 0) {
		return 0;
	}
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
	{
		close (x);
	}
#endif

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
#if DEBUG == 1
			printf("read %d from ssh server\n", n); fflush(stdout);
#endif
			if (n <= 0) {
				perror("read(s)");
				return 0;
			}
			n = write(d, buf, n);
			fsync(d);
#if DEBUG == 1
			printf("wrote %d to remote\n", n); fflush(stdout);
#endif
		}
		if (FD_ISSET(d, &readfds)) {
			int n = read(d, buf, sizeof(buf));
#if DEBUG == 1
			printf("read %d from remote\n", n); fflush(stdout);
#endif
			if (n <= 0) {
				perror("read(d)");
				return 0;
			}
			n = write(s, buf, n);
			fsync(s);
#if DEBUG == 1
			printf("wrote %d to remote\n", n); fflush(stdout);
#endif
		}
	}

	return 0;
}


