#include "MinecraftNetworkCore.h"

void RecvAndDispatchGamePacket(Player& player)
{
	if (player.connection.connectionState != Play)
	{
		std::cout << player.generalInfo.nickname << ": Critical error! Called RecvAndDispatchGamePacket function for player with wrong connection state!" << std::endl;
		DisconnectFromServer(player);
		return;
	}
	int recvSize = player.connection.tcpconnection.RecvData(player.connection.receivingBuffer, MAX_PACKET_SIZE);
	if (recvSize <= 0 || recvSize == MAX_PACKET_SIZE) // read MAX_PACKET_SIZE comments
	{
		DisconnectFromServer(player);
		return;
	}
	GamePacket* packet = ParseGamePacket(player.connection.receivingBuffer, player.connection.compressionThreshold);
	//std::cout << player.generalInfo.nickname << ": Received packet with id: 0x" << std::hex << packet->packetID << std::dec << std::endl;
	switch (packet->packetID)
	{
	case 0x25: JoinGamePacket(player, packet); break;
	case 0x32: PlayerPositionAndLookPacket(player, packet); break;
	}
	delete packet;
}

void JoinGamePacket(Player& player, GamePacket* packet)
{
	player.gameplayInfo.entityID = packet->data.ReadInt();
	player.gameplayInfo.gamemode = packet->data.ReadUnsignedByte();
	player.gameplayInfo.dimension = packet->data.ReadInt();
	player.serverInfo.difficulty = packet->data.ReadUnsignedByte();
	player.serverInfo.maxPlayers = packet->data.ReadUnsignedByte();
	player.serverInfo.levelType = packet->data.ReadMinecraftString(16);
	bool reducedDebugInfo = packet->data.ReadBool();
#ifdef LOG_PACKETS_INFO
	std::cout << player.generalInfo.nickname << ": my entity id is: " << player.gameplayInfo.entityID << std::endl;
	std::cout << player.generalInfo.nickname << ": my gamemode is: " << (unsigned int)player.gameplayInfo.gamemode << std::endl;
	std::cout << player.generalInfo.nickname << ": i'm in dimension: " << player.gameplayInfo.dimension << std::endl;
	std::cout << player.generalInfo.nickname << ": server difficulty is: " << (unsigned int)player.serverInfo.difficulty << std::endl;
	std::cout << player.generalInfo.nickname << ": server max player is: " << (unsigned int)player.serverInfo.maxPlayers << std::endl;
	std::cout << player.generalInfo.nickname << ": server level type is: " << player.serverInfo.levelType << std::endl;
	std::cout << player.generalInfo.nickname << ": server reduced debug info is: " << reducedDebugInfo << std::endl;
#endif
}

void PlayerPositionAndLookPacket(Player& player, GamePacket* packet)
{
	player.connection.connectionState = Login;







	player.gameplayInfo.positionAndLook.x = packet->data.ReadDouble();
	player.gameplayInfo.positionAndLook.y = packet->data.ReadDouble();
	player.gameplayInfo.positionAndLook.z = packet->data.ReadDouble();
	player.gameplayInfo.positionAndLook.yaw = packet->data.ReadFloat();
	player.gameplayInfo.positionAndLook.pitch = packet->data.ReadFloat();
	Minecraft_Byte flags = packet->data.ReadByte();
	if ((flags & 0x01) || (flags & 0x02) || (flags & 0x04) || (flags & 0x08) || (flags & 0x10))
		std::cout << player.generalInfo.nickname << ": Error! Received relative position or look, don't know how to handle it!" << std::endl;
	Minecraft_Int teleportID = packet->data.ReadVarInt();
#ifdef LOG_PACKETS_INFO
	std::cout << player.generalInfo.nickname << ": Received Player postion and look! (" << player.gameplayInfo.positionAndLook.x
		<< ", " << player.gameplayInfo.positionAndLook.y << ", " << player.gameplayInfo.positionAndLook.z << "), (" << player.gameplayInfo.positionAndLook.yaw
		<< ", " << player.gameplayInfo.positionAndLook.pitch << ")" << std::endl;
#endif
	// teleport confirm
	DataBuffer teleportConfirm;
	teleportConfirm.AllocateBuffer(10);
	teleportConfirm.WriteVarInt(0x00);
	teleportConfirm.WriteVarInt(teleportID);
	SendGamePacket(teleportConfirm, player.connection.tcpconnection, player.connection.compressionThreshold);
}

