#include "sock352.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>


#define WINDOW_LENGTH 1
#define INIT_SEQ_NO_CLIENT 1000
#define INIT_SEQ_NO_SERVER 2000
//#define PACKET_SIZE 100
//#define PACKET_BUFFER_SIZE  (PACKET_SIZE - sizeof(sock352_pkt_hdr_t))
#define PACKET_BUFFER_SIZE 12000
#define PACKET_SIZE (PACKET_BUFFER_SIZE + sizeof(sock352_pkt_hdr_t))



int sequenceNum;

struct Packet {
	sock352_pkt_hdr_t header;
	unsigned char data[PACKET_BUFFER_SIZE];

};
typedef struct Packet * Packet_p;

struct goBackN {
	int N;
	Packet_p window[WINDOW_LENGTH];
	int sentDate[WINDOW_LENGTH];
	int fileSize;
#define PACKET_TIMEOUT_MICROSECONDS 200
#define PACKET_TIMEOUT_SECONDS 0
};
typedef struct goBackN goBackN;

struct Connection {
	int src_port;
	int dest_port;
	struct sockaddr_in addr_in;
};

static struct Connection connection;

void timestamp(){

	time_t ltime;
	ltime =time(NULL);
	printf("%s", asctime(localtime(&ltime)));
}

void printHeader(Packet_p p) {
	sock352_pkt_hdr_t h = p->header;

	//printf("\n\nPACKET SEQUENCE: %d\nACK NUMBER: %d\nPAYLOAD: %d\n", h.sequence_no, h.ack_no, h.payload_len);

	//printf("SYN FLAG: %d\nACK FLAG: %d\n\n\n", getSynFlag(p), getAckFlag(p));
}

int getAckFlag(Packet_p p) {

	int x = p->header.flags & SOCK352_ACK;
	int y = (p->header.flags & SOCK352_ACK) == SOCK352_ACK;

	return (p->header.flags & SOCK352_ACK) == SOCK352_ACK;
}

int getSynFlag(Packet_p p) {

	return (p->header.flags & SOCK352_SYN) == SOCK352_SYN;
}

int expectedResponse(Packet_p resPacket, uint64_t ack_no, int ackFlag, int synFlag) {

	return resPacket->header.ack_no == ack_no && getAckFlag(resPacket) == ackFlag && getSynFlag(resPacket) == synFlag;

}

Packet_p sendPacketAndReturnAckPacket(int fd, Packet_p p) {

	int bw = 0, br = 0;
	Packet_p ackPacket = calloc(1, sizeof(struct Packet));

	struct timeval tv;

	tv.tv_sec = 2; /* 30 Secs Timeout */
	tv.tv_usec = 200000;  // Not init'ing this can cause strange errors

	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));

	while (br <= 0) {
		bw = write(fd, p, sizeof(struct Packet));
		//printf("Bytes written: %d\n\n", bw);
		//printf("Wrote packet # %d\n", p->header.sequence_no);

		wait(1);
		br = read(fd, ackPacket, sizeof(struct Packet));

//		br = recvfrom(fd,ackPacket,sizeof(struct Packet),0,(struct sockaddr *)&cliaddr,&len);
		//printf("Attempted to read packet, bytes read = %d\n", br);
		if (br == 0) {
			//printf("Read failed, sleeping\n");
			sleep(2);
		}
		if (br > 0) {
			//printf("Successfully read packet number %d\n", ackPacket->header.sequence_no);
			return ackPacket;

		}

	}

	return 0;
}

int sendPacketAndWait(Packet_p p, int fd) {
	write(fd, p, sizeof(struct Packet));

	int bytes_received = 0;

	struct timeval tv;

	tv.tv_sec = 5; /* 30 Secs Timeout */
	tv.tv_usec = 200000;  // Not init'ing this can cause strange errors

	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));

	Packet_p responsePacket = calloc(1, sizeof(struct Packet));

	while (bytes_received <= 0) {
//		bytes_sent = recvfrom(fd,resonsePacket,sizeof(struct Packet),);
		bytes_received = read(fd, responsePacket, sizeof(struct Packet));
		//printf("BYTES_RECEIVED:\n");
		if (bytes_received == sizeof(struct Packet)) {
			if (responsePacket->header.ack_no == p->header.sequence_no + 1 && getSynFlag(responsePacket)
					&& getAckFlag(responsePacket)) {
				//printf("PACKET SENT\n");
				break;;

			}
		}
	}

	return 0;
}

struct sockaddr_in convertAddr(sockaddr_sock352_t *addr) {
	struct sockaddr_in addr_in;
	memset(&addr_in, 0, sizeof(addr_in));

	addr_in.sin_port = addr->sin_port;
	addr_in.sin_family = addr->sin_family;
	addr_in.sin_addr.s_addr = addr->sin_addr.s_addr;
	return addr_in;
}

