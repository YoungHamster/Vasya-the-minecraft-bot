#pragma once

#include <zlib.h>

#include "MinecraftNetworkDataTypes.h"

#define RECVBUFSIZE 2097152
#define TICK_TIME_IN_MILLISECONDS 50

//#define LOG_PACKETS_HEADERS
#define LOG_PACKETS_DATA

struct PacketHeader // without compression
{
	Minecraft_Int length; //Length of packet data + length of the packet ID
	Minecraft_Int ID;
	int dataOffset;
};

struct CompressedPacket
{
	Minecraft_Int packLen; // Length of Data Length + compressed length of(Packet ID + Data)
	Minecraft_Int dataLen; // Length of uncompressed (Packet ID + Data) or 0
	Minecraft_Int ID;
	DataBuffer data;
	~CompressedPacket();
};

Player* ConnectToServer(std::string ip, std::string& port, std::string nickname);
void DisconnectFromServer(Player* player);


/* Use this functions only while connection is in Play state*/
void ProcessIncomingPacket(Player* player);
void HandleGame(Player* player);

/* IMPORTANT: packet, that all this functions get as an argument is a packet,
	that was previously received from server and parsed by ParseCompressedPacketHeaderAndUncompressPacket*/
/* Clientbound */
void PlayerInfoPacket(Player* player, CompressedPacket* packet);
void KeepAlivePacket(Player* player, CompressedPacket* packet);
void ChunkDataPacket(Player* player, CompressedPacket* packet);
void DisconnectPlayPacket(Player* player, CompressedPacket* packet);
void UpdateHealthPacket(Player* player, CompressedPacket* packet);
void EntityPropertiesPacket(Player* player, CompressedPacket* packet);
void PlayerPositionAndLookPacket(Player* player, CompressedPacket* packet);
void JoinGamePacket(Player* player, CompressedPacket* packet);
void SpawnPositionPacket(Player* player, CompressedPacket* packet);
void PluginMessagePacket(Player* player, CompressedPacket* packet);
void CombatEventPacket(Player* player, CompressedPacket* packet);
void SpawnMobPacket(Player* player, CompressedPacket* packet);
void StatisticsPacket(Player* player, CompressedPacket* packet);
void DestroyEntitiesPacket(Player* player, CompressedPacket* packet);
void ChangeGameStatePacket(Player* player, CompressedPacket* packet);

/* Serverbound */
// Client status packet with action id 0
void SendRespawnPacket(Player* player);
void SendClientSettingsPacket(Player* player);
void SendPlayerPacket(Player* player);
void SendPlayerPositionAndLookPacket(Player* player);
void SendPlayerPositionPacket(Player* player);
void SendPlayerLookPacket(Player* player);
// Client status packet with action id 1
void SendClientRequesStatsPacket(Player* player);

void SendCompressedGamePacket(DataBuffer& data, TCPClient& connection, int compressionThreshold);
void SendUncompressedGamePacket(DataBuffer& data, TCPClient& connection);

/* Return size of written data */
int WritePacketLenght(char* packet, Minecraft_Int packetLenght);
CompressedPacket* ParseCompressedPacketHeaderAndUncompressPacket(char* packet);