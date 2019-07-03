

/*
Attribution to #mcdevs
*/


#include "MinecraftNetworkDataTypes.h"

UnpackedPosition UnpackPosition(Minecraft_Position pos)
{
	UnpackedPosition unppos;
	uint64_t* hackpos = (uint64_t*)& pos;
	unppos.x = *hackpos >> 38;
	unppos.y = (*hackpos >> 26) & 0xFFF;
	unppos.z = *hackpos << 38 >> 38;
	return unppos;
}

Minecraft_Position PackPosition(UnpackedPosition pos)
{
	Minecraft_Position ppos = ((((int64_t)pos.x & 0x3FFFFFF) << 38) | (((int64_t)pos.y & 0xFFF) << 26) | (((int64_t)pos.z & 0x3FFFFFF)));
	return ppos;
}

Minecraft_Int DecodeVarInt(const char* input, int* sizeOfUsedBytes)
{
	int numRead = 0;
	Minecraft_Int result = 0;
	Minecraft_Byte read;
	do {
		read = input[numRead];
		Minecraft_Int value = (read & 0b01111111);
		result |= (value << (7 * numRead));

		numRead++;
		if (numRead > 5) {
			__debugbreak();//throw new RuntimeException("VarInt is too big");
		}
	} while ((read & 0b10000000) != 0);
	if (sizeOfUsedBytes != nullptr)
	{
		*sizeOfUsedBytes = numRead;
	}
	return result;
}

Minecraft_Long DecodeVarLong(const char* input, int* sizeOfUsedBytes)
{
	int numRead = 0;
	Minecraft_Long result = 0;
	Minecraft_Byte read;
	do {
		read = input[numRead];
		Minecraft_Long value = (read & 0b01111111);
		result |= (value << (7 * numRead));

		numRead++;
		if (numRead > 10) {
			__debugbreak();//throw new RuntimeException("VarLong is too big");
		}
	} while ((read & 0b10000000) != 0);
	if (sizeOfUsedBytes != nullptr) * sizeOfUsedBytes = numRead;
	return result;
}

Minecraft_VarInt CodeVarInt(Minecraft_Int value, void* PlaceToWrite)
{
	int numWrite = 0;
	Minecraft_VarInt result;
	do {
		Minecraft_Byte temp = (Minecraft_Byte)(value & 0b01111111);
		// Note: >>> means that the sign bit is shifted with the rest of the number rather than being left alone
		//value >>>= 7;
		uint32_t* hackValue = (uint32_t*)& value;
		*hackValue >>= 7;
		if (value != 0) {
			temp |= 0b10000000;
		}
		result.bytes[numWrite] = temp;
		numWrite++;
	} while (value != 0);
	result.sizeOfUsedBytes = numWrite;
	if (PlaceToWrite != nullptr) memcpy(PlaceToWrite, result.bytes, result.sizeOfUsedBytes);
	return result;
}

Minecraft_VarLong CodeVarLong(Minecraft_Long value, void* PlaceToWrite)
{
	int numWrite = 0;
	Minecraft_VarLong result;
	do {
		Minecraft_Byte temp = (Minecraft_Byte)(value & 0b01111111);
		// Note: >>> means that the sign bit is shifted with the rest of the number rather than being left alone
		//value >>>= 7;
		uint64_t* hackValue = (uint64_t*)&value;
		*hackValue >>= 7;
		if (value != 0) {
			temp |= 0b10000000;
		}
		result.bytes[numWrite] = temp;
		numWrite++;
	} while (value != 0);
	result.sizeOfUsedBytes = numWrite;
	if (PlaceToWrite != nullptr) memcpy(PlaceToWrite, result.bytes, result.sizeOfUsedBytes);
	return result;
}

void ConvertStringToMinecraftString(std::string& str, int N)
{
	if (str.size() > N) str.erase(str.begin() + N, str.end());
	Minecraft_Int stringSize = (Minecraft_Int)str.size();
	Minecraft_VarInt strSize = CodeVarInt(stringSize);
	for (int i = 0; i < strSize.sizeOfUsedBytes; i++)
	{
		str.insert(str.begin(), 0);
	}
	memcpy(const_cast<char*>(str.data()), strSize.bytes, strSize.sizeOfUsedBytes);
}

std::string DecodeMinecraftString(const char* minecraftString, int* sizeOfCodedString)
{
	int sizeOfUsedBytes;
	Minecraft_Int stringSize = DecodeVarInt(minecraftString, &sizeOfUsedBytes);
	if (sizeOfCodedString != nullptr) * sizeOfCodedString = sizeOfUsedBytes + stringSize;
	return std::string(&minecraftString[sizeOfUsedBytes], stringSize);
}

