#include "MinecraftNetworkCore.h"

Player* ConnectToServer(std::string ip, std::string& port, std::string nickname)
{
	Player* player = new Player();
	if (!player->connection.tcpconnection.ConnectToServer(ip, port))
		std::cout << "Cannot connect to server" << std::endl;

	player->generalInfo.nickname = nickname;

	char* buf = new char[RECVBUFSIZE];
	DataBuffer dataBuf;
	int bufSize = 0;
	memset(buf, 0, RECVBUFSIZE);
	{ // Handshaking
		player->connection.connectionState = Handshaking;

		dataBuf.AllocateBuffer(RECVBUFSIZE);

		dataBuf.WriteVarInt(0x00);
		player->connection.protocolVersion = 404;//1.13.2
		dataBuf.WriteVarInt(player->connection.protocolVersion);
		dataBuf.WriteMinecraftString(ip, 255);
		Minecraft_UnsignedShort serverPort = (Minecraft_UnsignedShort)strtoul(port.c_str(), NULL, 10);
		dataBuf.WriteUnsignedShort(serverPort);
		Minecraft_Int nextState = 2;// 2-login 1-status
		dataBuf.WriteVarInt(nextState);

		SendUncompressedGamePacket(dataBuf, player->connection.tcpconnection);
	}
	{ // Login
		player->connection.connectionState = Login;

		dataBuf.AllocateBuffer(RECVBUFSIZE);

		dataBuf.WriteVarInt(0x00);
		dataBuf.WriteMinecraftString(nickname, 16);

		SendUncompressedGamePacket(dataBuf, player->connection.tcpconnection);
	}

	int recvSize = player->connection.tcpconnection.RecvData(buf, RECVBUFSIZE);

	{
		dataBuf.AllocateBuffer(recvSize);
		dataBuf.WriteArrayOfBytes(buf, recvSize);
		dataBuf.SetOffset(0);

		Minecraft_Int packetLenght = dataBuf.ReadVarInt();
		Minecraft_Int packetID = dataBuf.ReadVarInt();

		/* From this moment all packets headers will be in "with compression" format */
		if (packetID == 0x03)
		{
			player->connection.compressionThreshold = dataBuf.ReadVarInt();
			std::cout << "Compression threshold: " << player->connection.compressionThreshold << std::endl;
		}
	}

	memset(buf, 0, recvSize);
	recvSize = player->connection.tcpconnection.RecvData(buf, RECVBUFSIZE);

	{ // Login success
		CompressedPacket* packet = ParseCompressedPacketHeaderAndUncompressPacket(buf);

		if (packet->ID == 0x02)
		{
			player->connection.connectionState = Play;
			std::cout << "Connected successfully!" << std::endl;
			std::string UUID = packet->data.ReadMinecraftString();
			std::string username = packet->data.ReadMinecraftString();
			std::cout << "UUID: " << UUID << ", username: " << username << std::endl;
		}
		else
		{
			player->connection.connectionState = Disconnected;
			player->connection.tcpconnection.CloseConnection();
			std::cout << "Can't login to server!" << std::endl;
		}
		delete packet;
	}
	delete[] buf;
	return player;
}

void DisconnectFromServer(Player* player)
{
	player->connection.tcpconnection.CloseConnection();
	player->connection.connectionState = Disconnected;
}

