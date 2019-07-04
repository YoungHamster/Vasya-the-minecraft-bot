#include <iostream>

#include "MinecraftNetworkCore.h"

int main()
{
	std::cout.sync_with_stdio(false);

	Player player;
	std::string nickname("Bot");
	std::string ip("127.0.0.1");
	std::string port("25565");
	bool success[200];
	for (int j = 0; j < 200; j++)
	{
		success[j] = false;
		ConnectToServer(player, nickname, ip, port);
		clock_t starttime = clock();
		int i;
		for (i = 0; i < 10000 && player.connection.connectionState == Play; i++)
		{
			RecvAndDispatchGamePacket(player);
		}
		if (player.connection.connectionState == Login)
		{
			success[j] = true;
		}
		std::cout << i << std::endl;
		std::cout << clock() - starttime << std::endl;
		if (player.connection.connectionState != Disconnected)
			DisconnectFromServer(player);
		delete[] player.connection.receivingBuffer;
		player.connection.receivingBuffer = nullptr;
		player.connection.compressionThreshold = -1;
	}
	int successCount = 0;
	for (int i = 0; i < 200; i++)
	{
		if (success[i])
			successCount += 1;
	}
	std::cout << "Success rate = " << (200.0f / (float)successCount) * 100 << ", successCount = " << successCount << std::endl;
	return 1337;
}