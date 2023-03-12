#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <cmath>
#include <algorithm>
#include <memory>
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
// todo: think of better const handling

/* ---Node-|-File----------------------------------------------------------- */

struct Node
{
protected:

	std::string _fathername;
	std::string _name;
	uint32_t _nodeAddr;

public:

	std::string GetName() { return _name; };
	std::string GetFather() { return _fathername; };
	uint32_t GetNode() const { return _nodeAddr; };

	virtual char* NodeToChar(uint32_t nodeCode) = 0;

	Node(uint32_t nodeAddr, std::string name, std::string fathername);
	virtual ~Node() {};

	friend std::ostream& operator<<(std::ostream& s, const Node& node);
};

struct Dir : public Node
{
	char* NodeToChar(uint32_t nodeCode);

	Dir() = delete;
	Dir(uint32_t nodeAddr, std::string name, std::string fathername);
	~Dir() {};
};

std::ostream& operator<<(std::ostream& s, const Node& node);

/// <summary>
/// The complex pointer used for locating file datablocks on VDisk.
/// On VDisk File is represented by node in node area and by data blocks in data area.
/// </summary>
struct File : public Node
{
private:
	inline static short MAX_READERS = 15;
	uint64_t _realSize;				// Changed after writing, used for calculating position for write data
	bool _writemode;
	short _readmode_count;

	uint32_t _mainTB;
	uint32_t _lastTB;

	std::vector<uint32_t> blocks;	// Remember: addresses are ADDR length

	char BuildFileMeta();
public:

	uint32_t GetMainTB() const { return _mainTB; };
	uint32_t GetLastTB() const { return _lastTB; };
	void SetLastTB(uint32_t addr) { _lastTB = addr; };
	uint64_t GetSize() const { return _realSize; };
	void IncreaseSize(uint32_t val) { _realSize += val; };
	uint32_t CountDataBlocks() const { return blocks.size(); };
	uint32_t GetCurDataBlock() const;
	uint32_t GetDataBlock(uint32_t index) const { return blocks[index]; };
	uint32_t GetLastDataBlock() const { return blocks.back(); };
	size_t GetRemainingSpace() const;
	uint32_t GetNextSlot() const; 
	uint32_t EstimateBlocksNeeded(size_t dataLength) const;
	bool IsBusy() { return _writemode || _readmode_count; };
	bool IsWriteMode() { return _writemode; };
	short GetReaders() { return _readmode_count; };

	void FlipWriteMode();
	void AddReader();
	void RemoveReader();
	void AddDataBlock(uint32_t datablock);

	uint64_t Fseekp() const;
	size_t ReadNext(char* buffer);		// TBD

	char* NodeToChar(uint32_t nodeCode);

	File() = delete;
	File(uint32_t nodeAddr, uint32_t blockAddr, std::string name, std::string fathername, bool writemode = false, short readmode_count = 0);
	~File() {};
};

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
		fd_firstDB		// Address of the first data block
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
	Vertice<Node*>* root;			// Hierarchy & search

	Vertice<Node*>* LoadHierarchy(uint32_t start_index = 0);
	void WriteHierarchy();

	uint32_t EstimateNodeCapacity(size_t size) const;
	uint32_t EstimateBlockCapacity(size_t size) const;
	uint64_t EstimateMaxSize(uint64_t size) const;		// User's size is truncated so that all blocks are of BLOCK size
	
	uint32_t TakeFreeNode();
	uint32_t TakeFreeBlocks();
	void UpdateBlockCounters(uint32_t count = 0);
	uint32_t TakeFreeBlock(File* f);
	
	bool InitDisk();									// Format new VDisk
	bool UpdateDisk();									// Write data into the associated file
	
	std::tuple<uint32_t, uint32_t> GetPosLen(Sect info, uint32_t i);
	char* ReadInfo(Sect info, uint32_t i = 0);			// Get raw data from a specific Section
	
	bool IsEmptyNode(short index);
	bool IsFileNode(short index);
	uint32_t GetNodeCode(short index);
	uint32_t GetChildAddr(short index);

	void InitTB(File* file, uint32_t newAddr);
	bool ExpandIfLT(File* f, size_t len);
	uint32_t AllocateNew(File* f, uint32_t nBlocks);
	bool NoSlotsInTB(File* f);

	uint64_t GetAbsoluteAddrInBlock(uint32_t blockAddr, uint32_t offset) const;

public:
	std::string GetName() const { return name; };
	uint64_t GetSizeInBytes() const{ return sizeInBytes; };
	uint32_t GetBlocksLeft() const { return freeBlocks; };
	uint32_t GetNodesLeft() const { return freeNodes; };

	File* SeekFile(const char* name) const;
	File* CreateFile(const char* name);						// Reserves space for a new file
	size_t WriteInFile(File* f, char* buff, size_t len);

	std::string PrintSpaceLeft() const;
	void PrintTree();

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
public:
	bool MountOrCreate(std::string& diskName);
	bool Unmount(const std::string& diskName);

	File* Open(const char* name) override;					// TBD
	File* Create(const char* name) override;
	size_t Read(File* f, char* buff, size_t len) override;	// TBD
	size_t Write(File* f, char* buff, size_t len) override;
	void Close(File* f) override;

	VFS();
	~VFS();
};

/* ---Misc------------------------------------------------------------------ */

void FillWithZeros(std::fstream& fs, size_t size);
char* OpenAndReadInfo(std::string filename, uint32_t position, const uint32_t length);

template<typename T> char* IntToChar(const T& data);
char* StrToChar(const std::string data);
uint32_t CharToInt32(const char* bytes);

uint64_t GetDiskSize(std::string filename);	// Check real size of an existing file

void TreeToPlain(std::vector<char*>& info, Vertice<Node*>& tree, uint32_t& count);