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

uint32_t VDisk::EstimateNodeÑapacity(size_t size) const
{
	// CLUSTER of BLOCKS is estimated per file by default
	return uint32_t((size - DISKDATA) / (NODEDATA + CLUSTER * BLOCK));
}

uint32_t VDisk::EstimateBlockÑapacity(size_t size) const
{
	return uint32_t((size - DISKDATA - (size - DISKDATA) / (NODEDATA + CLUSTER * BLOCK) * NODEDATA) / BLOCK);
}

bool VDisk::InitializeDisk()
{
	FillWithZeros(disk,sizeInBytes);
	return true;
}

bool VDisk::SetBytes(size_t position, const char* data, size_t length)
{
	return false;
}

bool VDisk::GetBytes(size_t position, const char* data, size_t length)
{
	return false;
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

VDisk::VDisk(const std::string fileName):
	name(fileName),
	sizeInBytes(GetSize()),
	maxNode(EstimateNodeÑapacity(sizeInBytes)),
	maxBlock(EstimateBlockÑapacity(sizeInBytes))
{
	VDisk::disk.open(fileName, std::fstream::in | std::fstream::out | std::ios::binary);
	freeNodes = maxNode;
	freeBlocks = maxBlock;

}

VDisk::VDisk(const std::string fileName, const uint64_t size) :
	name(fileName),
	sizeInBytes(size),
	maxNode(EstimateNodeÑapacity(size)),
	maxBlock(EstimateBlockÑapacity(size))
{
	freeNodes = maxNode;
	freeBlocks = maxBlock;
	VDisk::disk.open(fileName, std::fstream::in | std::fstream::out | std::ios::binary);
	InitializeDisk();
}

VDisk::~VDisk()
{
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