void ProcessIncomingPacket(Player* player)
{
	if (player->connection.connectionState != Play)
		__debugbreak();
	char* recvbuf = new char[RECVBUFSIZE];
	memset(recvbuf, 0, RECVBUFSIZE);
	int receivedSize = player->connection.tcpconnection.RecvData(recvbuf, RECVBUFSIZE);
	if (receivedSize <= 0)
	{
		player->connection.connectionState = Disconnected;
		delete[] recvbuf;
		return;
	}
	CompressedPacket* packet = ParseCompressedPacketHeaderAndUncompressPacket(recvbuf);
	if (packet->ID == 0x32 || packet->ID == 0x21 || packet->ID == 0x01)
	{
		std::cout << "That's it! packet id is" << packet->ID << std::endl;
	}
	switch (packet->ID)
	{
	case 0x3: SpawnMobPacket(player, packet); break;
	case 0x7: StatisticsPacket(player, packet); break;
	case 0x1B: DisconnectPlayPacket(player, packet); break;
	case 0x19: PluginMessagePacket(player, packet); break;
	case 0x20: ChangeGameStatePacket(player, packet); break;
	case 0x21: KeepAlivePacket(player, packet); break;
	case 0x22: ChunkDataPacket(player, packet); break;
	case 0x25: JoinGamePacket(player, packet); break;
	case 0x30: PlayerInfoPacket(player, packet); break;
	case 0x32: PlayerPositionAndLookPacket(player, packet); break;
	case 0x44: UpdateHealthPacket(player, packet); break;
	case 0x49: SpawnPositionPacket(player, packet); break;
	case 0x52: EntityPropertiesPacket(player, packet); break;
	}
	delete packet;
	delete[] recvbuf;
}

void HandleGame(Player* player)
{
	ProcessIncomingPacket(player);
	if (player->statisticsCounter >= 100)
	{
		SendClientRequesStatsPacket(player);	
		player->statisticsCounter = 0;
	}
	player->statisticsCounter += 1;
	if (player->gameplayInfo.positionAndLook.x != 9999999999.0L && clock() - player->lastTimeSentPosition >= TICK_TIME_IN_MILLISECONDS * 19)
	{
		player->lastTimeSentPosition = clock();
		SendPlayerPositionPacket(player);	
	}
}

void PlayerInfoPacket(Player* player, CompressedPacket* packet)
{
	Minecraft_Int action = packet->data.ReadVarInt();
	Minecraft_Int numberOfPlayers = packet->data.ReadVarInt();
	for (int i = 0; i < numberOfPlayers; i++)
	{
		Minecraft_UUID newUUID;
		memcpy(newUUID.bytes, packet->data.ReadArrayOfBytes(16), 16);
		Minecraft_Int numberOfProperties;
		Minecraft_Int newPlayerInfoNumber;
		Minecraft_Int newPlayerPropertyNumber;
		switch (action)
		{
		case 0:
			/* Sorry for this really long code lines, they should work fine */
			player->serverInfo.playersInfo.push_back(PlayerInfo());
			newPlayerInfoNumber = player->serverInfo.playersInfo.size() - 1;
			player->serverInfo.playersInfo.at(newPlayerInfoNumber).uuid = newUUID;
			player->serverInfo.playersInfo.at(newPlayerInfoNumber).name = packet->data.ReadMinecraftString();
			numberOfProperties = packet->data.ReadVarInt();
			for (int j = 0; j < numberOfProperties; j++)
			{
				player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.push_back(PlayerProperty());
				newPlayerPropertyNumber = player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.size() - 1;
				player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.at(newPlayerPropertyNumber).name = packet->data.ReadMinecraftString();
				player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.at(newPlayerPropertyNumber).value = packet->data.ReadMinecraftString();
				player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.at(newPlayerPropertyNumber).isSigned = packet->data.ReadBool();
				if(player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.at(newPlayerPropertyNumber).isSigned)
					player->serverInfo.playersInfo.at(newPlayerInfoNumber).properties.at(newPlayerPropertyNumber).signature = packet->data.ReadMinecraftString();
			}
			player->serverInfo.playersInfo.at(newPlayerInfoNumber).gamemode = packet->data.ReadVarInt();
			player->serverInfo.playersInfo.at(newPlayerInfoNumber).ping = packet->data.ReadVarInt();
			player->serverInfo.playersInfo.at(newPlayerInfoNumber).hasDisplayName = packet->data.ReadBool();
			if(player->serverInfo.playersInfo.at(newPlayerInfoNumber).hasDisplayName)
				player->serverInfo.playersInfo.at(newPlayerInfoNumber).displayName = packet->data.ReadMinecraftString();

#ifdef LOG_PACKETS_DATA
			std::cout << player->generalInfo.nickname << ": got info about player with nickname: " << player->serverInfo.playersInfo.at(newPlayerInfoNumber).name << std::endl;
#endif
			break;
		case 1: break;
		case 2: break;
		case 3: break;
		case 4:
			for (int j = 0; j < player->serverInfo.playersInfo.size(); j++)
			{
				if (player->serverInfo.playersInfo[j].uuid == newUUID)
					player->serverInfo.playersInfo.erase(player->serverInfo.playersInfo.begin() + j);
			}
			break;
		}
	}
}

