#pragma once

#include <zlib.h>

#include "MinecraftNetworkDataTypes.h"

#define LOG_PACKETS_INFO
#define LOG_GAMEPLAY_INFO

void HandlePlayer(Player& player);

/* Gameplay functions */
void MovePlayer(Player& player, Minecraft_Double dx, Minecraft_Double dy, Minecraft_Double dz);
void RespawnPlayer(Player& player);

void RecvGamePackets(Player* player);
void DispatchGamePacket(Player& player);

/* Basic functions */
void ConnectToServer(Player& player, const std::string& nickname, const std::string& ip, const std::string& port);
void DisconnectFromServer(Player& player);

void SendGamePacket(DataBuffer& data, const TCPClient& tcpconnection, int compressionThreshold);
GamePacket* ParseGamePacket(char* packet, int compressionThreshold);

/* Clientbound packets*/
void JoinGamePacket(Player& player, GamePacket* packet); // 0x25 in protocol 404
void PlayerPositionAndLookPacket(Player& player, GamePacket* packet); // 0x32 in protocol 404
void KeepAlivePacket(Player& player, GamePacket* packet); // 0x21 in protocol 404
void TimeUpdatePacket(Player& player, GamePacket* packet); // 0x4A in protocol 404
void DisconnectPacket(Player& player, GamePacket* packet); // 0x1B in protocol 404

/* Serverbound packets */
void SendPlayerPosition(Player& player); // 0x10 in protcol 404
void SendPlayerPositionAndLook(Player& player); // 0x11 in protcol 404
void SendClientStatus(Player& player, Minecraft_Int actionID); // 0x03 in protcol 404
void SendClientSettings(Player& player, std::string locale, Minecraft_Byte viewDistance, Minecraft_Int chatMode,
						bool chatColors, Minecraft_UnsignedByte displayedSkinParts, Minecraft_Int mainHand); // 0x03 in protcol 404

void SendCompressedGamePacket(DataBuffer& data, const TCPClient& tcpconnection, int compressionThreshold);
void SendUncompressedGamePacket(DataBuffer& data, const TCPClient& tcpconnection);
GamePacket* ParseCompressedGamePacketHeader(char* packet, int compressionThreshold);
GamePacket* ParseUncompressedGamePacketHeader(char* packet);