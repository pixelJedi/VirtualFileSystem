#include "IVFS.h"

#include <sstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <bitset>

/* ---File------------------------------------------------------------------ */

std::map<File::Sections, uint32_t> File::infoAddr = {
	{Sections::nextTB,			0 * ADDR},
	{Sections::sizeVal,			1 * ADDR},
	{Sections::firstDataAddr,	3 * ADDR}
};

uint64_t File::ReadPtr() const
{	
	return uint64_t(GetSize() % BLOCK + 1);
}

char* File::ReadNext()
{
	return nullptr;
}
size_t File::WriteData(char* data)
{
	return size_t();
}

File::File(uint32_t nodeAddr, uint32_t blockAddr, std::string name)
{
	_nodeAddr = nodeAddr;
	_name = name;
	_realSize = 0;

	for(int i = 0;i<CLUSTER;++i)
		blocks.push_back(blockAddr+i*BLOCK);
}
File::~File()
{
}

/* ---VDisk----------------------------------------------------------------- */

std::map<VDisk::Sections, uint32_t> VDisk::infoAddr = {
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
uint64_t VDisk::EstimateMaxSize(uint64_t size) const
{
	return uint64_t(DISKDATA + EstimateNodeCapacity(size) * NODEDATA + EstimateBlockCapacity(size) * BLOCK);
}
uint64_t VDisk::GetDiskSize(std::string filename) const
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	uint64_t size = in.tellg();
	in.close();
	return size;
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
	disk.read(data, length);
	return true;
}

uint32_t VDisk::GetFreeNode()
{
	if (freeNodes)
	{
		char* nodeValue = new char[NODEDATA];
		for (uint32_t i = 0; i <= maxNode; ++i)
		{
			GetBytes(infoAddr[Sections::firstNode] + i, nodeValue, NODEDATA);
			if (IsEmptyNode(nodeValue))
				return i;
		}
	}
	return UINT32_MAX;
}
bool VDisk::IsEmptyNode(char* nodeValue) const
{
	for (short i = 0; i != NODEDATA; ++i)
		if (nodeValue[i] != char(0)) return true;
	return false;
}
char* VDisk::CreateNodeAt(uint32_t freeNode, uint32_t freeBlock, const char* name, bool isDir)
{
	try
	{
		char* newNode = new char[NODEDATA];
		short ibyte = 0;
	// Node code
		for (; ibyte < ADDR; ++ibyte)					
			newNode[ibyte] = DataToChar(freeNode)[ibyte];
	// Metadata
		newNode[ibyte++] = BuildFileMetadata(isDir, 0, 0);
	// Name
		for (; ibyte < (NODEDATA-2*ADDR-1); ++ibyte)
			newNode[ibyte] = name[ibyte];
	// File address
		for (; ibyte < ADDR; ++ibyte)					
			newNode[ibyte] = DataToChar(freeBlock)[ibyte];
		return newNode;
	} 
	catch(...)
	{
		return nullptr;
	}
}
char VDisk::BuildFileMetadata(bool isDir, bool inWriteMode, short inReadMode)
{
	std::bitset<8> metabitset(inReadMode);
	if (isDir) metabitset.set(7);
	if (inWriteMode) metabitset.set(4);

	return static_cast<char>(metabitset.to_ulong());
}
uint64_t VDisk::AbsoluteReadPtr(File* file) const
{
	return (file->blocks.at(file->GetSize() / BLOCK + 1) + file->ReadPtr());
}

/// <summary>
/// Checks if the file exists on disk, tracking downs the relations in the Node section
/// </summary>
/// <returns>File* if file exists, nullptr otherwise</returns>
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

uint64_t VDisk::GetSizeInBytes() const
{
	return sizeInBytes;
}
std::string VDisk::GetName() const
{
	return name;
}

