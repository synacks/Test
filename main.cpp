#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ikcp.h"


int g_udpSock = -1;
int g_udpSvr = -1;

int output(const char *buf, int len, ikcpcb *kcp, void *user)
{
	sockaddr_in remote;
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr("127.0.0.1");
	remote.sin_port = htons(7777);
	ssize_t ret = sendto(g_udpSock, buf, len, 0, (sockaddr*)&remote, sizeof(remote));
	printf("send to udp sock %d bytes\n", (int)ret);
}

IUINT32 iclock()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return IUINT32(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int main()
{
	g_udpSvr = socket(AF_INET, SOCK_DGRAM, 0);
	fcntl(g_udpSvr, F_SETFL, fcntl(g_udpSvr, F_GETFL)|O_NONBLOCK);
	{
		sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		local.sin_port = htons(5555);
		bind(g_udpSvr, (sockaddr*)&local, sizeof(local));
	}

	g_udpSock = socket(AF_INET, SOCK_DGRAM, 0);
	fcntl(g_udpSock, F_SETFL, fcntl(g_udpSock, F_GETFL)|O_NONBLOCK);
	{
		sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = inet_addr("127.0.0.1");
		local.sin_port = htons(6666);
		bind(g_udpSock, (sockaddr*)&local, sizeof(local));
	}

	ikcpcb* k1 = ikcp_create(1, NULL);
	ikcp_setoutput(k1, output);



	while(1)
	{
		pollfd pfd[2];
		pfd[0].fd = g_udpSock;
		pfd[0].events = POLLIN;
		pfd[0].revents = 0;

		pfd[1].fd = g_udpSvr;
		pfd[1].events = POLLIN;
		pfd[1].revents = 0;

		int ret = poll(pfd, 2, 10);
		if(ret < 0)
		{
			if(errno == EINTR)
			{
				continue;
			}
			else
			{
				printf("poll failed, errno : %d\n", errno);
				break;
			}
		}

		ikcp_update(k1, iclock());

		if(ret > 0)
		{
			if(pfd[0].revents & POLLIN)
			{
				char buf[4096] = {0};
				ssize_t udpBytes = read(g_udpSock, buf, sizeof(buf));
				assert(udpBytes > 0);
				ikcp_input(k1, buf, udpBytes);
			}

			if(pfd[1].revents & POLLIN)
			{
				char buf[4096] = {0};
				ssize_t udpBytes = read(g_udpSvr, buf, sizeof(buf));
				assert(udpBytes > 0);
				ikcp_send(k1, buf, (int)udpBytes);
			}
		}
	}

	return 0;
}