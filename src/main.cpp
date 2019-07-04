#include <iostream>
#include <thread>

#include "MinecraftNetworkCore.h"

int main()
{
	std::cout.sync_with_stdio(false);

	Player player;
	std::string nickname("Bot");
	std::string ip("127.0.0.1");
	std::string port("25565");
	bool success[50];
	for (int j = 0; j < 50; j++)
	{
		success[j] = false;
		ConnectToServer(player, nickname, ip, port);
		std::thread recvThread(RecvGamePacket, &player);
		clock_t starttime = clock();
		while(player.connection.connectionState == Play)
		{
			HandlePlayer(player);
		}
		if (player.connection.connectionState == Login)
		{
			success[j] = true;
		}
		std::cout << clock() - starttime << std::endl;
		recvThread.join();
		if (player.connection.connectionState != Disconnected)
			DisconnectFromServer(player);
		delete[] player.connection.receivingBuffer;
		player.connection.receivingBuffer = nullptr;
		player.connection.compressionThreshold = -1;
	}
	int successCount = 0;
	for (int i = 0; i < 50; i++)
	{
		if (success[i])
			successCount += 1;
	}
	std::cout << "Success rate = " << ((float)successCount / 50.0f) * 100.0f << ", successCount = " << successCount << std::endl;
	return 1337;
}