void KeepAlivePacket(Player* player, CompressedPacket* packet)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": keeping alive" << std::endl;
#endif
	DataBuffer data;
	data.AllocateBuffer(5 + 8);//max size of varint(packetID) + long
	data.WriteVarInt(0x0E);
	data.WriteArrayOfBytes(packet->data.ReadArrayOfBytes(8), sizeof(Minecraft_Long));
	SendCompressedGamePacket(data, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void ChunkDataPacket(Player* player, CompressedPacket* packet)
{
	if (!player->spawned)
	{
		SendRespawnPacket(player);
		player->spawned = true;
	}
	Chunk* chunk = new Chunk;
	chunk->x = packet->data.ReadInt();
	chunk->z = packet->data.ReadInt();

	bool fullChunk = packet->data.ReadBool();

	chunk->bitmask = packet->data.ReadVarInt();
	chunk->size = packet->data.ReadVarInt();

	chunk->numberOfSections = 0;
	for (int i = 0; i < 16; i++)
	{
		uint16_t byte = 0x1;
		byte <<= i;
		if (chunk->bitmask & byte)
			chunk->numberOfSections += 1;
	}
	chunk->sections = new ChunkSection[chunk->numberOfSections];
	for (int i = 0; i < chunk->numberOfSections; i++)
	{
		chunk->sections[i].bitsPerBlock = packet->data.ReadUnsignedByte();
		chunk->sections[i].paletteLength = packet->data.ReadVarInt();
		chunk->sections[i].palette = new Minecraft_Int[chunk->sections[i].paletteLength];
		for (int j = 0; j < chunk->sections[i].paletteLength; j++)
		{
			chunk->sections[i].palette[j] = packet->data.ReadVarInt();
		}
		chunk->sections[i].dataArrayLength = packet->data.ReadVarInt();
		chunk->sections[i].dataArray = new Minecraft_Long[chunk->sections[i].dataArrayLength];
		memcpy(chunk->sections[i].dataArray,
			   packet->data.ReadArrayOfBytes(chunk->sections[i].dataArrayLength * sizeof(Minecraft_Long)),
			   chunk->sections[i].dataArrayLength * sizeof(Minecraft_Long));
		memcpy(chunk->sections[i].blockLight, packet->data.ReadArrayOfBytes(BLOCK_OR_SKY_LIGHT_SIZE), BLOCK_OR_SKY_LIGHT_SIZE);
		if (player->gameplayInfo.dimension == Overworld)
			memcpy(chunk->sections[i].skyLight, packet->data.ReadArrayOfBytes(BLOCK_OR_SKY_LIGHT_SIZE), BLOCK_OR_SKY_LIGHT_SIZE);
	}
	if (fullChunk)
	{
		// TODO: parse biomes data
		packet->data.AddToOffset(256);
	}

	Minecraft_Int numberOfBlockEntities = packet->data.ReadVarInt();
	Minecraft_NBT_Tag* blockEntities; // All block entities in the chunk. Use the x, y, and z tags in the NBT to determine their positions
	// For now i'll just ignore it
	// TODO: process block entities(array of NBT tags, see https://wiki.vg/NBT)

	player->gameplayInfo.world.chunks.push_back(chunk);
}

void DisconnectPlayPacket(Player* player, CompressedPacket* packet)
{
	player->connection.connectionState = Disconnected;
	player->connection.tcpconnection.CloseConnection();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": disconnected with reason: " << packet->data.ReadMinecraftString() << std::endl;
#endif
}

void UpdateHealthPacket(Player* player, CompressedPacket* packet)
{
	player->gameplayInfo.health = packet->data.ReadFloat();
	player->gameplayInfo.food = packet->data.ReadVarInt();
	player->gameplayInfo.foodSaturation = packet->data.ReadFloat();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": updateHealthPacket: health: " << player->gameplayInfo.health << ", food: " << player->gameplayInfo.food << ", food saturation: " << player->gameplayInfo.foodSaturation << std::endl;
#endif
	if (player->gameplayInfo.health <= 0.0f)
	{
		std::cout << player->generalInfo.nickname << ": I'm dead." << std::endl;
		SendRespawnPacket(player);
	}
}

void EntityPropertiesPacket(Player* player, CompressedPacket* packet)
{
	Minecraft_Int entityID = packet->data.ReadVarInt();
	Minecraft_Int numberOfProperties = packet->data.ReadVarInt();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": received " << numberOfProperties << " properties of entity with ID: " << entityID << std::endl;
#endif
	for (int i = 0; i < numberOfProperties; i++)
	{
		std::string key = packet->data.ReadMinecraftString();
		Minecraft_Double value = packet->data.ReadDouble();
		Minecraft_Int numberOfModifiers = packet->data.ReadVarInt();
#ifdef LOG_PACKETS_DATA
		std::cout << player->generalInfo.nickname << ": entity " << entityID << " " << key << ": " << value << ", " << numberOfModifiers << " modifiers." << std::endl;
#endif
		for (int j = 0; j < numberOfModifiers; j++)
		{
			Minecraft_UUID uuid;
			memcpy(uuid.bytes, packet->data.ReadArrayOfBytes(16), 16);
			Minecraft_Double amount = packet->data.ReadDouble();
			Minecraft_Byte operation = packet->data.ReadByte();
		}
	}
}

void PlayerPositionAndLookPacket(Player* player, CompressedPacket* packet)
{
	Minecraft_Double x = packet->data.ReadDouble();
	Minecraft_Double y = packet->data.ReadDouble();
	Minecraft_Double z = packet->data.ReadDouble();
	Minecraft_Float yaw = packet->data.ReadFloat();
	Minecraft_Float pitch = packet->data.ReadFloat();
	Minecraft_Byte flags = packet->data.ReadByte();
	Minecraft_Int teleportID = packet->data.ReadVarInt();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": position and look: x: " << x << ", y: " << y << ", z: " << z << ", yaw: " << yaw << ", pitch: " << pitch << std::endl;
#endif
	if ((flags & 0x01) || (flags & 0x02) || (flags & 0x04) || (flags & 0x08) || (flags & 0x10))
		std::cout << player->generalInfo.nickname << ": got realtive position? WTF? There shouldn't be any" << std::endl;
	player->gameplayInfo.positionAndLook.x = x;
	player->gameplayInfo.positionAndLook.y = y;
	player->gameplayInfo.positionAndLook.z = z;
	player->gameplayInfo.positionAndLook.yaw = yaw;
	player->gameplayInfo.positionAndLook.pitch = pitch;
	// Now confirm teleport id(it's required by protocol)
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": confirming teleport id" << std::endl;
#endif
	DataBuffer teleportConfirm;
	teleportConfirm.AllocateBuffer(10); // max size of 2 varints
	teleportConfirm.WriteVarInt(0x00);
	teleportConfirm.WriteVarInt(teleportID);
	SendCompressedGamePacket(teleportConfirm, player->connection.tcpconnection, player->connection.compressionThreshold);
	SendPlayerPositionAndLookPacket(player);
}

void JoinGamePacket(Player* player, CompressedPacket* packet)
{
	player->gameplayInfo.entityID = packet->data.ReadInt();
	player->gameplayInfo.gamemode = packet->data.ReadUnsignedByte();
	player->gameplayInfo.dimension = packet->data.ReadInt();
	player->serverInfo.difficulty = packet->data.ReadUnsignedByte();
	player->serverInfo.maxPlayers = packet->data.ReadUnsignedByte();
	player->serverInfo.levelType = packet->data.ReadMinecraftString();
	bool reducedDebugInfo = packet->data.ReadBool();

	std::cout << "Entity id of " << player->generalInfo.nickname << " is: " << player->gameplayInfo.entityID << std::endl;
	std::cout << "Gamemode of " << player->generalInfo.nickname << " is: " << (unsigned int)player->gameplayInfo.gamemode << std::endl;
	std::cout << "Dimension of " << player->generalInfo.nickname << " is: " << player->gameplayInfo.dimension << std::endl;
	std::cout << "Server difficulty is: " << (unsigned int)player->serverInfo.difficulty << std::endl;
	std::cout << "Server max players is: " << (unsigned int)player->serverInfo.maxPlayers << std::endl;
	std::cout << "Server levelType is: " << player->serverInfo.levelType << std::endl;
	std::cout << "Server reducedDebugInfo is: " << reducedDebugInfo << std::endl;

	SendClientSettingsPacket(player); // send settings to server
}

void SpawnPositionPacket(Player* player, CompressedPacket* packet)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": got player spawn position!" << std::endl;
#endif
}