extern int sock352_init(int udp_port) {
	if (udp_port < 0) {
		return SOCK352_FAILURE;
	}
	if (udp_port == 0) {
		connection.src_port = SOCK352_DEFAULT_UDP_PORT;
		calloc(1, sizeof(struct Packet));
		connection.dest_port = SOCK352_DEFAULT_UDP_PORT;

	} else {
		connection.src_port = udp_port;
		connection.dest_port = udp_port;
	}

	return SOCK352_SUCCESS;
}
extern int sock352_init2(int remote_port, int local_port) {

	connection.src_port = local_port;
	connection.dest_port = remote_port;

	return SOCK352_SUCCESS;
}

extern int sock352_socket(int domain, int type, int protocol) {
	if (domain != AF_CS352 || type != SOCK_STREAM || protocol != 0) {
		return SOCK352_FAILURE;
	}

	return socket(AF_INET, type, protocol);

//	return SOCK352_SUCCESS;
}

extern int sock352_bind(int fd, sockaddr_sock352_t *addr, socklen_t len) {
	__const struct sockaddr_in addr_in = convertAddr(addr);

	return bind(fd, (__CONST_SOCKADDR_ARG) &addr_in, len);
//	return SOCK352_SUCCESS;
}

extern int sock352_connect(int fd, sockaddr_sock352_t *addr, socklen_t len) {
	struct sockaddr_in addr_in = convertAddr(addr);
	int c = connect(fd, (__CONST_SOCKADDR_ARG) &addr_in, len);
// packet 0
	Packet_p reqPacket = calloc(1, sizeof(struct Packet));
	reqPacket->header.flags |= SOCK352_SYN;
	sequenceNum = INIT_SEQ_NO_CLIENT;
	reqPacket->header.sequence_no = sequenceNum;
	reqPacket->header.header_len = sizeof(sock352_pkt_hdr_t);
	reqPacket->header.version = SOCK352_VER_1;

//
//	reqPacket->header.flags |= SOCK352_ACK;
//	//printf("SYN %d\n", (reqPacket->header.flags & SOCK352_SYN) == SOCK352_SYN);
//	//printf("ACK %d\n", (reqPacket->header.flags & SOCK352_ACK) == SOCK352_ACK);
//
//	int a = getAckFlag(reqPacket);
//
//	int b = getSynFlag(reqPacket);
//
	printHeader(reqPacket);

	reqPacket->data[0] = 16;
	//printf("FIRST PACKET: %x\n", reqPacket->data[0]);

//	sendto(fd, cp0->data, 2, 0, (__CONST_SOCKADDR_ARG) &addr_in, len);
//	write(fd, cp0, sizeof(struct Packet));
//	sendPacketAndWait(reqPacket, fd);

	Packet_p resPacket = calloc(1, sizeof(struct Packet));
	int correctResponse = 0;
	while (correctResponse == 0) {
		memcpy(resPacket, sendPacketAndReturnAckPacket(fd, reqPacket), sizeof(struct Packet));
		correctResponse = expectedResponse(resPacket, reqPacket->header.sequence_no + 1, 1, 1);
		//printf("Correct response = %d\n", correctResponse);
	}
	Packet_p startPacket = calloc(1, sizeof(struct Packet));
	startPacket->header.flags = SOCK352_ACK;
	startPacket->header.sequence_no = ++sequenceNum;
	startPacket->header.ack_no = resPacket->header.sequence_no + 1;

	write(fd, startPacket, sizeof(struct Packet));
	//printf("Sent startPacket\n");
	//printf("CLIENT WRITE ERRNO: %d\n", errno);

//	//printf("%d  CONNECT NUMERROR: %d\n", c, errno);

	//printf("CLIENT HANDSHAKE SUCCESS\n");

	//printf("Accepted\n");

//	char a[10000];
//	//printf("Bytes in buffer after accept: %d\n",read(fd, a, 10000));

	return c;
//	return SOCK352_SUCCESS;
}

extern int sock352_listen(int fd, int n) {
	return listen(fd, n);
//	return SOCK352_SUCCESS;
}

