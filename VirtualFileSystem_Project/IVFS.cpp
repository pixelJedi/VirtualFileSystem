#include "IVFS.h"

#include <sstream>
#include <iostream>

/* ---File------------------------------------------------------------------ */

char* File::ReadNext()
{
	return nullptr;
}

size_t File::WriteData(char* data)
{
	return size_t();
}

File::File(uint32_t addr, uint32_t size, std::string name)
{
}

File::~File()
{
}

/* ---VDisk----------------------------------------------------------------- */

std::map<VDisk::Sections, uint32_t> VDisk::metadataAddr = {
	{Sections::freeNodes,	0 * ADDR},
	{Sections::freeBlocks,	1 * ADDR},
	{Sections::maxNode,		2 * ADDR},
	{Sections::nextFree,	3 * ADDR},
	{Sections::firstNode,	4 * ADDR}
};

uint32_t VDisk::EstimateNodeCapacity(size_t size) const
{
	// CLUSTER of BLOCKS is estimated per file by default
	return uint32_t((size - DISKDATA) / (NODEDATA + CLUSTER * BLOCK));
}
uint32_t VDisk::EstimateBlockCapacity(size_t size) const
{
	return uint32_t((size - DISKDATA - (size - DISKDATA) / (NODEDATA + CLUSTER * BLOCK) * NODEDATA) / BLOCK);
}

bool VDisk::SetBytes(uint32_t position, const char* data, uint32_t length)
{
	disk.seekp(position, std::ios_base::beg);
	disk.write(data, length);
	return true;
}

bool VDisk::GetBytes(uint32_t position, char* data, uint32_t length)
{
	disk.seekg(position, std::ios_base::beg);
	disk.read((char*)&data, length);
	return true;
}

uint64_t VDisk::GetSize()
{
	return uint64_t();
}

File* VDisk::SeekFile(const char* name) const
{
	return nullptr;
}

/*
-------------------------------------------
VDisk {name} usage:
Nodes used:		{x} of {x} ({x.xx}%)
Blocks used:	{x} of {x} ({x.xx}%)

Free dataspace:	{x} of {x} bytes ({x.xx}%)
-------------------------------------------
*/
std::string VDisk::PrintSpaceLeft() const
{
	size_t freeSpace = freeBlocks * BLOCK;

	std::ostringstream info;
	info.precision(2);
	info << std::fixed;
	info << "-------------------------------------------\n";
	info << "VDisk \"" << name << "\" usage:\n";
	info << "Nodes used: \t" << (maxNode-freeNodes) << " of " << maxNode << " (" << ((maxNode - freeNodes) * 1.0 / maxNode) * 100 << "%)\n";
	info << "Blocks used:\t" << (maxBlock-freeBlocks) << " of " << maxBlock << " (" << ((maxBlock - freeBlocks) * 1.0 / maxBlock) * 100 << "%)\n\n";
	info << "Free dataspace:\t" << freeSpace << " of " << sizeInBytes << " bytes (" << ((freeSpace * 1.0) / sizeInBytes) * 100 << "%)\n";
	info << "-------------------------------------------\n";

	return info.str();
}

bool VDisk::InitializeDisk()
{
	try
	{
		FillWithZeros(disk, sizeInBytes);
		UpdateDisk();
	}
	catch (...)
	{
		return false;
	}
	return true;
}

/// <summary>
/// Saves the VDisk stats in the assosiated file
/// </summary>
bool VDisk::UpdateDisk()
{
	try
	{
		SetBytes(metadataAddr[Sections::freeNodes], DataToChar(freeNodes), ADDR);
		SetBytes(metadataAddr[Sections::freeBlocks], DataToChar(freeBlocks), ADDR);
		SetBytes(metadataAddr[Sections::maxNode], DataToChar(maxNode), ADDR);
		SetBytes(metadataAddr[Sections::nextFree], DataToChar(nextFree), ADDR);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

char* VDisk::ReadInfo(VDisk::Sections info)
{
	char* bytes = new char[ADDR];
	switch (info)
	{
	case VDisk::Sections::freeNodes:
	case VDisk::Sections::freeBlocks:
	case VDisk::Sections::maxNode:
	case VDisk::Sections::nextFree:
		GetBytes(metadataAddr[info],bytes,ADDR);
		return bytes;
	default:
		return nullptr;
	}
}

VDisk::VDisk(const std::string fileName):
	name(fileName),
	sizeInBytes(CharToInt32(ReadInfo(Sections::maxNode))),
	maxNode(EstimateNodeCapacity(sizeInBytes)),
	maxBlock(EstimateBlockCapacity(sizeInBytes))
{
	VDisk::disk.open(fileName, std::fstream::in | std::fstream::out | std::ios::binary);
	freeNodes = maxNode;
	freeBlocks = maxBlock;

}

VDisk::VDisk(const std::string fileName, const uint64_t size) :
	name(fileName),
	sizeInBytes(size),	// todo: truncate to fit blocks estimated
	maxNode(EstimateNodeCapacity(sizeInBytes)),
	maxBlock(EstimateBlockCapacity(sizeInBytes))
{
	freeNodes = maxNode;
	freeBlocks = maxBlock;
	VDisk::disk.open(fileName, std::fstream::in | std::fstream::out | std::ios::binary);
	InitializeDisk();
}

VDisk::~VDisk()
{
	UpdateDisk();
	disk.close();
}

/* ---VFS------------------------------------------------------------------- */

bool VFS::MountOrCreate(std::string& diskName)
{
	return false;
}

bool VFS::Unmount(const std::string& diskName)
{
	return false;
}

File* VFS::Open(const char* name)
{
	return nullptr;
}

File* VFS::Create(const char* name)
{
	return nullptr;
}

size_t VFS::Read(File* f, char* buff, size_t len)
{
	return size_t();
}

size_t VFS::Write(File* f, char* buff, size_t len)
{
	return size_t();
}

void VFS::Close(File* f)
{
}

VFS::VFS()
{
}

VFS::~VFS()
{
}

/* ---Misc------------------------------------------------------------------ */

void FillWithZeros(std::fstream& fs, size_t size)
{
	char c = char(0b0);
	while (size--) {
		fs.write(&c, 1);
	}
}

char* DataToChar(const std::uint32_t& data)
{
	const int mask = 0xFF;
	uint32_t temp = data;

	char* bytes = new char[sizeof(data)];
	for (int i = 0; i != sizeof(data); ++i)
	{
		bytes[i] = (temp >> ((sizeof(data) - i - 1) * BYTE)) & mask;
	}
	return bytes;
}
char* DataToChar(const std::string data)
{
	char* bytes = new char[data.length() + 1];
	strcpy_s(bytes, data.length()+1, data.c_str());
	return bytes;
}

uint32_t CharToInt32(const char* bytes)
{
	const int mask = 0xFF;
	uint32_t data = 0;

	for (int i = 0; i != sizeof(int32_t); ++i)
	{
		data *= 0b100000000;
		data += (bytes[i] & mask);
	}
	/*
	while (*bytes)
	{
		total *= 2;
		if (*bytes++ == '1') total += 1;
	}*/
	return data;
}