void bitShiftRightVarInt(Minecraft_VarInt* value, int shift)
{
	for (int i = 1; i < value->sizeOfUsedBytes; i++)
	{
		value->bytes[value->sizeOfUsedBytes - i] = value->bytes[value->sizeOfUsedBytes - i] >> shift | value->bytes[value->sizeOfUsedBytes - i - 1] << (8 - shift);
	}
	value->bytes[0] >>= shift;
}

void bitShiftRightVarLong(Minecraft_VarLong* value, int shift)
{
	for (int i = 1; i < value->sizeOfUsedBytes; i++)
	{
		value->bytes[value->sizeOfUsedBytes - i] = value->bytes[value->sizeOfUsedBytes - i] >> shift | value->bytes[value->sizeOfUsedBytes - i - 1] << (8 - shift);
	}
	value->bytes[0] >>= shift;
}

DataBuffer::~DataBuffer()
{
	delete[] buffer;
}

void DataBuffer::AllocateBuffer(int size)
{
	if (buffer != nullptr) delete[] buffer;
	offset = 0;
	bufferSize = size;
	buffer = new char[bufferSize];
	memset(buffer, 0, bufferSize);
}

void DataBuffer::SetBufferTo(char* data, int dataSize)
{
	if (data == nullptr || dataSize <= 0)
		__debugbreak();
	if (buffer != nullptr)
	{
		delete[] buffer;
	}
	offset = 0;
	bufferSize = dataSize;
	buffer = data;
}

void DataBuffer::SetOffset(int newOffset)
{
	offset = newOffset;
}

int DataBuffer::GetOffset()
{
	return offset;
}

void DataBuffer::AddToOffset(int value)
{
	offset += value;
}

bool DataBuffer::ReadBool()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	bool returnValue = *(bool*)& buffer[offset];
	offset += sizeof(bool);
	return returnValue;
}

Minecraft_Int DataBuffer::ReadInt()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	Minecraft_Int returnValue = *(Minecraft_Int*)& buffer[offset];
	uint32_t hack = _byteswap_ulong(returnValue);
	returnValue = *(Minecraft_Int*)& hack;
	offset += sizeof(Minecraft_Int);
	return returnValue;
}
 
Minecraft_Int DataBuffer::ReadVarInt()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	int additonalOffset;
	Minecraft_Int returnValue = DecodeVarInt(&buffer[offset], &additonalOffset);
	offset += additonalOffset;
	return returnValue;
}

Minecraft_Long DataBuffer::ReadLong()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	Minecraft_Long returnValue = *(Minecraft_Long*)& buffer[offset];
	uint64_t hack = _byteswap_uint64(returnValue);
	returnValue = *(Minecraft_Long*)& hack;
	offset += sizeof(Minecraft_Long);
	return returnValue;
}

Minecraft_Long DataBuffer::ReadVarLong()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	int additonalOffset;
	Minecraft_Long returnValue = DecodeVarLong(&buffer[offset], &additonalOffset);
	offset += additonalOffset;
	return returnValue;
}

std::string DataBuffer::ReadMinecraftString()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	int additonalOffset;
	std::string returnValue = DecodeMinecraftString(&buffer[offset], &additonalOffset);
	offset += additonalOffset;
	return returnValue;
}

Minecraft_Byte DataBuffer::ReadByte()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	offset += sizeof(Minecraft_Byte);
	return *(Minecraft_Byte*)& buffer[offset - sizeof(Minecraft_Byte)];
}

Minecraft_UnsignedByte DataBuffer::ReadUnsignedByte()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	offset += sizeof(Minecraft_UnsignedByte);
	return *(Minecraft_UnsignedByte*)& buffer[offset - sizeof(Minecraft_UnsignedByte)];
}

Minecraft_Short DataBuffer::ReadShort()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	Minecraft_Short returnValue = *(Minecraft_Short*)& buffer[offset];
	uint16_t hack = _byteswap_ushort(returnValue);
	returnValue = *(Minecraft_Short*)& hack;
	offset += sizeof(Minecraft_Short);
	return returnValue;
}

Minecraft_UnsignedShort DataBuffer::ReadUnsignedShort()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	Minecraft_UnsignedShort returnValue = *(Minecraft_UnsignedShort*)& buffer[offset];
	returnValue = _byteswap_ushort(returnValue);
	offset += sizeof(Minecraft_UnsignedShort);
	return returnValue;
}

