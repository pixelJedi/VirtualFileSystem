#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <cmath>
#include <algorithm>
#include <memory>
#include <mutex>
#include "Vertice.h"

/* ---Commmon--------------------------------------------------------------- */

#define BYTE		8
#define BLOCK		1024			// Reserved per block, bytes
#define CLUSTER		16				// Default blocks estimated per file
#define ADDR		4				// Address length, bytes
#define DISKDATA	ADDR*4			// Reserved for disk info, bytes
#define NODEDATA	64							// Reserved per node, bytes
#define NODEMETA	1							// Reserved per node metadata, bytes
#define NODENAME	NODEDATA-2*ADDR-NODEMETA	// Reserved per node name, bytes
#define TBDIFF		2				// Difference between main and secondary TB, ADDR

// todo: think of better const handling

/* ---File------------------------------------------------------------------ */

/// <summary>
/// The complex pointer used for locating file datablocks on VDisk.
/// On VDisk File is represented by node in node area and by data blocks in data area.
/// </summary>
struct File
{
private:

	std::string _fathername;
	std::string _name;
	uint64_t _realSize;				// Changed after writing, used for calculating position for write data
	bool _writemode;
	short _readmode_count;

	uint32_t _mainTB;
	uint32_t _lastTB;

	std::vector<uint32_t> blocks;

	char BuildFileMeta();
public:
	inline static short MAX_READERS = 15;

	// Getters

	std::string GetName() const { return _name; };
	std::string GetFather() const { return _fathername; };
	bool IsBusy() const { return _writemode || _readmode_count; };
	bool IsWriteMode() const { return _writemode; };
	short GetReaders() const { return _readmode_count; };
	uint32_t GetMainTB() const { return _mainTB; };
	uint32_t GetLastTB() const { return _lastTB; };
	uint64_t GetSize() const { return _realSize; };

	uint32_t CountDataBlocks() const { return blocks.size(); };	
	uint32_t GetCurDataBlock() const { return uint32_t(blocks[_realSize / BLOCK]); };	// Addr of the last written DB
	uint32_t GetDataBlock(uint32_t index) const { return blocks[index]; };				// Addr of the index-th DB
	uint32_t GetLastDataBlock() const { return blocks.back(); };						// Addr of the last allocated DB
	static short STBCapacity();
	uint32_t CountSlotsInTB() const;
	uint32_t Fseekp() const { return uint32_t(_realSize % BLOCK); };					// Number of the first empty byte from the last block's beginning
	size_t GetRemainingSize() const { return CountDataBlocks()* BLOCK - _realSize; };	// Remaining size

	// Setters

	void FlipWriteMode() { _writemode = !_writemode; };
	void AddReader();
	void RemoveReader();
	void SetLastTB(uint32_t addr) { _lastTB = addr; };
	void IncreaseSize(uint32_t val) { _realSize += val; };

	// Other

	uint32_t EstimateBlocksNeeded(size_t dataLength) const;
	std::string ParseLast(std::string path, char delim = '\\') const;
	char* NodeToChar(uint32_t nodeCode);
	void AddDataBlock(uint32_t datablock) { blocks.push_back(datablock); };
	friend std::ostream& operator<<(std::ostream& s, const File& node);

	// Class

	File() = delete;
	File(uint32_t blockAddr, std::string name, std::string fathername, bool addCluster = true);
	~File() {};
};
std::ostream& operator<<(std::ostream& s, const File& node);

/* ---BinDisk--------------------------------------------------------------- */

class BinDisk : std::fstream 
{
public:
	bool SetBytes(uint32_t position, const char* data, uint32_t length);	// Low-level writing
	bool GetBytes(uint32_t position, char* data, uint32_t length);			// Low-level reading

	void MakeZeroFile(size_t size);

	void Open(const std::string fileName, bool asNew = false);
	void Close();
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
	// Aliases for all key data sections
	enum class Sect 
	{ 
		s_data,
		s_nodes,
		s_blocks,

		dd_fNodes,
		dd_fBlks, 
		dd_maxNode,
		dd_nextFreeBlk,

		nd_ncode,
		nd_meta,
		nd_name,
		nd_addr,

