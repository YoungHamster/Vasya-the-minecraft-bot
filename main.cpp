#include <iostream>

#include "MinecraftNetworkCore.h"

int main()
{
	//std::cout.sync_with_stdio(false);

	Player player;
	std::string nickname("Bot");
	std::string ip("127.0.0.1");
	std::string port("25565");
	for (int j = 0; j < 50; j++)
	{
		ConnectToServer(player, nickname, ip, port);
		for (int i = 0; i < 4000 && player.connection.connectionState == Play; i++)
		{
			RecvAndDispatchGamePacket(player);
		}
		if (player.connection.connectionState != Disconnected)
			DisconnectFromServer(player);
		delete[] player.connection.receivingBuffer;
		player.connection.receivingBuffer = nullptr;
		player.connection.compressionThreshold = -1;
	}
	return 1337;
}