File* VDisk::Fmalloc(const char* name)
{
	if (!freeNodes) return nullptr;

	uint32_t freeNode = GetFreeNode();
	if (freeNode == UINT32_MAX)
	{
		std::cout << "Failed to find a free node\n";
		return nullptr;
	}   

	uint32_t freeBlock = TakeFreeBlocks(nextFree);
	if (freeBlock == UINT32_MAX)
	{
		std::cout << "Failed to get a free block\n";
		return nullptr;
	}

	File* f = new File(freeNode, freeBlock, name);
	SetBytes(infoAddr[Sections::firstNode] + freeNode, CreateNodeAt(freeNode, freeBlock, name, 0), NODEDATA);
	InitTitleBlock(f);

	return f;
}

uint32_t VDisk::BlocksLeft() const
{
	return freeBlocks;
}
uint32_t VDisk::TakeFreeBlocks(uint32_t& nextFree)
{
	if (!freeBlocks) return UINT32_MAX;
	// todo: implement proper tracking of busy block, e.g. bitmap
	uint32_t curFree = nextFree;
	nextFree = (nextFree + CLUSTER) > maxBlock ? UINT32_MAX : nextFree + CLUSTER;
	// todo: make it respect eof
	freeBlocks -= CLUSTER;
	return curFree;
}
void VDisk::InitTitleBlock(File* file)
{
	char* data = new char[BLOCK];

	SetBytes(file->GetAddr() + File::infoAddr[File::Sections::nextTB], DataToChar(file->GetAddr()), ADDR);
	SetBytes(file->GetAddr() + File::infoAddr[File::Sections::sizeVal], DataToChar(file->GetSize()), 2 * ADDR);

	uint64_t totalBlocks = file->blocks.size();
	for(int i = 0; i < totalBlocks; ++i)
		SetBytes(file->GetAddr() + File::infoAddr[File::Sections::firstDataAddr] + i * ADDR, DataToChar(file->blocks[i]), ADDR);

	delete[] data;
}

bool VDisk::InitDisk()
{
	try
	{
		FillWithZeros(disk, sizeInBytes);
		freeNodes = maxNode;
		freeBlocks = maxBlock;
		nextFree = DISKDATA + maxNode * NODEDATA;
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
		SetBytes(infoAddr[Sections::freeNodes], DataToChar(freeNodes), ADDR);
		SetBytes(infoAddr[Sections::freeBlocks], DataToChar(freeBlocks), ADDR);
		SetBytes(infoAddr[Sections::maxNode], DataToChar(maxNode), ADDR);
		SetBytes(infoAddr[Sections::nextFree], DataToChar(nextFree), ADDR);
		std::cout << "Disk \"" << name << "\" updated\n";
	}
	catch (...)
	{
		return false;
	}
	return true;
}

char* VDisk::ReadInfo(VDisk::Sections info)
{
	char* bytes; 
	switch (info)
	{
	case VDisk::Sections::freeNodes:
	case VDisk::Sections::freeBlocks:
	case VDisk::Sections::maxNode:
	case VDisk::Sections::nextFree:
		bytes = new char[ADDR];
		GetBytes(infoAddr[info], bytes, ADDR);
		return bytes;
	default:
		return nullptr;
	};
}