extern int sock352_accept(int _fd, sockaddr_sock352_t *addr, int *len) {
	struct sockaddr_in addr_in = convertAddr(addr);

	struct Packet buffer[10];
	memset(buffer, 0, sizeof(struct Packet) * 10);

	int newfd = accept(_fd, (__SOCKADDR_ARG) &addr_in, len);

	Packet_p accPacket = calloc(1, sizeof(struct Packet));
	int bytes_read = read(newfd, accPacket, sizeof(struct Packet));

	//printf("Accept Packet:");
	printHeader(accPacket);

//	//printf("error: %d\n", errno);

	Packet_p estabPacket = calloc(1, sizeof(struct Packet));
	estabPacket->header.flags |= SOCK352_SYN;
	estabPacket->header.flags |= SOCK352_ACK;
	estabPacket->header.ack_no = accPacket->header.sequence_no + 1;
	estabPacket->header.sequence_no = INIT_SEQ_NO_SERVER;
	//printf("Trying to send estab packet:\n");
	printHeader(estabPacket);

	Packet_p startPacket = calloc(1, sizeof(struct Packet));
	int correctResponse = 0;

	while (correctResponse == 0) {

		memcpy(startPacket, sendPacketAndReturnAckPacket(newfd, estabPacket), sizeof(struct Packet));
		//printf("Sent packet # %d\n", estabPacket->header.sequence_no);
		correctResponse = expectedResponse(startPacket, estabPacket->header.sequence_no + 1, 1, 0);
		//printf("Correct response = %d\n", correctResponse);

		// TODO make client catch this somehow
	}

//	//printf("PACKET 0 data: %d\n", accPacket->data[0]);

	//printf("SERVER HANDSHAKE SUCCESS\n\n\n\n");
	sequenceNum = startPacket->header.sequence_no + 1;

	return newfd;
}

extern int sock352_close(int fd) {

	return close(fd);
//	return SOCK352_SUCCESS;
}

extern int sock352_read(int fd, void *buf, int count) {
	Packet_p pBuffer = calloc(1, sizeof(struct Packet) * WINDOW_LENGTH);
	int totalBytesRead = 0;
	//printf("Print bytes to read: %d\n", count);

	while (totalBytesRead < count) {
		//printf("TOTAL BYTES READ LOOP: %d\n", totalBytesRead);
		//printf("CURRENT EXPECTED SEQ NUMBER: %d\n\n\n", sequenceNum);
		int br = read(fd, pBuffer, sizeof(struct Packet));
		if(br == 0){
			//printf("Read 0 bytes\n");
			break; //TODO
		}
		//printf("SOCK352_READ PACKET READ: %d\n", br);
		//printf("Packet Sequence Number: %d\n", pBuffer->header.sequence_no);
		int seq_no = pBuffer->header.sequence_no;

		Packet_p ackPacket = calloc(1, sizeof(struct Packet));
		ackPacket->header.flags |= SOCK352_ACK;
		if (seq_no + 1 <= sequenceNum) {
			ackPacket->header.ack_no = sequenceNum - 1;
		} else {
			ackPacket->header.ack_no = sequenceNum++;

			memcpy(buf + totalBytesRead, pBuffer->data, pBuffer->header.payload_len);

			totalBytesRead += pBuffer->header.payload_len;
			printf("\nPayload read: %d\nBuffer header:\n", pBuffer->header.payload_len);
			timestamp();
			printHeader(pBuffer);

		}
		//printf("Sending ack packet with ack no: %d\n", ackPacket->header.ack_no);
		write(fd, ackPacket, sizeof(struct Packet));
		//printf("Sent ack packet with ack no: %d\n", ackPacket->header.ack_no);

	}
	//printf("TOTAL BYTES READ END: %d\n", totalBytesRead);

//	return read(fd, buf, count);
	return totalBytesRead;
}

extern int sock352_write(int fd, void *buf, int count) {
	int totalBytesWritten = 0;
	//printf("write byte count: %d\n\n", count);



	int numPackets = (count / PACKET_BUFFER_SIZE) + 1;
	if (count % PACKET_BUFFER_SIZE == 0) {
		numPackets--;
	}
	int i;
	Packet_p ackPacket = calloc(1, sizeof(struct Packet));


	for (i = 0; i < numPackets; i++) {
		Packet_p writePacket = calloc(1, sizeof(struct Packet));
		writePacket->header.sequence_no = ++sequenceNum;

		int size = (i == numPackets - 1) ? count % PACKET_BUFFER_SIZE  : PACKET_BUFFER_SIZE;

		writePacket->header.payload_len = size;
		//printf("Payload length: %d\n", size);


		memcpy(writePacket->data, (buf + i * PACKET_BUFFER_SIZE), size);
		int correctResponse = 0;

		while (correctResponse == 0) {

			memcpy(ackPacket, sendPacketAndReturnAckPacket(fd, writePacket), sizeof(struct Packet));
			printf("Sent packet # %d\n", writePacket->header.sequence_no);
			timestamp();
			//printf("RECEIVED RESPONSE PACKET # %d\n", ackPacket->header.ack_no);
			correctResponse = expectedResponse(ackPacket, writePacket->header.sequence_no, 1, 0);
			//printf(" is correct RESPONSE : %d\n\n", correctResponse);

			//printf("Correct response = %d\n", correctResponse);
		}

		free(writePacket);
		totalBytesWritten += size; // TODO check write for bytes written


	}

//	return write(fd, buf, count);
	return totalBytesWritten;
}