void PluginMessagePacket(Player* player, CompressedPacket* packet)
{
	std::string channel = packet->data.ReadMinecraftString();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": got plugin message packet from channel: " << channel << std::endl;
	if (channel == "minecraft:brand")
	{
		std::cout << player->generalInfo.nickname << ": server brand is: " << packet->data.ReadMinecraftString() << std::endl;
		DataBuffer respone;
		respone.AllocateBuffer(100); // more that i'll probably need for this packet, don't want to count exact max size
		respone.WriteVarInt(0x0a);
		respone.WriteMinecraftString("minecraft:brand", 32767);
		respone.WriteMinecraftString("Vasya-bot", 16);
		SendCompressedGamePacket(respone, player->connection.tcpconnection, player->connection.compressionThreshold);
	}
#endif
}

void CombatEventPacket(Player* player, CompressedPacket* packet)
{

}

void SpawnMobPacket(Player* player, CompressedPacket* packet)
{
	player->gameplayInfo.mobs.push_back(Mob());
	int newMobNumber = player->gameplayInfo.mobs.size() - 1;
	player->gameplayInfo.mobs[newMobNumber].entityID = packet->data.ReadVarInt();
	memcpy(player->gameplayInfo.mobs[newMobNumber].entityUUID.bytes, packet->data.ReadArrayOfBytes(16), 16);
	player->gameplayInfo.mobs[newMobNumber].type = packet->data.ReadVarInt();
	player->gameplayInfo.mobs[newMobNumber].x = packet->data.ReadDouble();
	player->gameplayInfo.mobs[newMobNumber].y = packet->data.ReadDouble();
	player->gameplayInfo.mobs[newMobNumber].z = packet->data.ReadDouble();
	player->gameplayInfo.mobs[newMobNumber].yaw = packet->data.ReadUnsignedByte();
	player->gameplayInfo.mobs[newMobNumber].pitch = packet->data.ReadUnsignedByte();
	player->gameplayInfo.mobs[newMobNumber].headPitch = packet->data.ReadUnsignedByte();
	player->gameplayInfo.mobs[newMobNumber].vx = packet->data.ReadShort();
	player->gameplayInfo.mobs[newMobNumber].vy = packet->data.ReadShort();
	player->gameplayInfo.mobs[newMobNumber].vz = packet->data.ReadShort();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": received new mob packet. \n New mob x: " << player->gameplayInfo.mobs[newMobNumber].x
		<< ", y: " << player->gameplayInfo.mobs[newMobNumber].y << ", z: " << player->gameplayInfo.mobs[newMobNumber].y << std::endl;
#endif
}

