#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <chrono>
#include <intrin.h>
#include <vector>
#include <queue>
#include <mutex>

//#include <nbt.h>

#include "MinecraftDefinitionsAndEnums.h"
#include "NetworkBase.h"

/* IMPORTANT: minecraft integers are big_endian encoded except varints */

/*
	This is basic data types
*/

typedef int8_t		Minecraft_Byte;
typedef uint8_t		Minecraft_UnsignedByte;
typedef int16_t		Minecraft_Short;
typedef uint16_t	Minecraft_UnsignedShort;
typedef int32_t		Minecraft_Int;
typedef int64_t		Minecraft_Long;
typedef float		Minecraft_Float; //A single-precision 32-bit IEEE 754 floating point number
typedef double		Minecraft_Double; //A double-precision 64-bit IEEE 754 floating point number
typedef	std::string Minecraft_String;
typedef std::string Minecraft_Chat; // JSON max length 32767
typedef std::string Minecraft_Identifier; // https://wiki.vg/Protocol#Identifier max length 32767
struct				Minecraft_UUID
{
	char bytes[16];
	bool operator==(const Minecraft_UUID& operand)
	{
		if (*(uint64_t*)this->bytes == *(uint64_t*)operand.bytes &&
			*(uint64_t*)& this->bytes[sizeof(uint64_t)] == *(uint64_t*)& operand.bytes[sizeof(uint64_t)])
			return true;
		return false;
	}
};
struct				Minecraft_VarInt // Variable-length data encoding a two's complement signed 32-bit integer
{
	unsigned char bytes[5];//max size of varint
	int sizeOfUsedBytes = 0;
};
struct				Minecraft_VarLong
{
	unsigned char bytes[10];//max size of varlong
	int sizeOfUsedBytes = 0;
};
typedef char		Minecraft_Entity_Metadata;//don't care
typedef char		Minecraft_Slot;
typedef char		Minecraft_NBT_Tag; // should be first node in the tree
typedef uint64_t	Minecraft_Position; // An integer/block position: x (-33554432 to 33554431), y (-2048 to 2047), z (-33554432 to 33554431)
typedef uint8_t		Minecraft_Angle; // A rotation angle in steps of 1/256 of a full turn. Whether or not this is signed does not matter, since the resulting angles are the same.
typedef char Minecraft_Optional_X; // bla
typedef char Minecraft_Array_of_X; // bla
typedef char Minecraft_X_Enum; // bla
typedef char Minecraft_Byte_Array; // bla

struct UnpackedPosition
{
	Minecraft_Int x;
	Minecraft_Short y;
	Minecraft_Int z;
};

struct ChunkSection
{
	Minecraft_UnsignedByte bitsPerBlock; /*For bits per block <= 4, 4 bits are used to represent a block.
											For bits per block between 5 and 8, the given value is used.*/
	Minecraft_Int paletteLength;
	Minecraft_Int* palette = nullptr; // Mapping of block state IDs in the global palette to indices of this array
	Minecraft_Int dataArrayLength; // Number of longs in the following array
	Minecraft_Long* dataArray = nullptr;
	Minecraft_Byte blockLight[BLOCK_OR_SKY_LIGHT_SIZE]; // half byte per block
	Minecraft_Byte skyLight[BLOCK_OR_SKY_LIGHT_SIZE]; // only if in the Overworld; half byte per block
	~ChunkSection();
};

struct Chunk
{
	Minecraft_Int x;
	Minecraft_Int z;
	Minecraft_Int bitmask;
	Minecraft_Int numberOfSections;
	Minecraft_Int size; // size of chunk sections and optional biomes
	ChunkSection* sections = nullptr;
	Minecraft_Int numberOfBlockEntities;
	Minecraft_NBT_Tag* blockEntities = nullptr;
	~Chunk();
};

struct World
{
	std::vector<Chunk*> chunks;
	~World();
};

struct PlayerProperty
{
	std::string name;
	std::string value;
	bool isSigned;
	std::string signature; // Only if Is Signed is true
};

struct PlayerInfo
{
	Minecraft_UUID uuid;
	std::string name;
	std::vector<PlayerProperty> properties;
	Minecraft_Int gamemode;
	Minecraft_Int ping; // in milliseconds
	bool hasDisplayName;
	Minecraft_Chat displayName;
};

/*
	Functions for basic data types
*/