		fd_nextTB,		// Address of the next title block, if such exists; address of the current block otherwise
		fd_realSize,	// Real size of the file (not takes into account reserved space)
		fd_firstDB,		// Address of the first data block
		fd_s_firstDB	// Address of the first data block in secondary TB
	};
	// Offsets for all key data sections in bytes
	static std::map<Sect, uint32_t> addrMap;

	const std::string name;
	const uint64_t sizeInBytes;		// Reserved size provided during creation
	const uint32_t maxNode;			// The limit on files
	const uint32_t maxBlock;		// The limit on blocks
	uint32_t freeNodes;
	uint32_t freeBlocks;
	uint32_t nextFreeBlock;

	BinDisk disk;					// Main data in/out stream
	Vertice<File*>* root;			// Hierarchy & search

	std::mutex blockReserve, nodeReserve, freeNodeReserve;

	Vertice<File*>* LoadHierarchy(uint32_t start_index = 0);	// Plain to tree
	void WriteHierarchy();										// Tree to plain
	void LoadFile(File* f);

	bool InitDisk();									// Format new VDisk
	bool UpdateDisk();									// Write data into the associated file

	uint32_t EstimateNodeCapacity(size_t size) const;
	uint32_t EstimateBlockCapacity(size_t size) const;
	uint64_t EstimateMaxSize(uint64_t size) const;		// User's size is truncated so that all blocks are of BLOCK size
	
	void UpdateBlockCounters(uint32_t count = 1);
	void TakeFreeBlock(File* f);						// Allocate 1 datablock + TB if needed
	uint32_t RequestDBlocks(File* f, uint32_t number); 	// Try to allocate [number] of blocks for [f]
	uint32_t RequestTB();
	void AppendTB(File* f, uint32_t addr);
	bool ExpandIfLT(File* f, size_t len);				// Allocate blocks to fit [len] bytes 
	void InitTB(File* f, uint32_t newAddr);
	
	std::tuple<uint32_t, uint32_t> GetPosLen(Sect info, uint32_t offset);	// Converts section offsets to an absolute data position
	char* ReadInfo(Sect info, uint32_t i = 0);			// Get raw data from a specific Section
	
	uint32_t TakeNode(uint32_t size);					// Reserve one node
	bool IsNodeEmpty(short index);
	bool IsNodeFile(short index);
	uint32_t GetNodeCode(short index);
	uint32_t GetNodeChild(short index);


public:
	std::string GetName() const { return name; };
	uint64_t GetSizeInBytes() const{ return sizeInBytes; };
	uint32_t GetBlocksLeft() const { return freeBlocks; };
	uint32_t GetNodesLeft() const { return freeNodes; };

	File* SeekFile(const char* path) const;					// Seeks for a file without creating it
	File* CreateFile(const char* path);						// Reserves space for a new file
	size_t WriteInFile(File* f, char* buff, size_t len);
	size_t ReadFromFile(File* f, char* buff, size_t len);

	std::string PrintSpaceLeft() const;
	void PrintTree();

	std::mutex tree;	// Locks file hierarchy updates 

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
	VDisk* GetMostFreeDisk();
	std::vector<VDisk*>::iterator GetDisk(std::string name);

	std::mutex diskSelection;
	std::mutex readAccessCheck;
	std::mutex writeAccessCheck;
public:
	bool MountOrCreate(std::string& diskName);
	bool Unmount(const std::string& diskName);

	File* Open(const char* name) override;
	File* Create(const char* name) override;
	size_t Read(File* f, char* buff, size_t len) override;
	size_t Write(File* f, char* buff, size_t len) override;
	void Close(File* f) override;

	void PrintAll();

	VFS();
	~VFS();
};

/* ---Misc------------------------------------------------------------------ */

char* OpenAndReadInfo(std::string filename, uint32_t position, const uint32_t length);

template<typename T> char* IntToChar(const T& data);
char* StrToChar(const std::string data);
uint32_t CharToInt32(const char* bytes);
uint64_t CharToInt64(const char* bytes);
char* DirToChar(uint32_t nodecode, std::string name, uint32_t firstchild);

uint64_t GetDiskSize(std::string filename);	

void TreeToPlain(std::vector<char*>& info, Vertice<File*>& tree, uint32_t& nodecode);
uint32_t CountNodes(const std::string_view path); // todo: use Path class for all path operations