char* DataBuffer::ReadArrayOfBytes(int arraySize)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0 || arraySize < 0 || arraySize + offset > bufferSize) __debugbreak();
	offset += arraySize;
	return &buffer[offset - arraySize];
}

Minecraft_Float DataBuffer::ReadFloat()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	offset += sizeof(Minecraft_Float);
	uint32_t hack = _byteswap_ulong(*(uint32_t*)& buffer[offset - sizeof(Minecraft_Float)]); // change bytes format from big endian to little endian
	return *(Minecraft_Float*)& hack;
}

Minecraft_Double DataBuffer::ReadDouble()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	offset += sizeof(Minecraft_Double);
	uint64_t hack = _byteswap_uint64(*(uint64_t*)& buffer[offset - sizeof(Minecraft_Double)]); // change bytes format from big endian to little endian
	return *(Minecraft_Double*)& hack;
}

void DataBuffer::WriteBool(bool value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	*(bool*)& buffer[offset] = value;
	offset += sizeof(bool);
}

void DataBuffer::WriteInt(Minecraft_Int value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	uint32_t hack = _byteswap_ulong(value);
	*(Minecraft_Int*)& buffer[offset] = *(Minecraft_Int*) &hack;
	offset += sizeof(Minecraft_Int);
}

void DataBuffer::WriteVarInt(Minecraft_Int value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	Minecraft_VarInt val = CodeVarInt(value, &buffer[offset]);
	offset += val.sizeOfUsedBytes;
}

void DataBuffer::WriteLong(Minecraft_Long value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	uint64_t hack = _byteswap_uint64(value);
	*(Minecraft_Long*)& buffer[offset] = *(Minecraft_Long*) &hack;
	offset += sizeof(Minecraft_Long);
}

void DataBuffer::WriteVarLong(Minecraft_Long value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	Minecraft_VarLong val = CodeVarLong(value, &buffer[offset]);
	offset += val.sizeOfUsedBytes;
}

void DataBuffer::WriteMinecraftString(std::string value, int N)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	ConvertStringToMinecraftString(value, N);
	memcpy(&buffer[offset], value.c_str(), value.size());
	offset += (int)value.size();
}

void DataBuffer::WriteByte(Minecraft_Byte value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	*(Minecraft_Byte*)& buffer[offset] = value;
	offset += sizeof(Minecraft_Byte);
}

void DataBuffer::WriteUnsignedByte(Minecraft_UnsignedByte value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	*(Minecraft_UnsignedByte*)& buffer[offset] = value;
	offset += sizeof(Minecraft_UnsignedByte);
}

void DataBuffer::WriteShort(Minecraft_Short value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	uint16_t hack = _byteswap_ushort(value);
	*(Minecraft_Short*)& buffer[offset] = *(Minecraft_Short*) &hack;
	offset += sizeof(Minecraft_Short);
}

void DataBuffer::WriteUnsignedShort(Minecraft_UnsignedShort value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	*(Minecraft_UnsignedShort*)& buffer[offset] = _byteswap_ushort(value);;
	offset += sizeof(Minecraft_UnsignedShort);
}

void DataBuffer::WriteArrayOfBytes(char* bytes, int arraySize)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0 || arraySize + offset > bufferSize || arraySize < 0) __debugbreak();
	memcpy(&buffer[offset], bytes, arraySize);
	offset += arraySize;
}

void DataBuffer::WriteDouble(Minecraft_Double value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	*(uint64_t*)& buffer[offset] = _byteswap_uint64(*(uint64_t*)& value); // change bytes format from little endian to big endian
	offset += sizeof(Minecraft_Double);
}

void DataBuffer::WriteFloat(Minecraft_Float value)
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	*(uint32_t*)& buffer[offset] = _byteswap_ulong(*(uint32_t*)&value); // change bytes format from little endian to big endian
	offset += sizeof(Minecraft_Float);
}

char* DataBuffer::GetBuffer()
{
	if (buffer == nullptr || offset >= bufferSize || offset < 0) __debugbreak();
	return buffer;
}

Chunk::~Chunk()
{
	if (sections) delete[] sections;
	if (blockEntities) delete[] blockEntities;
}

ChunkSection::~ChunkSection()
{
	if (palette) delete[] palette;
	if (dataArray) delete[] dataArray;
}

World::~World()
{
	for (int i = 0; i < chunks.size(); i++)
	{
		delete chunks[i];
	}
}
