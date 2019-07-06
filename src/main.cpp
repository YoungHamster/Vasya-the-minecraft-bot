#include <iostream>
#include <thread>

#include "MinecraftNetworkCore.h"

int main()
{
	std::cout.sync_with_stdio(false);

	Player player;
	std::string nickname("Bot2");
	std::string ip("127.0.0.1");
	std::string port("25565");
	for (int j = 0; j < 10; j++)
	{
		ConnectToServer(player, nickname, ip, port);
		std::thread recvThread(RecvGamePackets, &player);
		clock_t starttime = clock();
		while(player.connection.connectionState == Play)
		{
			HandlePlayer(player);
		}
		std::cout << clock() - starttime << std::endl;
		recvThread.join();
		if (player.connection.connectionState != Disconnected)
			DisconnectFromServer(player);
		player.connection.compressionThreshold = -1;
		player.spawned = false;
		player.connection.packetQueue.ClearQueue();
	}
	return 1337;
}