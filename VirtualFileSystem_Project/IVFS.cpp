#include "IVFS.h"

#include <sstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <bitset>

/* ---File------------------------------------------------------------------ */

/// <summary>
/// Offsets from the block's beginning
/// </summary>
std::map<File::Sect, uint32_t> File::infoAddr = {
	{Sect::nextTB,			0 * ADDR},
	{Sect::sizeVal,			1 * ADDR},
	{Sect::firstAddr,	3 * ADDR}
};

/// <summary>
/// Check the read/write flags in the filenode
/// </summary>
/// <returns>true, if somebody is either reading or writing</returns>
bool File::IsBusy()
{
	// TBD
	return false;
}
/// <summary>
/// Check the write flag in the filenode
/// </summary>
/// <returns>true, if somebody is writing</returns>
bool File::IsWriteMode()
{
	// TBD
	return false;
}
///
/// <returns>Number of the first empty byte from the last block's beginning</returns>
uint64_t File::WritePtr() const
{	
	return uint64_t(GetSize() % BLOCK + 1);
}

/// <summary>
/// Reads the next block of data, storing it in the buffer
/// </summary>
/// <returns>The number of bytes that are left to read</returns>
size_t File::ReadNext(char* buffer)
{
	// TBD
	return 0;
}
/// <summary>
/// Writes the next block of data from the buffer
/// </summary>
/// <returns>The number of bytes that are left to write</returns>
size_t File::WriteData(char* buffer)
{
	// TBD
	// Use the WritePtr() to get the starting position
	// Remember: first and last blocks can be incomplete
	return size_t();
}

File::File(uint32_t nodeAddr, uint32_t blockAddr, std::string name)
{
	_name = name;
	_nodeAddr = nodeAddr;
	_realSize = 0;

	for(int i = 0;i<CLUSTER;++i)
		blocks.push_back(blockAddr+i);
}

/* ---VDisk----------------------------------------------------------------- */