void ConnectToServer(Player& player, const std::string& nickname, const std::string& ip, const std::string& port)
{
	player.generalInfo.nickname = nickname;
	if (player.connection.receivingBuffer != nullptr)
	{
		delete[] player.connection.receivingBuffer;
	}
	player.connection.receivingBuffer = new char[MAX_PACKET_SIZE];
	if (!player.connection.tcpconnection.ConnectToServer(ip, port))
	{
		player.connection.connectionState = Disconnected;
		return;
	}

	DataBuffer sendBuffer;
	// Send handshake packet
	sendBuffer.AllocateBuffer(5 + 255 + 2 + 5); // maximum size of data in this packet
	sendBuffer.WriteVarInt(0x00); // packet id
	sendBuffer.WriteVarInt(404); // protocol version(404 for minecraft 1.13.2)
	sendBuffer.WriteMinecraftString(ip, 255);
	sendBuffer.WriteUnsignedShort((unsigned short)strtoul(port.c_str(), nullptr, 10));
	sendBuffer.WriteVarInt(2); // next state(1 for status, 2 for login)
	player.connection.connectionState = Handshaking;

	SendGamePacket(sendBuffer, player.connection.tcpconnection, player.connection.compressionThreshold);

	// Send login start packet
	sendBuffer.AllocateBuffer(5 + 16); // maximum size of data in this packet
	sendBuffer.WriteVarInt(0x00); // packet id
	sendBuffer.WriteMinecraftString(nickname, 16);
	player.connection.connectionState = Login;

	SendGamePacket(sendBuffer, player.connection.tcpconnection, player.connection.compressionThreshold);

	int recvSize = player.connection.tcpconnection.RecvData(player.connection.receivingBuffer, MAX_PACKET_SIZE);
	if (recvSize <= 0)
	{
		DisconnectFromServer(player);
		return;
	}

	GamePacket* newPacket = ParseGamePacket(player.connection.receivingBuffer, player.connection.compressionThreshold);
	std::string uuid;
	switch (newPacket->packetID)
	{
	case 0x01: std::cout << player.generalInfo.nickname << ": Can't connect! Server requested encryption, but this client doesn't support it.\n \
							Try offline mode, if it doesn't help idk what to do. Turn compruter off and on again." << std::endl;
		DisconnectFromServer(player);
		return;
	case 0x02:
		uuid = newPacket->data.ReadMinecraftString(36);
		player.generalInfo.nickname = newPacket->data.ReadMinecraftString(16);
		std::cout << player.generalInfo.nickname << ": Login success! UUID: " << uuid << ", nickname: " << player.generalInfo.nickname << std::endl;
		player.connection.connectionState = Play;
		return;
	case 0x03:
		player.connection.compressionThreshold = newPacket->data.ReadVarInt();
		std::cout << player.generalInfo.nickname << ": Compression threshold set to " << player.connection.compressionThreshold << std::endl;
	}
	delete newPacket;

	recvSize = player.connection.tcpconnection.RecvData(player.connection.receivingBuffer, MAX_PACKET_SIZE);
	if (recvSize <= 0)
	{
		DisconnectFromServer(player);
		return;
	}

	newPacket = ParseGamePacket(player.connection.receivingBuffer, player.connection.compressionThreshold);

	if (newPacket->packetID == 0x02)
	{
		uuid = newPacket->data.ReadMinecraftString(36);
		player.generalInfo.nickname = newPacket->data.ReadMinecraftString(16);
		std::cout << player.generalInfo.nickname << ": Login success! UUID: " << uuid << ", nickname: " << player.generalInfo.nickname << std::endl;
		player.connection.connectionState = Play;
	}
	else
	{
		std::cout << player.generalInfo.nickname << ": Received wrong packet: "  << std::hex << newPacket->packetID << std::dec <<
			"! Login success(0x2) was expected " << std::endl;
		DisconnectFromServer(player);
	}
	delete newPacket;
}

void DisconnectFromServer(Player& player)
{
	if (player.connection.connectionState == Disconnected)
	{
		std::cout << player.generalInfo.nickname << ": Error while disconnecting from server! Already disconnected." << std::endl;
		return;
	}
	std::cout << player.generalInfo.nickname << ": Successfully disconnected from server." << std::endl;
	player.connection.connectionState = Disconnected;
	player.connection.tcpconnection.CloseConnection();
}

