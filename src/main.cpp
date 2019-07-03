#include <iostream>

#include "MinecraftNetworkCore.h"

int main()
{
	std::cout.sync_with_stdio(false);

	std::string nickname("BOT");
	std::string ip("127.0.0.1");
	std::string port("25565");
	Player* player = ConnectToServer(ip, port, nickname);
	clock_t lastPosChangeTime = clock();
	for (int i = 0; i < 1600 && player->connection.connectionState == Play; i++)
	{
		HandleGame(player);
	}
	DisconnectFromServer(player);
	delete player;

	return 1337;
}