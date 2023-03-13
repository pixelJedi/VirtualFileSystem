#include "IVFS.h"

#include <sstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <bitset>

/* ---Node-|-File----------------------------------------------------------- */

Node::Node(uint32_t nodeAddr, std::string name, std::string fathername)
{
	_name = name;
	_name.resize(NODENAME);
	_nodeAddr = nodeAddr;
	_fathername = fathername;
}

std::ostream& operator<<(std::ostream& s, const Node& node)
{
	return s << node._nodeAddr;
}

Dir::Dir(uint32_t nodeAddr, std::string name, std::string fathername) : Node(nodeAddr, name, fathername){}

char* Dir::NodeToChar(uint32_t nodeCode)
{
	try
	{
		char* newNode = new char[NODEDATA];
		short ibyte = 0;
		// Node code
		for (; ibyte < ADDR; ++ibyte)
			newNode[ibyte] = IntToChar(nodeCode)[ibyte];
		// Metadata
		for (; ibyte < NODEMETA; ++ibyte)
			newNode[ibyte] = char(0b10000000);
		// Name
		for (; ibyte < NODENAME; ++ibyte)
			newNode[ibyte] = _name[ibyte - (ADDR + 1)];
		// File address
		for (; ibyte < ADDR; ++ibyte)
			newNode[ibyte] = IntToChar(_nodeAddr)[ibyte];
		return newNode;
	}
	catch (...)
	{
		return nullptr;
	}
}

char File::BuildFileMeta()
{
	std::bitset<8> metabitset(_readmode_count);
	if (_writemode) metabitset.set(4);
	return static_cast<char>(metabitset.to_ulong());
}

void File::FlipWriteMode()
{
	_writemode = !_writemode;
}

void File::AddReader()
{ 
	_readmode_count = _readmode_count < MAX_READERS ? ++_readmode_count : MAX_READERS;
}
void File::RemoveReader() 
{ 
	_readmode_count = _readmode_count > 0 ? --_readmode_count : 0;
}

