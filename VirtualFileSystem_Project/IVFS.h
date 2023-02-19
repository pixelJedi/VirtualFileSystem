#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <map>

/* ---File------------------------------------------------------------------ */

/// <summary>
/// The complex pointer used for locating file datablocks on VDisk
/// </summary>
struct File
{
private:
	uint32_t _addr;		// Unmutable
	uint32_t _size;		// Unmutable
	std::string _name;

	std::vector<uint32_t> blocks;

	size_t rptr;		// Next unread byte of data
public:
	uint32_t GetAddr() { return _addr; };
	uint32_t GetSize() { return _size; };
	std::string GetName() { return _name; };
	std::string SetName(std::string value) { _name = value; };

	char* ReadNext();
	size_t WriteData(char * data);

	File() = delete;
	File(uint32_t addr, uint32_t size, std::string name);
	~File();
};

/* ---VDisk----------------------------------------------------------------- */

/// <summary>
/// VDisk emulates a physical storage within a physical file and is responsible for low-level data management.
/// Basically, VDisk is only intended to be used internally by the VFS class.
/// <para> For the details on the file format, see the README.md </para>
/// </summary>
class VDisk
{
private:
	// todo: replace the temporary const handling
	#define BYTE		8
	#define BLOCK		1024			// Reserved per block, bytes
	#define CLUSTER		16				// Default blocks estimated per file
	#define ADDR		4				// Address length, bytes
	#define DISKDATA	ADDR*4			// Reserved for disk info, bytes
	#define NODEDATA	64				// Reserved per node, bytes
	enum class Sections 
	{ 
		freeNodes,
		freeBlocks, 
		maxNode, 
		nextFree,
		firstNode,
		firstBlock
	};
	struct SectionsClassHash
	{
		template <typename T>
		std::size_t operator()(T t) const
		{
			return static_cast<std::size_t>(t);
		}
	};
	static std::map<Sections, uint32_t> metadataAddr;	// Offsets in bytes for data sections

	std::fstream disk;					// Main data in/out stream

	const std::string name;
	const uint64_t sizeInBytes;			// The limitation is fixed and determines the number of files available
	const uint32_t maxNode;				// The number of files that can be stored
	const uint32_t maxBlock;			// The memory is allocated by Blocks
	uint32_t freeNodes;
	uint32_t freeBlocks;
	uint32_t nextFree;					// For fast write access

	uint32_t EstimateNodeCapacity(size_t size) const;
	uint32_t EstimateBlockCapacity(size_t size) const;
	uint64_t EstimateRealSize(uint64_t size) const;
	uint64_t GetSize(std::string filename) const;
	bool InitializeDisk();
	bool UpdateDisk();
	char* ReadInfo(Sections info);

	bool SetBytes(uint32_t position, const char* data, uint32_t length);
	bool GetBytes(uint32_t position, char* data, uint32_t length);
public:
	File* SeekFile(const char* name) const;
	std::string PrintSpaceLeft() const;

	VDisk() = delete;
	VDisk(const std::string fileName);							// Open existing VDisk
	VDisk(const std::string fileName, const uint64_t size);		// Create new VDisk
	~VDisk();
};

/* ---VFS------------------------------------------------------------------- */

/// <summary>
/// The original task requires the implementation of this interface.
/// See the VFS child class code for the implementation itself.
/// </summary>
struct IVFS
{
	virtual File* Open(const char* name) = 0;
	virtual File* Create(const char* name) = 0;
	virtual size_t Read(File* f, char* buff, size_t len) = 0;
	virtual size_t Write(File* f, char* buff, size_t len) = 0;
	virtual void Close(File* f) = 0;
};

/// <summary>
/// Stores multiple VDisks, each of which is associated with a physical file on the underlying file system.
/// In addition to obligatory IVFS functions, Mount/Unmount were added for managing physical files.
/// </summary>
class VFS : IVFS
{
private:
	std::vector <VDisk> disks;
public:
	bool MountOrCreate(std::string& diskName);
	bool Unmount(const std::string& diskName);

	File* Open(const char* name) override;
	File* Create(const char* name) override;
	size_t Read(File* f, char* buff, size_t len) override;
	size_t Write(File* f, char* buff, size_t len) override;
	void Close(File* f) override;

	VFS();
	~VFS();
};

/* ---Misc------------------------------------------------------------------ */

void FillWithZeros(std::fstream& fs, size_t size);
char* OpenAndReadInfo(std::string filename, uint32_t position, const uint32_t length);

char* DataToChar(const std::uint32_t& data);
char* DataToChar(const std::string data);

uint32_t CharToInt32(const char* bytes);