VDisk::VDisk(const std::string fileName):
	name(fileName),
	sizeInBytes(GetDiskSize(fileName)),
	maxNode(CharToInt32(OpenAndReadInfo(fileName,infoAddr[Sections::maxNode],ADDR))),
	maxBlock(EstimateBlockCapacity(sizeInBytes))
{
	std::cout << "Disk \"" << name << "\" opened\n";
	VDisk::disk.open(fileName, std::fstream::in | std::ios::out | std::fstream::binary);
	freeNodes = CharToInt32(ReadInfo(Sections::freeNodes));
	freeBlocks = CharToInt32(ReadInfo(Sections::freeBlocks));
	nextFree = CharToInt32(ReadInfo(Sections::nextFree));

}
VDisk::VDisk(const std::string fileName, const uint64_t size) :
	name(fileName),
	maxNode(EstimateNodeCapacity(size)),
	maxBlock(EstimateBlockCapacity(size)),
	sizeInBytes(EstimateMaxSize(size))
{
	std::cout << "Disk \"" << name << "\" created\n";
	VDisk::disk.open(fileName, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
	if (!InitDisk())
		std::cout << "Init failed!\n";
}
VDisk::~VDisk()
{
	UpdateDisk();
	disk.close();
	std::cout << "Disk \"" << name << "\" closed\n";
}

/* ---VFS------------------------------------------------------------------- */

bool VFS::MountOrCreate(std::string& diskName)
{
	std::cout << "-------------------------------------------\n";
	bool mountSuccessful = false;
	try
	{
		if (!std::filesystem::exists(diskName))
		{
			char answer;
			std::cout << "\"" << diskName << "\" doesn't exist. Create? y/n\n";
			std::cin >> answer;
			std::cin.ignore(UINT32_MAX, '\n');
				std::cin.clear();
			switch (answer)
			{
			case 'Y':
			case 'y':
			{
				size_t diskSize = 0;
				std::cout << "Specify size for disk \"" + diskName + "\", bytes (minimum " + std::to_string(DISKDATA + NODEDATA + CLUSTER*BLOCK) + " B)\n";
				std::cin >> diskSize;
				std::cin.ignore(UINT32_MAX, '\n');
				std::cin.clear();
				if (IsValidSize(diskSize))
				{
					VDisk* vd = new VDisk(diskName, diskSize);
					VFS::disks.push_back(vd);
					std::cout << "Created and mounted disk \"" + diskName + "\" with size " + std::to_string(vd->GetSizeInBytes()) + " B\n";
					mountSuccessful = true;
					break;
				}
				std::cout << "Size is invalid. ";
			}
			[[fallthrough]];
			case 'N':
			case 'n':
				std::cout << "Disk \"" << diskName << "\" was not mounted\n";
				break;
			default:
				std::cout << "Bad input. Try again\n";
				break;
			}
		}
		else
		{
			std::cout << "Mounted disk \"" << diskName << "\" to the VFS\n";
			VFS::disks.push_back(new VDisk(diskName));
			mountSuccessful = true;
		}
	}
	catch (...)
	{
	}
	std::cout << "-------------------------------------------\n";
	return mountSuccessful;
}
bool VFS::Unmount(const std::string& diskName)
{
	if (disks.empty())
	{
		std::cout << "No mounted disks\n";
		return false;
	}
	for (auto iter = disks.begin(); iter != disks.end(); )
	{
		if ((*iter)->GetName() == diskName)
		{
			disks.erase(iter);
			std::cout << "Disk \"" << diskName << "\" was unmounted\n";
			return true;
		}
	}
	std::cout << "Disk \"" << diskName << "\" not found\n";
	return false;
}

bool VFS::IsValidSize(size_t size)
{
	return (size >= (DISKDATA + NODEDATA + CLUSTER * BLOCK) && size <= UINT32_MAX);
}

File* VFS::Open(const char* name)
{
	return nullptr;
}

File* VFS::Create(const char* name)
{
	File* file = nullptr;
	VDisk* mostFreeDisk = disks[0];

	for (const auto& disk : disks)
	{
		file = disk->SeekFile(name);
		if (file) return file;
		if (disk->BlocksLeft() > mostFreeDisk->BlocksLeft()) mostFreeDisk = disk;
	}
	file = mostFreeDisk->Fmalloc(name);

	return file;
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

template<typename T> char* DataToChar(const T& data)
{
	const int mask = 0xFF;

	char* bytes = new char[sizeof(data)];
	for (int i = 0; i != sizeof(data); ++i)
	{
		bytes[i] = (data >> ((sizeof(data) - i - 1) * BYTE)) & mask;
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
	return data;
}

char* OpenAndReadInfo(std::string filename, uint32_t position, const uint32_t length)
{
	char* data = new char[length];

	std::ifstream is;
	is.open(filename);
	is.seekg(position, std::ios_base::beg);
	is.read(data, length);
	is.close();

	return data;
}