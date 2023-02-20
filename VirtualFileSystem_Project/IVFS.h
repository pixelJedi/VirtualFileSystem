#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <map>

/* ---Commmon--------------------------------------------------------------- */

#define BYTE		8
#define BLOCK		1024			// Reserved per block, bytes
#define CLUSTER		16				// Default blocks estimated per file
#define ADDR		4				// Address length, bytes
#define DISKDATA	ADDR*4			// Reserved for disk info, bytes
#define NODEDATA	64				// Reserved per node, bytes
// todo: think of better const handling

/* ---File------------------------------------------------------------------ */

/// <summary>
/// The complex pointer used for locating file datablocks on VDisk.
/// On VDisk File is represented by node in node area and by data blocks in data area.
/// </summary>
struct File
{
private:

	uint32_t _nodeAddr;			// Unmutable
	uint64_t _realSize;
	std::string _name;

public:

	enum class Sections
	{
		nextTB,			// Address of the next title block, if such exists; address of the current block otherwise
		sizeVal,		// Real size of the file (not takes into account reserved space)
		firstDataAddr	// Address of the first data block
	};
	static std::map<Sections, uint32_t> infoAddr;	// Offsets in bytes for data sections

	std::vector<uint32_t> blocks;					// Remember: addresses are ADDR length

	uint32_t GetNode() const { return _nodeAddr; };
	uint32_t GetAddr() const { return *blocks.begin(); };
	uint64_t GetSize() const { return _realSize; };
	uint32_t SetSize(uint64_t value) { _realSize = value; };
	std::string GetName() const { return _name; };
	std::string SetName(std::string value) { _name = value; };

	///
	/// <returns>Number of the next empty byte from the last block's beginning</returns>
	uint64_t ReadPtr() const;
	char* ReadNext();				// TBD
	size_t WriteData(char * data);	// TBD

	File() = delete;
	File(uint32_t nodeAddr, uint32_t blockAddr, std::string name);
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

	enum class Sections 
	{ 
		freeNodes,
		freeBlocks, 
		maxNode, 
		nextFree,
		firstNode,
		firstBlock
	};
	static std::map<Sections, uint32_t> infoAddr;	// Offsets in bytes for data sections

	const std::string name;
	const uint64_t sizeInBytes;			// The limitation is fixed and determines the number of files available
	const uint32_t maxNode;				// The number of files that can be stored
	const uint32_t maxBlock;			// The memory is allocated by Blocks
	uint32_t freeNodes;
	uint32_t freeBlocks;
	uint32_t nextFree;					// For fast write access

	uint32_t EstimateNodeCapacity(size_t size) const;
	uint32_t EstimateBlockCapacity(size_t size) const;
	uint64_t EstimateMaxSize(uint64_t size) const;
	uint64_t GetDiskSize(std::string filename) const;

	bool InitDisk();
	bool UpdateDisk();
	char* ReadInfo(Sections info);

	uint32_t GetFreeNode();
	bool IsEmptyNode(char* nodeValue) const;
	char* CreateNodeAt(uint32_t freeNode, uint32_t freeBlock, const char* name, bool isDir);
	char BuildFileMetadata(bool isDir, bool inWriteMode, short inReadMode);
	uint64_t AbsoluteReadPtr(File* file) const;

	uint32_t TakeFreeBlocks(uint32_t& nextFree);
	void InitTitleBlock(File* file);

	bool SetBytes(uint32_t position, const char* data, uint32_t length);
	bool GetBytes(uint32_t position, char* data, uint32_t length);

public:

	std::fstream disk;					// Main data in/out stream

	File* SeekFile(const char* name) const;
	File* Fmalloc(const char* name);

	std::string PrintSpaceLeft() const;
	uint32_t BlocksLeft() const;
	uint64_t GetSizeInBytes() const;
	std::string GetName() const;

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
	std::vector <VDisk*> disks;
	bool IsValidSize(size_t size);
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

template<typename T> char* DataToChar(const T& data);
char* DataToChar(const std::string data);

uint32_t CharToInt32(const char* bytes);