#include "NetworkBase.h"

Address::Address() {}

Address::Address(sockaddr_in addr)
	: address(addr.sin_addr.s_addr), port(addr.sin_port) {}

Address::Address(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port)
	: address(htonl((a << 24) | (b << 16) | (c << 8) | d)), port(htons(port)) {}

Address::Address(unsigned int address, unsigned short port)
	: address(htonl(address)), port(htons(port)) {}

unsigned int Address::GetAddress() const
{
	return address;
}

sockaddr_in Address::GetSockaddr() const
{
	sockaddr_in addr;
	addr.sin_addr.s_addr = address;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	return addr;
}

unsigned char Address::GetA() const
{
	return address;
}

unsigned char Address::GetB() const
{
	return address >> 8;
}

unsigned char Address::GetC() const
{
	return address >> 16;
}

unsigned char Address::GetD() const
{
	return address >> 24;
}

unsigned short Address::GetPort() const
{
	return port;
}

bool Address::operator==(const Address& other) const
{
	if (this->address == other.address && this->port == other.port)
	{
		return true;
	}
	return false;
}

bool Address::operator!=(const Address& other) const
{
	return !(*this == other);
}

TCPClient::TCPClient()
{
}

TCPClient::~TCPClient()
{

}

bool TCPClient::ConnectToServer(std::string& ip, std::string& port)
{
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	memset(&hints, 0, sizeof(hints));

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		__debugbreak();
		return false;
	}

	// Resolve the server address and port
	iResult = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
	if (iResult != 0) {
		std::cout << "getaddrinfo failed with error: %d\n" << iResult << std::endl;
		WSACleanup();
		__debugbreak();
		return false;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		sock = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (sock == INVALID_SOCKET) {
			std::cout << "socket failed with error: %ld\n" << WSAGetLastError() << std::endl;
			WSACleanup();
			__debugbreak();
			return false;
		}

		// Connect to server.
		iResult = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(sock);
			sock = INVALID_SOCKET;
			__debugbreak();
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (sock == INVALID_SOCKET) {
		std::cout << "Unable to connect to server!\n" << std::endl;
		WSACleanup();
		return false;
	}
	return true;
	/*sockaddr_in address = addr.GetSockaddr();
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		__debugbreak();
		return false;
	}
	res = connect(sock, (sockaddr*)& address, sizeof(address));
	if (res == SOCKET_ERROR)
	{
		__debugbreak();
		return false;
	}
	return true;*/
}

int TCPClient::SendData(const char* data, int dataSize)
{
#ifndef LOG_RECV_SEND
	return send(sock, data, dataSize, NULL);
#endif
	int size = send(sock, data, dataSize, NULL);
	std::cout << "Sent " << size << " bytes." << std::endl;
	return size;
}

int TCPClient::RecvData(char* buf, int bufSize)
{
#ifndef LOG_RECV_SEND
	return recv(sock, buf, bufSize, NULL);
#endif
	int size = recv(sock, buf, bufSize, NULL);
	if (size <= 0) __debugbreak();
	std::cout << "Received " << size << " bytes." << std::endl;
	return size;
}

void TCPClient::CloseConnection()
{
	int res = shutdown(sock, SD_BOTH);
	if (res == SOCKET_ERROR)
		std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;

	// cleanup
	closesocket(sock);
	WSACleanup();
}
