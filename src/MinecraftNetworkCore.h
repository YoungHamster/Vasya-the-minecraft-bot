#pragma once

#include <zlib.h>

#include "MinecraftNetworkDataTypes.h"

struct PlayerPosAndLook
{
	Minecraft_Double x = 9999999999.0L;
	Minecraft_Double y;
	Minecraft_Double z;
	Minecraft_Float yaw;
	Minecraft_Float pitch;
};

struct PlayerGeneralInfo
{
	std::string nickname;
	Minecraft_UUID uuid;
};

struct PlayerGameplayInfo
{
	Minecraft_Int entityID;
	Minecraft_UnsignedByte gamemode;
	World world;
	std::vector<Mob> mobs;
	Minecraft_Int dimension;
	PlayerPosAndLook positionAndLook;
	Minecraft_Float health;
	Minecraft_Int food;
	Minecraft_Float foodSaturation;
};

struct ServerInfo
{
	Minecraft_UnsignedByte difficulty;
	Minecraft_UnsignedByte maxPlayers;
	std::string levelType;
	std::vector <PlayerInfo> playersInfo;
};

struct Player
{
	PlayerGeneralInfo generalInfo;
	PlayerGameplayInfo gameplayInfo;
	ServerInfo serverInfo;

	clock_t lastTimeSentPosition;
	bool spawned = false;

	Connection connection;
};

struct GamePacket
{
	Minecraft_Int packetDataSize;/* if packet is uncompressed it's equal to packet length,
									if packet is compressed it's equal to size of uncompressed data
									(if packet was compressed it's data length,
									if it wasn't then it's (packet length - size of data length field) */
	Minecraft_Int packetID;
	DataBuffer data;/* packet id(that is already written to packetID) and actual data */
};

void ConnectToServer(Player& player, const std::string& nickname, const std::string& ip, const std::string& port);
void DisconnectFromServer(Player& player);

void SendGamePacket(DataBuffer& data, const TCPClient& tcpconnection, int compressionThreshold);
GamePacket* ParseGamePacket(char* packet, int compressionThreshold);

void SendCompressedGamePacket(DataBuffer& data, const TCPClient& tcpconnection, int compressionThreshold);
void SendUncompressedGamePacket(DataBuffer& data, const TCPClient& tcpconnection);
GamePacket* ParseCompressedGamePacketHeader(char* packet, int compressionThreshold);
GamePacket* ParseUncompressedGamePacketHeader(char* packet);