void SendGamePacket(DataBuffer& data, const TCPClient& tcpconnection, int compressionThreshold)
{
	/* if compression threshold < 0 then compression is disabled and packets have different format, than with it enabled */
	if (compressionThreshold < 0)
	{
		SendUncompressedGamePacket(data, tcpconnection);
	}
	else
	{
		SendCompressedGamePacket(data, tcpconnection, compressionThreshold);
	}
}

GamePacket* ParseGamePacket(char* packet, int compressionThreshold)
{
	/* if compression threshold < 0 then compression is disabled and packets have different format, than with it enabled */
	if (compressionThreshold < 0)
	{
		return ParseUncompressedGamePacketHeader(packet);
	}
	else
	{
		return ParseCompressedGamePacketHeader(packet, compressionThreshold);
	}
}

void SendCompressedGamePacket(DataBuffer& data, const TCPClient& tcpconnection, int compressionThreshold)
{
	DataBuffer packet;
	packet.AllocateBuffer(data.GetOffset() + 10 + 10);
	if(data.GetOffset() > compressionThreshold)
	{
		Minecraft_VarInt dataLength = CodeVarInt(data.GetOffset());
		Bytef* compressedData = new Bytef[data.GetOffset() + 10]; // in some cases compressedData may be bigger, than source data
		uLongf compressedDataLen = (uLongf)data.GetOffset() + 10;
		int res = compress(compressedData, &compressedDataLen, (const Bytef*)data.GetBuffer(), data.GetOffset());
		if (res != Z_OK)
			__debugbreak();// std::cout << "compress function returned with error code: " << res << std::endl;
		packet.WriteVarInt((Minecraft_Int)compressedDataLen + dataLength.sizeOfUsedBytes);
		packet.WriteVarInt(data.GetOffset());
		packet.WriteArrayOfBytes((char*)compressedData, (int)compressedDataLen);
		delete[] compressedData;
	}
	else
	{
		Minecraft_VarInt dataLength = CodeVarInt(0);
		packet.WriteVarInt(data.GetOffset() + dataLength.sizeOfUsedBytes);
		packet.WriteVarInt(0);
		packet.WriteArrayOfBytes(data.GetBuffer(), data.GetOffset());
	}
	tcpconnection.SendData(packet.GetBuffer(), packet.GetOffset());
}

void SendUncompressedGamePacket(DataBuffer& data, const TCPClient& tcpconnection)
{
	DataBuffer packet;
	packet.AllocateBuffer(data.GetOffset() + 5);
	packet.WriteVarInt(data.GetOffset());
	packet.WriteArrayOfBytes(data.GetBuffer(), data.GetOffset());
	tcpconnection.SendData(packet.GetBuffer(), packet.GetOffset());
}

GamePacket* ParseCompressedGamePacketHeader(char* packet, int compressionThreshold)
{
	int packetLengthSize;
	int dataLengthSize;
	Minecraft_Int packetLength = DecodeVarInt(packet, &packetLengthSize);
	Minecraft_Int dataLength = DecodeVarInt(&packet[packetLengthSize], &dataLengthSize);
	if (dataLength > 0 && dataLength < compressionThreshold)
		__debugbreak();
	GamePacket* returnPacket = new GamePacket;
	if (dataLength > 0) // packet was compressed
	{
		returnPacket->data.AllocateBuffer(dataLength);
		uLongf uLongfdataLength = (uLongf)dataLength;
		int res = uncompress((Bytef*)returnPacket->data.GetBuffer(), &uLongfdataLength,
							(const Bytef*)& packet[packetLengthSize + dataLengthSize], packetLength - dataLengthSize);
		if (res != Z_OK)
			__debugbreak();
	}
	else // dataLength equals 0-means packet wasn't compressed
	{
		returnPacket->data.AllocateBuffer(packetLength - dataLengthSize);
		returnPacket->data.WriteArrayOfBytes(&packet[packetLengthSize + dataLengthSize], packetLength - dataLengthSize);
	}
	returnPacket->data.SetOffset(0);
	returnPacket->packetID = returnPacket->data.ReadVarInt();
	return returnPacket;
}

GamePacket* ParseUncompressedGamePacketHeader(char* packet)
{
	int lengthSize;
	Minecraft_Int length = DecodeVarInt(packet, &lengthSize);
	GamePacket* returnPacket = new GamePacket;
	returnPacket->packetDataSize = length;
	returnPacket->data.AllocateBuffer(length);
	returnPacket->data.WriteArrayOfBytes(&packet[lengthSize], length);
	returnPacket->data.SetOffset(0);
	returnPacket->packetID = returnPacket->data.ReadVarInt();
	return returnPacket;
}


