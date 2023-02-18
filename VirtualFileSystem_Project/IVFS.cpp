#include "IVFS.h"

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
	return uint32_t();
}

uint32_t VDisk::EstimateBlockÑapacity(size_t size) const
{
	return uint32_t();
}

bool VDisk::InitializeDisk()
{
	return false;
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

std::string VDisk::PrintSpaceLeft() const
{
	return std::string();
}

VDisk::VDisk(const std::string fileName):
	name(fileName),
	sizeInBytes(GetSize()),
	maxNode(EstimateNodeÑapacity(sizeInBytes)),
	maxBlock(EstimateBlockÑapacity(sizeInBytes))
{
}

VDisk::VDisk(const std::string fileName, const uint64_t size) :
	name(fileName),
	sizeInBytes(size),
	maxNode(EstimateNodeÑapacity(size)),
	maxBlock(EstimateBlockÑapacity(size))
{
}

VDisk::~VDisk()
{
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
