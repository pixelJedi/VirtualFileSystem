#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include "Vertice.h"

/* ---Commmon--------------------------------------------------------------- */

#define BYTE		8
#define BLOCK		1024			// Reserved per block, bytes
#define CLUSTER		16				// Default blocks estimated per file
#define ADDR		4				// Address length, bytes
#define DISKDATA	ADDR*4			// Reserved for disk info, bytes
#define NODEDATA	64							// Reserved per node, bytes
#define NODEMETA	1							// Reserved per node metadata, bytes
#define NODENAME	NODEDATA-2*ADDR-NODEMETA	// Address length, bytes
// todo: think of better const handling

/* ---File------------------------------------------------------------------ */

struct Node
{

};
/// <summary>
/// The complex pointer used for locating file datablocks on VDisk.
/// On VDisk File is represented by node in node area and by data blocks in data area.
/// </summary>
struct File : Node
{
private:

	std::string _name;
	uint32_t _nodeAddr;			// Unmutable
	uint64_t _realSize;			// Changed after writing, used for calculating position for write data

public:

	std::vector<uint32_t> blocks;				// Remember: addresses are ADDR length

	enum class Sect
	{
		nextTB,		// Address of the next title block, if such exists; address of the current block otherwise
		sizeVal,	// Real size of the file (not takes into account reserved space)
		firstAddr	// Address of the first data block
	};
	static std::map<Sect, uint32_t> infoAddr;	// Offsets in bytes for data sections

	uint32_t GetNode() const { return _nodeAddr; };
	uint32_t GetData() const { return *blocks.begin(); };
	uint64_t GetSize() const { return _realSize; };
	uint32_t SetSize(uint64_t value) { _realSize = value; };
	std::string GetName() const { return _name; };
	std::string SetName(std::string value) { _name = value; };

	bool IsBusy();						// TBD
	bool IsWriteMode();					// TBD

	uint64_t WritePtr() const;
	size_t ReadNext(char* buffer);	// TBD
	size_t WriteData(char * buffer);		// TBD

	File() = delete;
	File(uint32_t nodeAddr, uint32_t blockAddr, std::string name);
};

/* ---VDisk----------------------------------------------------------------- */

/// <summary>
/// VDisk emulates a physical storage within a physical file and is responsible for low-level data management.
/// Basically, VDisk is only intended to be used internally by the VFS class.
/// The data is reached through the fstream 'disk'; it's generally open while VDisk exists.
/// <para> For the details on the file format, see the README.md </para>
/// </summary>
class VDisk
{
private:

	std::fstream disk;							// Main data in/out stream
	Vertice<Node>* root;

	enum class Sect 
	{ 
		freeNodes,
		freeBlocks, 
		maxNode, 
		nextFree,
		firstNode,
		firstBlock,

		nofs_ncode,
		nofs_meta,
		nofs_name,
		nofs_addr
	};
	static std::map<Sect, uint32_t> infoAddr;	// Offsets in bytes for data sections

	const std::string name;
	const uint64_t sizeInBytes;
	const uint32_t maxNode;						// The limit on files
	const uint32_t maxBlock;					// The limit on blocks
	uint32_t freeNodes;
	uint32_t freeBlocks;
	uint32_t nextFreeBlock;

	uint32_t EstimateNodeCapacity(size_t size) const;
	uint32_t EstimateBlockCapacity(size_t size) const;
	uint64_t EstimateMaxSize(uint64_t size) const;		// User's size is truncated so that all blocks are of BLOCK size
	uint64_t GetDiskSize(std::string filename) const;	// Check real size of an existing file
	Vertice<Node>* LoadHierarchy(uint32_t start_index = 0);
	void WriteHierarchy();
	bool InitDisk();									// Format new VDisk
	bool UpdateDisk();									// Write data into the associated file
	char* ReadInfo(Sect info);							// Get raw data from a specific Section

	bool IsEmptyNode(short index);
	bool IsFileNode(short index);
	uint32_t GetNodeCode(short index);
	uint32_t GetChildAddr(short index);

	uint32_t TakeFreeNode();
	uint32_t TakeFreeBlocks();

	char* BuildNode(uint32_t nodeCode, uint32_t blockAddr, const char* name, bool isDir);
	char BuildFileMeta(bool isDir, bool inWriteMode, short inReadMode);
	void InitTitleBlock(File* file);

	uint64_t GetAbsoluteAddrInBlock(uint32_t blockAddr, uint32_t offset) const;

	bool SetBytes(uint32_t position, const char* data, uint32_t length);	// Low-level writing
	bool GetBytes(uint32_t position, char* data, uint32_t length);			// Low-level reading

public:
	std::string GetName() const { return name; };
	uint64_t GetSizeInBytes() const{ return sizeInBytes; };
	uint32_t GetBlocksLeft() const { return freeBlocks;	};

	std::string PrintSpaceLeft() const;

	File* SeekFile(const char* name) const;
	File* FMalloc(const char* name);						// Reserves space for a new file

	VDisk() = delete;
	VDisk(const std::string fileName);						// Open existing VDisk
	VDisk(const std::string fileName, const uint64_t size);	// Create new VDisk
	~VDisk();
};

/* ---VFS------------------------------------------------------------------- */

/// <summary>
/// The original task requires using this interface.
/// See the VFS child class for the implementation.
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
	std::vector <VDisk*> disks;
	bool IsValidSize(size_t size);
public:
	bool MountOrCreate(std::string& diskName);
	bool Unmount(const std::string& diskName);

	File* Open(const char* name) override;					// TBD
	File* Create(const char* name) override;				// TBD
	size_t Read(File* f, char* buff, size_t len) override;	// TBD
	size_t Write(File* f, char* buff, size_t len) override;	// TBD
	void Close(File* f) override;							// TBD

	VFS();
	~VFS();
};

/* ---Misc------------------------------------------------------------------ */

void FillWithZeros(std::fstream& fs, size_t size);
char* OpenAndReadInfo(std::string filename, uint32_t position, const uint32_t length);

template<typename T> char* IntToChar(const T& data);
char* StrToChar(const std::string data);

uint32_t CharToInt32(const char* bytes);

void PrintVerticeTree(Vertice<Node>* v, uint32_t count = 0);