void File::AddDataBlock(uint32_t datablock)
{
	blocks.push_back(datablock);
}
///
/// <returns>Number of the first empty byte from the last block's beginning</returns>
uint64_t File::Fseekp() const
{	
	return uint64_t(_realSize % BLOCK);
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
uint32_t File::GetCurDataBlock() const
{
	return uint32_t(blocks[_realSize/BLOCK]);
}

size_t File::GetRemainingSpace() const
{
	return size_t(CountDataBlocks() * BLOCK - _realSize);
}

uint32_t File::GetNextSlot() const
{
	return (CountDataBlocks() + 2) % (BLOCK / ADDR - 1) + 1;
}

uint32_t File::EstimateBlocksNeeded(size_t dataLength) const
{
	long long difference = dataLength - GetRemainingSpace();
	return difference > 0 ? uint32_t(std::ceil(difference * 1.0 / BLOCK)) : 0;
}

char* File::NodeToChar(uint32_t nodeCode)
{
	try
	{
		char* newNode = new char[NODEDATA];
		short i, ibyte = 0;
		// Node code
		for (i = 0; i < ADDR; ++i, ++ibyte)
			newNode[ibyte] = IntToChar(nodeCode)[i];
		// Metadata
		newNode[ibyte] = BuildFileMeta(); ++ibyte;
		// Name
		for (i = 0; i < NODENAME; ++i, ++ibyte)
			newNode[ibyte] = _name[i];
		// File address
		for (i = 0; i < ADDR; ++i, ++ibyte)
			newNode[ibyte] = IntToChar(_nodeAddr)[i];
		return newNode;
	}
	catch (...)
	{
		return nullptr;
	}
}

File::File(uint32_t nodeAddr, uint32_t blockAddr, std::string name, std::string fathername, bool addCluster) : Node(nodeAddr, name, fathername)
{
	_realSize = 0;
	_writemode = false;
	_readmode_count = 0;
	_lastTB = _mainTB = blockAddr;
	blockAddr++;

	if (addCluster)
		for(int i = 0;i<CLUSTER; ++i)
			blocks.push_back(blockAddr+i);
}

/* ---BinDisk--------------------------------------------------------------- */

bool BinDisk::SetBytes(uint32_t position, const char* data, uint32_t length)
{
	seekp(position, std::ios_base::beg);
	write(data, length);
	return true;
}
bool BinDisk::GetBytes(uint32_t position, char* data, uint32_t length)
{
	seekg(position, std::ios_base::beg);
	read(data, length);
	return true;
}

void BinDisk::MakeZeroFile(size_t size)
{
	char c = char(0b0);
	while (size--) {
		this->write(&c, 1);
	}
}

void BinDisk::Open(const std::string fileName, bool asNew)
{
	auto traits = std::fstream::in | std::fstream::out | std::fstream::binary;
	if (asNew) traits |= std::fstream::trunc;
	this->open(fileName, traits);
}
void BinDisk::Close()
{
	this->close();
}

/* ---VDisk----------------------------------------------------------------- */

std::map<VDisk::Sect, uint32_t> VDisk::addrMap = {
// Sections | offset from file begin
	{Sect::s_data,			0 * ADDR},
	{Sect::s_nodes,			4 * ADDR},
	{Sect::s_blocks,		-1},		// Depends on file, is updated in VDisk(...)
// DiskData | offset from s_data begin
	{Sect::dd_fNodes,		0 * ADDR},
	{Sect::dd_fBlks,		1 * ADDR},
	{Sect::dd_maxNode,		2 * ADDR},
	{Sect::dd_nextFreeBlk,	3 * ADDR},
// NodeData | offset from node's begin
	{Sect::nd_ncode,		0 * ADDR},
	{Sect::nd_meta,			1 * ADDR},
	{Sect::nd_name,			1 * ADDR + NODEMETA},
	{Sect::nd_addr,			1 * ADDR + NODEMETA + NODENAME},
// FileData | offset from Title Block's begin
	{Sect::fd_nextTB,		0 * ADDR},
	{Sect::fd_realSize,		1 * ADDR},
	{Sect::fd_firstDB,		3 * ADDR},
	{Sect::fd_s_firstDB,	1 * ADDR}
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

Vertice<Node*>* VDisk::LoadHierarchy(uint32_t start_index)
{
	/* Algorithm:
	1. Add root Vertice for this NC.
	2. Remember the NC (NodeCode) of node[start_index]: similar NC == children of the same node.
	3. Go through nodes, starting from start_index, while NC is similar.
	4.1 If is file, add File.
	4.2 If is dir, add Dir
	4.2.1 Call LoadHierarchy(<child's node addr as parameter>) and bind the result to root.
	*/

	Vertice<Node*>* root = new Vertice<Node*>();	// [1]
	uint32_t curr_nc = GetNodeCode(start_index);	// [2]

	for (uint32_t i = start_index; i != maxNode - freeNodes && curr_nc == GetNodeCode(i); ++i)  // [3]
	{
		Node* node;
		char* filename = ReadInfo(Sect::nd_name, i);
		uint32_t addr = GetChildAddr(i);

		if (IsFileNode(i))
		{
			node = new File(i, addr, filename, this->name, false);
			LoadFile((File*)node);
		}
		else
			node = new Dir(i, filename, this->name);
		root->Add(std::string{ filename }, node);	// [4]
		if (!IsFileNode(i))	root->BindNewTreeToChild(filename, LoadHierarchy(addr));
	}

	return root;
}
void VDisk::WriteHierarchy()
{
	uint32_t nodecode = 0;
	std::ostringstream info;

	std::vector<char*> nodes;
	TreeToPlain(nodes, *root, nodecode);

	std::cout << "Max nodecode: " << nodecode << std::endl;
	uint32_t pos = addrMap[Sect::s_nodes];
	for (int i = 0; i!= nodes.size(); i++)
	{
		disk.SetBytes(pos + i * NODEDATA, nodes[i], NODEDATA);
		//delete nodes[i];
	}
}
/// <summary>
/// Sets the file that hasn't been used yet
/// </summary>
/// <returns>True if operation was successful</returns>
bool VDisk::InitDisk()
{
	try
	{
		disk.MakeZeroFile(sizeInBytes);
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
	// todo: write nodes from root to node section
	try
	{
		disk.SetBytes(addrMap[Sect::dd_fNodes], IntToChar(freeNodes), ADDR);
		disk.SetBytes(addrMap[Sect::dd_fBlks], IntToChar(freeBlocks), ADDR);
		disk.SetBytes(addrMap[Sect::dd_maxNode], IntToChar(maxNode), ADDR);
		disk.SetBytes(addrMap[Sect::dd_nextFreeBlk], IntToChar(nextFreeBlock), ADDR);
		WriteHierarchy();
		// Debug: the fstream is reopened in order to update displayed values in hex editor
		disk.Close();
		disk.Open(name);
		// eof Debug
		std::cout << "Disk \"" << name << "\" updated\n";
	}
	catch (...)
	{
		return false;
	}
	return true;
}

std::tuple<uint32_t, uint32_t> VDisk::GetPosLen(Sect info, uint32_t i)
{
	uint32_t pos, len;
	switch (info)
	{
	case Sect::s_nodes:
		pos = addrMap[info] + i * NODEDATA;
		len = NODEDATA;
		break;
	case Sect::s_blocks:
		pos = addrMap[info] + i * BLOCK;
		len = BLOCK;
		break;
	case Sect::dd_fNodes:
	case Sect::dd_fBlks:
	case Sect::dd_maxNode:
	case Sect::dd_nextFreeBlk:
		pos = addrMap[Sect::s_data] + addrMap[info];
		len = ADDR;
		break;
	case Sect::nd_ncode:
	case Sect::nd_addr:
		pos = addrMap[Sect::s_nodes] + i * NODEDATA + addrMap[info];
		len = ADDR;
		break;
	case Sect::nd_meta:
		pos = addrMap[Sect::s_nodes] + i * NODEDATA + addrMap[info];
		len = NODEMETA;
		break;
	case Sect::nd_name:
		pos = addrMap[Sect::s_nodes] + i * NODEDATA + addrMap[info];
		len = NODENAME;
		break;
	case Sect::fd_nextTB:
	case Sect::fd_firstDB:
	case Sect::fd_s_firstDB:
		pos = addrMap[Sect::s_blocks] + i * BLOCK + addrMap[info];
		len = ADDR;
		break;
	case Sect::fd_realSize:
		pos = addrMap[Sect::s_blocks] + i * BLOCK + addrMap[info];
		len = 2 * ADDR;
		break;
	default:
		throw std::domain_error("The <info> parameter has no address assigned");
	};
	return std::make_tuple(pos,len);
}
/// <summary>
/// Wraps the GetBytes for easy access to specific data segments
/// </summary>
char* VDisk::ReadInfo(Sect info, uint32_t i)
{
	uint32_t pos, len;
	std::tie(pos, len) = GetPosLen(info, i);
	char* bytes = new char[len];
	disk.GetBytes(pos, bytes, len);
	return bytes;
}

/// <summary>
/// Checks for node availability and updates FreeNodes counter
/// </summary>
/// <returns>The node code</returns>
uint32_t VDisk::TakeFreeNode()
{
	if (!freeNodes) throw std::logic_error("Failed to find a free node");
	uint32_t curFree = maxNode - freeNodes;
	--freeNodes;
	return curFree;
}
void VDisk::UpdateBlockCounters(uint32_t count)
{
	nextFreeBlock+= count;
	freeBlocks-=count;
}
/// <summary>
/// Checks for blocks availability and updates FreeBlocks counter
/// </summary>
/// <returns>Address of the first allocated block</returns>
uint32_t VDisk::ReserveCluster()
{
	if (!freeBlocks) throw std::logic_error("Failed to get a free block");
	uint32_t curFree = nextFreeBlock;	
// todo: allow to 1) allocate less than a cluster, and 2) non-neighbour blocks
// 3) implement proper tracking of busy block, e.g. through bitmap
	nextFreeBlock = (nextFreeBlock + CLUSTER) > maxBlock ? throw std::logic_error("Failed to get a full cluster") : nextFreeBlock + CLUSTER;
	freeBlocks -= CLUSTER;
	return curFree;
}
void VDisk::TakeFreeBlock(File* f)
{
	if (!freeBlocks) throw std::logic_error("Failed to get a free block");
	f->AddDataBlock(nextFreeBlock);
	UpdateBlockCounters(1);
	if (NoSlotsInTB(f))
	{
		if (!freeBlocks) throw std::logic_error("Failed to get a new title block");
		InitTB(f, nextFreeBlock + 1);
		UpdateBlockCounters(1);
	}
}

bool VDisk::IsEmptyNode(short index)
{
	return (*ReadInfo(Sect::nd_name, index)) == char(0);	// Checking if name is null
}
bool VDisk::IsFileNode(short index)
{
	return (*ReadInfo(Sect::nd_meta, index) & 0b1000'0000) == 0;
}
uint32_t VDisk::GetNodeCode(short index)
{
	return CharToInt32(ReadInfo(Sect::nd_ncode, index));
}
uint32_t VDisk::GetChildAddr(short index)
{
	return CharToInt32(ReadInfo(Sect::nd_addr, index));
}

void VDisk::InitTB(File* file, uint32_t newTBAddr)
{
	// todo: move the writing to the UpdateDisk, build data charset here
	disk.SetBytes(file->GetLastTB() + addrMap[Sect::fd_nextTB], IntToChar(newTBAddr), ADDR);
	file->SetLastTB(newTBAddr);
	disk.SetBytes(newTBAddr + addrMap[Sect::fd_nextTB], IntToChar(newTBAddr), ADDR);
	disk.SetBytes(newTBAddr + addrMap[Sect::fd_realSize], IntToChar(file->GetSize()), 2 * ADDR);
	disk.SetBytes(newTBAddr + addrMap[Sect::fd_firstDB], IntToChar(newTBAddr), ADDR);

	uint64_t totalBlocks = file->CountDataBlocks();
	for (int i = 0; i < totalBlocks; ++i)
		disk.SetBytes(GetAbsoluteAddrInBlock(file->GetMainTB(), addrMap[Sect::fd_firstDB] + i * ADDR), IntToChar(file->GetDataBlock(i)), ADDR);
}
bool VDisk::ExpandIfLT(File* f, size_t len)
{
	uint32_t freeSpace = f->GetRemainingSpace();
	if (freeSpace < len)
	{
		uint32_t required = f->EstimateBlocksNeeded(len);
		while (required)
		{
			try
			{
				TakeFreeBlock(f);
				--required;
			}
			catch (...)
			{
				return true;
			}
		}
	}
	return false;
}
/// <summary>
/// Byte number from the beginning of the disk
/// </summary>
uint64_t VDisk::GetAbsoluteAddrInBlock(uint32_t blockAddr, uint32_t offset) const
{
	return addrMap[Sect::s_blocks] + blockAddr * BLOCK + offset;
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

void VDisk::PrintTree()
{
	std::cout << root->PrintVerticeTree();
}

/// <summary>
/// Checks if the file exists on disk, tracking down path through the Node section
/// </summary>
/// <returns>File* if file exists, nullptr otherwise</returns>
File* VDisk::SeekFile(const char* name) const
{
	try
	{
		return (File*)root->GetData(name);
	}
	catch (std::logic_error& e)
	{
		std::cout << e.what() << std::endl;
		return nullptr;
	}
}
uint32_t VDisk::AllocateNew(File* f, uint32_t nBlocks)
{
	uint32_t allocated = 0;
	while (allocated!=nBlocks)
	{
		try
		{
			TakeFreeBlock(f);
			++allocated;
		}
		catch (...)
		{
			break;
		}
	}
	return allocated;
}
bool VDisk::NoSlotsInTB(File* f)
{
	// "2" is the difference between MainTB and SecondaryTB tech data length in bytes
	return (f->CountDataBlocks() + 2) % BLOCK == 0;
}

void VDisk::LoadFile(File* f)
{
	uint32_t main, last, next, addr;
	last = main = f->GetMainTB();
	char* rawaddr;
	while (true)	// == through different Title Blocks
	{
		size_t pos = std::get<0>(GetPosLen(Sect::fd_s_firstDB, last));
		for (short i = 0; i != BLOCK/ADDR-1; ++i)	// == within same Title Block
		{
			if (main == last && i < 2) continue;	// Skip difference between Main and Secondary TBs

			rawaddr = new char[ADDR];
			disk.GetBytes(pos + i*ADDR, rawaddr, ADDR);
			addr = CharToInt32(rawaddr);
			delete[] rawaddr;

			if (addr == main) break;

			f->AddDataBlock(addr);
		}

		next = CharToInt32(ReadInfo(Sect::fd_nextTB, main));
		if (next == last) break;
		last = next;
	}

	f->SetLastTB(last);
	f->IncreaseSize(CharToInt64(ReadInfo(Sect::fd_realSize, main)));
}
/// <summary>
/// Reserves and initializes space for a new file. 
/// Writes all data required to the disk.
/// </summary>
File* VDisk::CreateFile(const char* name)
{
	try
	{
		uint32_t freeNode = TakeFreeNode();
		File* f = new File(freeNode, ReserveCluster(), name, this->name);
		Node* t = f;
		root->Add(name, t);
		InitTB(f, f->GetLastTB());
		UpdateDisk();
		return f;
	}
	catch (std::logic_error& e)
	{
		std::cout << e.what();
		return nullptr;
	}
}

size_t VDisk::WriteInFile(File* f, char* buff, size_t len)
{
	ExpandIfLT(f, len);
	len = std::min(f->GetRemainingSpace(), len);
	size_t wrote = 0;
	while (wrote!=len)
	{
		try
		{
			uint32_t ilen = std::min(BLOCK - f->Fseekp(), len-wrote);
			uint32_t pos = std::get<0>(GetPosLen(Sect::s_blocks, f->GetCurDataBlock())) + f->Fseekp();
			disk.SetBytes(pos, &buff[wrote], ilen);
			f->IncreaseSize(ilen);
			wrote += ilen;
			pos = std::get<0>(GetPosLen(Sect::fd_realSize, f->GetMainTB()));
			disk.SetBytes(pos, IntToChar(f->GetSize()), 2 * ADDR);
		}
		catch (...)
		{
			break;
		}
	}
	return wrote;
}

/// Loading existing disk 
VDisk::VDisk(const std::string fileName):
	name(fileName),
	sizeInBytes(GetDiskSize(fileName)),
	maxNode(CharToInt32(OpenAndReadInfo(fileName,addrMap[Sect::dd_maxNode],ADDR))),
	maxBlock(EstimateBlockCapacity(sizeInBytes))
{
	addrMap[Sect::s_blocks] = DISKDATA + maxNode * NODEDATA;
	disk.Open(fileName);
	root = LoadHierarchy();
	freeNodes = CharToInt32(ReadInfo(Sect::dd_fNodes));
	freeBlocks = CharToInt32(ReadInfo(Sect::dd_fBlks));
	nextFreeBlock = CharToInt32(ReadInfo(Sect::dd_nextFreeBlk));
	std::cout << "Disk \"" << name << "\" opened\n";
}
/// Creating new disk 
VDisk::VDisk(const std::string fileName, const uint64_t size) :
	name(fileName),
	maxNode(EstimateNodeCapacity(size)),
	maxBlock(EstimateBlockCapacity(size)),
	sizeInBytes(EstimateMaxSize(size))
{
	addrMap[Sect::s_blocks] = DISKDATA + maxNode * NODEDATA;
	disk.Open(fileName, true);
	root = new Vertice<Node*>();
	if (!InitDisk()) throw std::runtime_error("Initialization failed");
	else std::cout << "Disk \"" << name << "\" initialized\n";
}
VDisk::~VDisk()
{
	UpdateDisk();
	disk.Close();
	root->Destroy();
	std::cout << "Disk \"" << name << "\" closed\n";
}

/* ---VFS------------------------------------------------------------------- */

bool VFS::IsValidSize(size_t size)
{
	return (size >= (DISKDATA + NODEDATA + CLUSTER * BLOCK) && size <= UINT32_MAX);
}

bool VFS::MountOrCreate(std::string& diskName)
{
	bool mountSuccessful = false;
	
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
			throw std::invalid_argument("Bad input. Try again");
			break;
		}
	}
	else
	{
		std::cout << "Mounted disk \"" << diskName << "\" to the VFS\n";
		VFS::disks.push_back(new VDisk(diskName));
		mountSuccessful = true;
	}

	return mountSuccessful;
}
bool VFS::Unmount(const std::string& diskName)
{
	auto disk = GetDisk(diskName);
	if (disk!=disks.end())
	{
		delete (*disk);
		disks.erase(disk);
		std::cout << "Disk \"" << diskName << "\" was unmounted\n";
		return true;
	}
	else
	{
		std::cout << "Disk \"" << diskName << "\" not found\n";
		return false;
	}
}
VDisk* VFS::GetMostFreeDisk()
{
	if (!disks.size()) return nullptr;
	VDisk* chosenDisk = nullptr;
	uint32_t max_blocks = 0;
	for (const auto& disk : disks)
	{
		uint32_t cur_nodes = disk->GetNodesLeft();
		uint32_t cur_blocks = disk->GetBlocksLeft();
		if (!cur_nodes || !cur_blocks) continue;
		if (disk->GetBlocksLeft() > max_blocks)
		{
			max_blocks = disk->GetBlocksLeft();
			chosenDisk = disk;
		}
	}
	return chosenDisk;
}
std::vector<VDisk*>::iterator VFS::GetDisk(std::string name)
{
	for (auto iter = disks.begin(); iter != disks.end(); ++iter)
	{
		if ((*iter)->GetName() == name) return iter;
		
	}
	return disks.end();
}

/// <summary>
/// Opens the file in readonly mode. 
/// </summary>
/// <returns>nullptr if the file is already open or does not exist.</returns>
File* VFS::Open(const char* name)
{
	std::cout << "Trying to open: " << name << " ->";
	File* file = nullptr;
	for (const auto& disk : disks)
	{
		file = disk->SeekFile(name);
		if (file && !(file->IsWriteMode()))
		{
			std::cout << "successful" << name << std::endl;
			return file;
		}
	}
	std::cout << "failed" << std::endl;
	return file;
}
/// <summary>
/// Opens or creates the file in writeonly mode. 
/// </summary>
/// <returns>nullptr if the file is already open.</returns>
File* VFS::Create(const char* name)
{
	std::cout << "Trying to open " << name << " (write mode) -> ";
	File* file = nullptr;

	if (disks.empty()) throw std::out_of_range("No disks mounted");
	else
	{
		VDisk* mostFreeDisk = disks[0];
		for (const auto& disk : disks)
		{
			file = disk->SeekFile(name);
			if (file && !(file->IsBusy()))
			{
				std::cout << "file found" << std::endl;
				file->FlipWriteMode();
				return file;
			}
		}
		std::cout << "file not found, creating ..." << std::endl;
		file = GetMostFreeDisk()->CreateFile(name);

		if (file)
		{
			// debug code
			std::cout << "dbg-----------------------------------" << std::endl;
			mostFreeDisk->PrintTree();
			std::cout << "dbg-----------------------------------" << std::endl;
			// <- debug ends here
		}
		std::cout << "Created file: " << name << std::endl;
	}
	return file;
}
size_t VFS::Read(File* f, char* buff, size_t len)
{
	// TBD
	return size_t();
}
size_t VFS::Write(File* f, char* buff, size_t len)
{
	std::cout << "Writing in file: " << f->GetName() << std::endl;
	VDisk* vd = (*GetDisk(f->GetFather()));
	if (!vd) throw std::runtime_error("No disk found for the file");
	return vd->WriteInFile(f, buff, len);
}
void VFS::Close(File* f)
{
	std::cout << "Clsoing file: " << f->GetName() << std::endl;
	if (f->IsBusy())
	{
		if (f->IsWriteMode())
		{
			f->FlipWriteMode();
			std::cout << "File \"" << f->GetName() << "\": writing closed\n";
		}
		else
		{
			f->RemoveReader();
			std::cout << "File \"" << f->GetName() << "\": "<< f->GetReaders() <<" readers remain\n";
		}
	}
	std::cout << "File closed: " << f->GetName() << std::endl;
}

VFS::VFS()
{
	std::cout << "\nVFS created\n";
}
VFS::~VFS()
{
	for (auto iter = disks.begin(); iter != disks.end(); ++iter)
	{
		delete (*iter);
	}
	std::cout << "\nOver and out\n";
}

/* ---Misc------------------------------------------------------------------ */

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
uint64_t CharToInt64(const char* bytes)
{
	// todo: make template
	const int mask = 0xFF;
	uint64_t data = 0;

	for (int i = 0; i != sizeof(uint64_t); ++i)
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
uint64_t GetDiskSize(std::string filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}

void TreeToPlain(std::vector<char*>& info, Vertice<Node*>& tree, uint32_t& nodecode)
{
	// !todo: think of better implementation - tree logic should be hidden
	if (!tree._children.empty())
	{
		for (auto iter = tree._children.begin(); iter != tree._children.end(); ++iter)
			info.push_back(&(*(*((*iter).second.first))->NodeToChar(nodecode)));
		for (auto iter = tree._children.begin(); iter != tree._children.end(); ++iter)
			if ((*iter).second.second)
			{
				++nodecode;
				TreeToPlain(info, *(*iter).second.second, nodecode);
			}
	}
}