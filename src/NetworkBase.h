#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>

//#define LOG_RECV_SEND

class TCPClient
{
private:
	WSAData wsaData;
	SOCKET sock;
public:
	TCPClient();
	~TCPClient();
	bool ConnectToServer(const std::string& ip, const std::string& port);
	int SendData(const char* data, int dataSize) const;
	int RecvData(char* buf, int bufSize) const;
	void CloseConnection();

};