void StatisticsPacket(Player* player, CompressedPacket* packet)
{
	Minecraft_Int count = packet->data.ReadVarInt();
	if (count == 0)
	{
#ifdef LOG_PACKETS_DATA
		std::cout << player->generalInfo.nickname << ": received statistics respone. No statistics." << std::endl;
#endif
		return;
	}
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": received statistics: " << std::endl;
#endif
	for (int i = 0; i < count; i++)
	{
		Minecraft_Int categoryID = packet->data.ReadVarInt();
		Minecraft_Int statisticID = packet->data.ReadVarInt();
#ifdef LOG_PACKETS_DATA
		std::cout << player->generalInfo.nickname << ": statistic castegoryID: " << categoryID << ", statisticID: " << statisticID << std::endl;
#endif
	}
	Minecraft_Int value = packet->data.ReadVarInt();
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": statisctics value: " << value << std::endl;
#endif
}

void ChangeGameStatePacket(Player* player, CompressedPacket* packet)
{
	Minecraft_Int reason = packet->data.ReadVarInt();
	Minecraft_Float value = packet->data.ReadFloat();

#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": change game state packet: reason: " << reason << ", value: " << value << std::endl;
#endif
}

//void DestroyEntitiesPacket(Player* player, CompressedPacket* packet)
//{
//	Minecraft_Int count = packet->data.ReadVarInt();
//	for (int i = 0; i < count; i++)
//	{
//		Minecraft_Int entityID = packet->data.ReadVarInt();
//		for(int i = 0; i < player->gameplayInfo.entities)
//	}
//}