std::map<VDisk::Sect, uint32_t> VDisk::infoAddr = {
	{Sect::freeNodes,	0 * ADDR},
	{Sect::freeBlocks,	1 * ADDR},
	{Sect::maxNode,		2 * ADDR},
	{Sect::nextFree,	3 * ADDR},
	{Sect::firstNode,	4 * ADDR}
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
	return in.tellg();
}
/// <summary>
/// Sets the file that hasn't been used yet
/// </summary>
/// <returns>True if operation was successful</returns>
bool VDisk::InitDisk()
{
	try
	{
		FillWithZeros(disk, sizeInBytes);
		freeNodes = maxNode;
		freeBlocks = maxBlock;
		nextFreeBlock = 0;
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
	// todo: check why not all binary data is displayed immediately in Notepad+. Stream updates data with a delay?
	// todo: think of splitting this method when other writings are moved here
	try
	{
		SetBytes(infoAddr[Sect::freeNodes], IntToChar(freeNodes), ADDR);
		SetBytes(infoAddr[Sect::freeBlocks], IntToChar(freeBlocks), ADDR);
		SetBytes(infoAddr[Sect::maxNode], IntToChar(maxNode), ADDR);
		SetBytes(infoAddr[Sect::nextFree], IntToChar(nextFreeBlock), ADDR);
		// The fstream is reopened in order to update displayed values in hex editor
		disk.close();
		disk.open(name, std::fstream::in | std::ios::out | std::fstream::binary);
		std::cout << "Disk \"" << name << "\" updated\n";
	}
	catch (...)
	{
		return false;
	}
	return true;
}
/// <summary>
/// Wraps the GetBytes for easy access to specific data segments
/// </summary>
char* VDisk::ReadInfo(VDisk::Sect info)
{
	char* bytes;
	switch (info)
	{
	case VDisk::Sect::freeNodes:
	case VDisk::Sect::freeBlocks:
	case VDisk::Sect::maxNode:
	case VDisk::Sect::nextFree:
		bytes = new char[ADDR];
		GetBytes(infoAddr[info], bytes, ADDR);
		return bytes;
	default:
		return nullptr;
	};
}

/// <summary>
/// Checks for node availability and updates FreeNodes counter
/// </summary>
/// <returns>The node code</returns>
uint32_t VDisk::TakeFreeNode()
{
	if (freeNodes)
	{
		for (uint32_t i = 0; i <= maxNode; ++i)
			// Optimization candidate: search for a free node
			// - start looking from last filled node
			// - store bitmap of used nodes
			if (IsEmptyNode(i))
			{
				--freeNodes;
				return i;
			}
	}
	return UINT32_MAX;
}
/// <summary>
/// Checks for blocks availability and updates FreeBlocks counter
/// </summary>
/// <returns>Address of the first allocated block</returns>
uint32_t VDisk::TakeFreeBlocks()
{
	if (!freeBlocks) return UINT32_MAX;
// todo: implement proper tracking of busy block, e.g. through bitmap
	uint32_t curFree = nextFreeBlock;	
// todo: allow to 1) allocate less than a cluster, and 2) non-neighbour blocks
	nextFreeBlock = (nextFreeBlock + CLUSTER) > maxBlock ? UINT32_MAX : nextFreeBlock + CLUSTER; 
	freeBlocks -= CLUSTER;
	return curFree;
}
bool VDisk::IsEmptyNode(short index)
{
	char* nodeValue = new char;	// Checking if name is empty
	GetBytes(infoAddr[Sect::firstNode] + index*NODEDATA, nodeValue, 1);
	return *nodeValue == char(0);
}
char* VDisk::BuildNode(uint32_t nodeCode, uint32_t blockAddr, const char* name, bool isDir)
{
	try
	{
		char* newNode = new char[NODEDATA];
		short ibyte = 0;
		// Node code
		for (; ibyte < ADDR; ++ibyte)
			newNode[ibyte] = IntToChar(nodeCode)[ibyte];
		// Metadata
		newNode[ibyte++] = BuildFileMeta(isDir, 0, 0);
		// Name
		for (; ibyte < (NODEDATA - 2 * ADDR - 1); ++ibyte)
			newNode[ibyte] = name[ibyte-ADDR-1];
		// File address
		for (; ibyte < ADDR; ++ibyte)					
			newNode[ibyte] = IntToChar(blockAddr)[ibyte];
		return newNode;
	} 
	catch(...)
	{
		return nullptr;
	}
}
char VDisk::BuildFileMeta(bool isDir, bool inWriteMode, short inReadMode)
{
	std::bitset<8> metabitset(inReadMode);
	if (isDir) metabitset.set(7);
	if (inWriteMode) metabitset.set(4);
	return static_cast<char>(metabitset.to_ulong());
}
void VDisk::InitTitleBlock(File* file)
{
	// todo: move the writing to the UpdateDisk, build data charset here
	char* data = new char[BLOCK];

	SetBytes(file->GetAddr() + File::infoAddr[File::Sect::nextTB], IntToChar(file->GetAddr()), ADDR);
	SetBytes(file->GetAddr() + File::infoAddr[File::Sect::sizeVal], IntToChar(file->GetSize()), 2 * ADDR);

	uint64_t totalBlocks = file->blocks.size();
	for (int i = 0; i < totalBlocks; ++i)
		SetBytes(GetAbsoluteAddr(file->GetAddr(),File::infoAddr[File::Sect::firstAddr] + i * ADDR), IntToChar(file->blocks[i]), ADDR);
	delete[] data;
}
/// <summary>
/// Byte number from the beginning of the disk
/// </summary>
uint64_t VDisk::GetAbsoluteAddr(uint32_t blockAddr, uint32_t offset) const
{
	return infoAddr[Sect::firstBlock] + blockAddr * BLOCK + offset;
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

/// <summary>
/// Checks if the file exists on disk, tracking down path through the Node section
/// </summary>
/// <returns>File* if file exists, nullptr otherwise</returns>
File* VDisk::SeekFile(const char* name) const
{
	// TBD
	return nullptr;
}
/// <summary>
/// Reserves and initializes space for a new file. 
/// Writes all data required to the disk.
/// </summary>
File* VDisk::FMalloc(const char* name)
{
	if (!freeNodes) return nullptr;

	uint32_t freeNode = TakeFreeNode();
	if (freeNode == UINT32_MAX)
	{
		std::cout << "Failed to find a free node\n";
		return nullptr;
	}   

	uint32_t freeBlock = TakeFreeBlocks();
	if (freeBlock == UINT32_MAX)
	{
		std::cout << "Failed to get a free block\n";
		return nullptr;
	}

	File* f = new File(freeNode, freeBlock, name);
	SetBytes(infoAddr[Sect::firstNode] + freeNode, BuildNode(freeNode, freeBlock, name, 0), NODEDATA);
	InitTitleBlock(f);
	UpdateDisk();
	return f;
}

VDisk::VDisk(const std::string fileName):
	name(fileName),
	sizeInBytes(GetDiskSize(fileName)),
	maxNode(CharToInt32(OpenAndReadInfo(fileName,infoAddr[Sect::maxNode],ADDR))),
	maxBlock(EstimateBlockCapacity(sizeInBytes))
{
	std::cout << "Disk \"" << name << "\" opened\n";
	
	disk.open(fileName, std::fstream::in | std::ios::out | std::fstream::binary);
	freeNodes = CharToInt32(ReadInfo(Sect::freeNodes));
	freeBlocks = CharToInt32(ReadInfo(Sect::freeBlocks));
	nextFreeBlock = CharToInt32(ReadInfo(Sect::nextFree));
	infoAddr[Sect::firstBlock] = DISKDATA + maxNode * NODEDATA;
}
VDisk::VDisk(const std::string fileName, const uint64_t size) :
	name(fileName),
	maxNode(EstimateNodeCapacity(size)),
	maxBlock(EstimateBlockCapacity(size)),
	sizeInBytes(EstimateMaxSize(size))
{
	std::cout << "Disk \"" << name << "\" created\n";
	
	disk.open(fileName, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
	infoAddr[Sect::firstBlock] = DISKDATA + maxNode * NODEDATA;
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

bool VFS::IsValidSize(size_t size)
{
	return (size >= (DISKDATA + NODEDATA + CLUSTER * BLOCK) && size <= UINT32_MAX);
}

bool VFS::MountOrCreate(std::string& diskName)
{
	std::cout << "-------------------------------------------\n";
	bool mountSuccessful = false;
	try
	{
		if (!std::filesystem::exists(diskName))
		{
			char answer;
			std::cout << "\"" << diskName << "\" doesn't exist. Create? y/n\n> ";
			std::cin >> answer;
			std::cin.ignore(UINT32_MAX, '\n');
			std::cin.clear();
			switch (answer)
			{
			case 'Y':
			case 'y':
			{
				size_t diskSize = 0;
				std::cout << "Specify size of disk \"" + diskName + "\", bytes (minimum " + std::to_string(DISKDATA + NODEDATA + CLUSTER*BLOCK) + " B)\n> ";
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
			delete (*iter);
			disks.erase(iter);
			std::cout << "Disk \"" << diskName << "\" was unmounted\n";
			return true;
		}
	}
	std::cout << "Disk \"" << diskName << "\" not found\n";
	return false;
}
/// <summary>
/// Opens the file in readonly mode. 
/// </summary>
/// <returns>nullptr if the file is already open or does not exist.</returns>
File* VFS::Open(const char* name)
{
	File* file = nullptr;
	for (const auto& disk : disks)
	{
		file = disk->SeekFile(name);
		if (file && !(file->IsWriteMode())) return file;
	}
	return file;
}
/// <summary>
/// Opens or creates the file in writeonly mode. 
/// </summary>
/// <returns>nullptr if the file is already open.</returns>
File* VFS::Create(const char* name)
{
	File* file = nullptr;
	VDisk* mostFreeDisk = disks[0];

	for (const auto& disk : disks)
	{
		file = disk->SeekFile(name);
		if (file && !(file->IsBusy())) return file;
		if (disk->GetBlocksLeft() > mostFreeDisk->GetBlocksLeft()) mostFreeDisk = disk;
	}
	file = mostFreeDisk->FMalloc(name);
	return file;
}
size_t VFS::Read(File* f, char* buff, size_t len)
{
	// TBD
	return size_t();
}
size_t VFS::Write(File* f, char* buff, size_t len)
{
	// TBD
	return size_t();
}
void VFS::Close(File* f)
{
	// todo: update read/write flags
	delete f;
	std::cout << "File \"" << f->GetName() << "\" closed\n";
}

VFS::VFS()
{
	std::cout << "\nVFS created\n";
}
VFS::~VFS()
{
	std::cout << "\nOver and out\n";
}

/* ---Misc------------------------------------------------------------------ */

void FillWithZeros(std::fstream& fs, size_t size)
{
	char c = char(0b0);
	while (size--) {
		fs.write(&c, 1);
	}
}

template<typename T> char* IntToChar(const T& data)
{
	const int mask = 0xFF;

	char* bytes = new char[sizeof(data)];
	for (int i = 0; i != sizeof(data); ++i)
	{
		bytes[i] = (data >> ((sizeof(data) - i - 1) * BYTE)) & mask;
	}
	return bytes;
}
char* StrToChar(const std::string data)
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