UnpackedPosition UnpackPosition(Minecraft_Position pos);
Minecraft_Position PackPosition(UnpackedPosition pos);

Minecraft_Int DecodeVarInt(const char* input, int* sizeOfUsedBytes = nullptr);
Minecraft_Long DecodeVarLong(const char* input, int* sizeOfUsedBytes = nullptr);

Minecraft_VarInt CodeVarInt(Minecraft_Int value, void* PlaceToWrite = nullptr);
Minecraft_VarLong CodeVarLong(Minecraft_Long value, void* PlaceToWrite = nullptr);

/* N is maximum number of characters in string defined by protocol documentation */
void ConvertStringToMinecraftString(std::string& str, int N);
/* N is maximum number of characters in string defined by protocol documentation */
std::string DecodeMinecraftString(const char* minecraftString, int N, int* sizeOfCodedString = nullptr);

void bitShiftRightVarInt(Minecraft_VarInt* value, int shift);
void bitShiftRightVarLong(Minecraft_VarLong* value, int shift);

/*
	This is complex data types, containing simple data types in them
*/

class DataBuffer
{
private:
	int bufferSize = 0;
	char* buffer = nullptr;
	int offset = 0;
public:
	~DataBuffer();
	void AllocateBuffer(int size);
	void SetBufferTo(char* data, int dataSize);
	void SetOffset(int newOffset);
	int GetOffset();
	void AddToOffset(int value);

	bool ReadBool();
	Minecraft_Int ReadInt();
	Minecraft_Int ReadVarInt();
	Minecraft_Long ReadLong();
	Minecraft_Long ReadVarLong();
	/* N is maximum number of characters in string defined by protocol documentation */
	std::string ReadMinecraftString(int N);
	Minecraft_Byte ReadByte();
	Minecraft_UnsignedByte ReadUnsignedByte();
	Minecraft_Short ReadShort();
	Minecraft_UnsignedShort ReadUnsignedShort();
	char* ReadArrayOfBytes(int arraySize);
	Minecraft_Float ReadFloat();
	Minecraft_Double ReadDouble();

	void WriteBool(bool value);
	void WriteInt(Minecraft_Int value);
	void WriteVarInt(Minecraft_Int value);
	void WriteLong(Minecraft_Long value);
	void WriteVarLong(Minecraft_Long value);
	/* N is maximum number of characters in string defined by protocol documentation */
	void WriteMinecraftString(std::string value, int N);
	void WriteByte(Minecraft_Byte value);
	void WriteUnsignedByte(Minecraft_UnsignedByte value);
	void WriteShort(Minecraft_Short value);
	void WriteUnsignedShort(Minecraft_UnsignedShort value);
	void WriteArrayOfBytes(char* bytes, int arraySize);
	void WriteDouble(Minecraft_Double value);
	void WriteFloat(Minecraft_Float value);

	char* GetBuffer();
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

class AsyncGamePacketQueue
{
private:
	std::mutex mutex;
	std::queue<GamePacket*> gamePacketsQueue;
public:
	void Queue(GamePacket* newPacket);
	GamePacket* Dequeue();
};

struct Connection
{
	TCPClient tcpconnection;
	Minecraft_Int protocolVersion;
	std::string serverAddress;/* String (255)	Hostname or IP, e.g. localhost or 127.0.0.1, that was used to connect.
								 The Notchian server does not use this information. */
	Minecraft_UnsignedShort serverPort = 25565;

	char* receivingBuffer = nullptr;
	AsyncGamePacketQueue packetQueue;


	ConnectionStates connectionState = Disconnected;
	int compressionThreshold = -1;
};

struct Mob
{
	Minecraft_Int entityID;
	Minecraft_UUID entityUUID;
	Minecraft_Int type;
	Minecraft_Double x;
	Minecraft_Double y;
	Minecraft_Double z;
	Minecraft_Angle yaw;
	Minecraft_Angle pitch;
	Minecraft_Angle headPitch;
	Minecraft_Short vx; // velocity x measured in 1/8000 part of block
	Minecraft_Short vy; // velocity y measured in 1/8000 part of block
	Minecraft_Short vz; // velocity z measured in 1/8000 part of block
	// There should also be entity metadata, but it's hard to parse, and i'll do it later
};

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

	clock_t lastTimeSentPosition = 0;
	bool spawned = false;

	Connection connection;
};