void SendRespawnPacket(Player* player)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": I'm spawning!" << std::endl;
#endif
	DataBuffer packet;
	packet.AllocateBuffer(10); // maximum size of 2 varints
	packet.WriteVarInt(0x03); // Client status packet
	packet.WriteVarInt(0); // Perform respawn action id
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void SendClientSettingsPacket(Player* player)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": sending client settings to server" << std::endl;
#endif
	DataBuffer packet;
	packet.AllocateBuffer(39); // maximum size of everything below
	packet.WriteVarInt(0x04); // Client settings packet id
	// for now fill with default values
	packet.WriteMinecraftString("en_GB", 16); // locale
	packet.WriteByte(10); // render distance in chunks
	packet.WriteVarInt(0); // chat mode 0: enabled, 1: commands only, 2: hidden
	packet.WriteBool(false); // chat colors
	packet.WriteUnsignedByte(0); // displayed skin parts bit mask
	packet.WriteVarInt(1); // main hand 0: Left, 1: Right
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void SendPlayerPacket(Player* player)
{
	DataBuffer packet;
	packet.AllocateBuffer(6); // max size of varint and boolean
	packet.WriteVarInt(0x0f);
	packet.WriteBool(true); // is player on ground(probably server will kick you if you send lies)
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": sending player packet(0x0f)." << std::endl;
#endif
}

void SendPlayerPositionAndLookPacket(Player* player)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": sending player position and look on server: x: " << player->gameplayInfo.positionAndLook.x
		<< ", y: " << player->gameplayInfo.positionAndLook.y << ", z: " << player->gameplayInfo.positionAndLook.z
		<< ", yaw: " << player->gameplayInfo.positionAndLook.yaw << ", pitch: " << player->gameplayInfo.positionAndLook.pitch << std::endl;
#endif
	DataBuffer packet;
	packet.AllocateBuffer(38); // maximum posible size of data in this packet
	packet.WriteVarInt(0x11);
	packet.WriteDouble(player->gameplayInfo.positionAndLook.x);
	packet.WriteDouble(player->gameplayInfo.positionAndLook.y);
	packet.WriteDouble(player->gameplayInfo.positionAndLook.z);
	packet.WriteFloat(player->gameplayInfo.positionAndLook.yaw);
	packet.WriteFloat(player->gameplayInfo.positionAndLook.pitch);
	packet.WriteBool(true); // is player on ground(probably server will kick you if you send lies)
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void SendPlayerPositionPacket(Player* player)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": sending player position on server: x: " << player->gameplayInfo.positionAndLook.x
		<< ", y: " << player->gameplayInfo.positionAndLook.y << ", z: " << player->gameplayInfo.positionAndLook.z << std::endl;
#endif
	DataBuffer packet;
	packet.AllocateBuffer(30); // maximum posible size of data in this packet
	packet.WriteVarInt(0x10);
	packet.WriteDouble(player->gameplayInfo.positionAndLook.x);
	packet.WriteDouble(player->gameplayInfo.positionAndLook.y);
	packet.WriteDouble(player->gameplayInfo.positionAndLook.z);
	packet.WriteBool(true); // is player on ground(probably server will kick you if you send lies)
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void SendPlayerLookPacket(Player* player)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": sending player look on server: x: " << player->gameplayInfo.positionAndLook.x
		<< ", y: " << player->gameplayInfo.positionAndLook.y << ", z: " << player->gameplayInfo.positionAndLook.z
		<< ", yaw: " << player->gameplayInfo.positionAndLook.yaw << ", pitch: " << player->gameplayInfo.positionAndLook.pitch << std::endl;
#endif
	DataBuffer packet;
	packet.AllocateBuffer(14); // maximum posible size of data in this packet
	packet.WriteVarInt(0x12);
	packet.WriteFloat(player->gameplayInfo.positionAndLook.yaw);
	packet.WriteFloat(player->gameplayInfo.positionAndLook.pitch);
	packet.WriteBool(true); // is player on ground(probably server will kick you if you send lies)
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void SendClientRequesStatsPacket(Player* player)
{
#ifdef LOG_PACKETS_DATA
	std::cout << player->generalInfo.nickname << ": sent client status request on server." << std::endl;
#endif
	DataBuffer packet;
	packet.AllocateBuffer(10); // max size of 2 varints
	packet.WriteVarInt(0x03);
	packet.WriteVarInt(1);
	SendCompressedGamePacket(packet, player->connection.tcpconnection, player->connection.compressionThreshold);
}

void SendCompressedGamePacket(DataBuffer& data, TCPClient& connection, int compressionThreshold)
{
	DataBuffer packet;
	packet.AllocateBuffer(data.GetOffset() + 15 + 20);// 15-maximum size of 3 VarInts packetid, data and packet lenght

	char* compressedData;
	Minecraft_Int packetLength;
	Minecraft_Int dataLength;
	uLongf comprDatLen;
	if (data.GetOffset() > compressionThreshold)
	{
		dataLength = data.GetOffset();
		comprDatLen = data.GetOffset() + 20; /* 20 is random additional size, i'm not sure, 
												but i think sowhere was mentioned, that compressed data can be bigger,
												than uncompressed in some cases */
		compressedData = new char[comprDatLen];
		memset(compressedData, 0, comprDatLen);
		int res = compress((Bytef*)compressedData, &comprDatLen, (Bytef*)data.GetBuffer(), data.GetOffset());
		if (res != Z_OK) __debugbreak();
		packetLength = (Minecraft_Int)comprDatLen;
	}
	else
	{
		dataLength = 0;
		comprDatLen = data.GetOffset();
		compressedData = data.GetBuffer();
	}
	Minecraft_VarInt datLen = CodeVarInt(dataLength);
	packetLength = comprDatLen + datLen.sizeOfUsedBytes;
	packet.WriteVarInt(packetLength);
	packet.WriteVarInt(dataLength);
	packet.WriteArrayOfBytes(compressedData, comprDatLen);
	connection.SendData(packet.GetBuffer(), packet.GetOffset());

#ifdef LOG_PACKETS_HEADERS
	data.SetOffset(0);
	std::cout << "Sent: packet id: 0x" << std::hex << data.ReadVarInt() << std::dec << ", packet lenght: " << packetLength << ", data lenght: " << dataLength << std::endl;
#endif

	if (dataLength > 0) delete[] compressedData;
}

void SendUncompressedGamePacket(DataBuffer& data, TCPClient& connection)
{
	DataBuffer packet;
	packet.AllocateBuffer(data.GetOffset() + 5);// 5-maximum size of 1 varint(packetSize)
	packet.WriteVarInt(data.GetOffset());
	packet.WriteArrayOfBytes(data.GetBuffer(), data.GetOffset());
	connection.SendData(packet.GetBuffer(), packet.GetOffset());
}

int WritePacketLenght(char* packet, Minecraft_Int dataSize)
{
	Minecraft_Int packetLenght = dataSize;
	Minecraft_VarInt packLen = CodeVarInt(packetLenght);
	memcpy(packet, packLen.bytes, packLen.sizeOfUsedBytes);
	return packLen.sizeOfUsedBytes;
}

CompressedPacket* ParseCompressedPacketHeaderAndUncompressPacket(char* packet)
{

	int packLenSize;
	int datLenSize;
	CompressedPacket* pack = new CompressedPacket;
	pack->packLen = DecodeVarInt(packet, &packLenSize);
	pack->dataLen = DecodeVarInt(&packet[packLenSize], &datLenSize);
	if (pack->dataLen == 0)
	{
		pack->data.AllocateBuffer(pack->packLen - datLenSize);
		pack->data.WriteArrayOfBytes(packet + packLenSize + datLenSize, pack->packLen - datLenSize);
		pack->data.SetOffset(0);
	}
	else
	{
		char* uncompressedData = new char[pack->dataLen];
		uLongf uncompressedDataSize = (uLongf)pack->dataLen;
		memset(uncompressedData, 0, uncompressedDataSize);
		int res = uncompress((Bytef*)uncompressedData, &uncompressedDataSize, (Bytef*)& packet[packLenSize + datLenSize], (pack->packLen - datLenSize));
		if (res != Z_OK)
			__debugbreak();// std::cout << "CRITICAL ERRORORRROROROROORORO!!!!! Function uncompress from zlib failed with error code: " << res << std::endl;
		pack->data.SetBufferTo(uncompressedData, uncompressedDataSize);
	}
	pack->ID = pack->data.ReadVarInt();
#ifdef LOG_PACKETS_HEADERS
	//if(pack->ID != 0x50 && pack->ID != 0x4d && pack->ID != 0x4a && pack->ID != 0x41 && pack->ID != 0x3f && pack->ID != 0x39 && pack->ID != 0x29 && pack->ID != 0x28 && pack->ID != 0x22 && pack->ID != 0x0b && pack->ID != 0x07 && pack->ID != 0x3)
	//if (pack->ID != 0x50 && pack->ID != 0x4d && pack->ID != 0x4a && pack->ID != 0x41 && pack->ID != 0x3f && pack->ID != 0x39 && pack->ID != 0x29 && pack->ID != 0x28 && pack->ID != 0x22 && pack->ID != 0x07 && pack->ID != 0x3)
	std::cout << "Received: packet id: 0x" << std::hex << pack->ID << std::dec << ", packet lenght: " << pack->packLen << ", data lenght: " << pack->dataLen << std::endl;
#endif
	return pack;
}

CompressedPacket::~CompressedPacket()
{	
}

Player::~Player()
{
}
