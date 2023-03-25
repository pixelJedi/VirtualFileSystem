#include "IVFS.h"

#include <sstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <bitset>
#include <queue>

/* ---File------------------------------------------------------------------ */

std::ostream& operator<<(std::ostream& s, const File& node)
{
	return s << "[" << node._mainTB << "] "<< node.CountDataBlocks() << " dblocks, " << node._realSize << " bytes" ;
}

char File::BuildFileMeta()
{
	std::bitset<8> metabitset(_readmode_count);
	if (_writemode) metabitset.set(4);
	return static_cast<char>(metabitset.to_ulong());
}

void File::AddReader()
{ 
	_readmode_count = _readmode_count < MAX_READERS ? _readmode_count + 1 : MAX_READERS;
}
void File::RemoveReader() 
{ 
	_readmode_count = _readmode_count > 0 ? _readmode_count - 1 : 0;
}

/// <returns>The number of blocks needed to be added to fit data</returns>
uint32_t File::EstimateBlocksNeeded(size_t dataLength) const
{
	long long difference = dataLength - GetRemainingSize();
	return difference > 0 ? uint32_t(std::ceil(difference * 1.0 / BLOCK)) : 0;
}
short File::STBCapacity()
{
	return BLOCK / ADDR - 1;
}
/// <returns>Remaining capacity of the active (last) TB</returns>
uint32_t File::CountSlotsInTB() const
{
	short sTBCap = STBCapacity();
	short remCap = sTBCap - (blocks.size() + TBDIFF) % sTBCap;
	return remCap == sTBCap ? 0 : remCap;
}

std::string File::ParseLast(std::string path, char delim) const
{
	auto pos = path.find_last_of(delim);
	return pos < path.length() ? std::string{ path.substr(pos + 1, path.length()) } : path;
}

char* File::NodeToChar(uint32_t nodeCode)
{
	char* newNode = new char[NODEDATA];
	short i, ibyte = 0;
	// Node code
	for (i = 0; i < ADDR; ++i, ++ibyte)
		newNode[ibyte] = IntToChar(nodeCode)[i];
	// Metadata
	newNode[ibyte] = BuildFileMeta(); ++ibyte;
	// Name
	std::string name = _name;
	name.resize(NODENAME);
	for (i = 0; i < NODENAME; ++i, ++ibyte)
		newNode[ibyte] = name[i];
	// File address
	for (i = 0; i < ADDR; ++i, ++ibyte)
		newNode[ibyte] = IntToChar(_mainTB)[i];
	return newNode;
}

File::File(uint32_t blockAddr, std::string name, std::string fathername)
{
	_name = ParseLast(name);
	_fathername = fathername;
	_realSize = 0;
	_writemode = false;
	_readmode_count = 0;
	_lastTB = _mainTB = blockAddr;
}

/* ---BinDisk--------------------------------------------------------------- */

bool BinDisk::SetBytes(size_t position, const char* data, size_t length)
{
	seekp(position, std::ios_base::beg);
	write(data, length);
	return true;
}
bool BinDisk::GetBytes(size_t position, char* data, size_t length)
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
/// <summary>
/// Opens the file in io mode
/// </summary>
/// <param name="asNew">Existing file contents will be erased</param>
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
/// <summary>
/// Recursively parses data from the Node data section, building tree
/// </summary>
/// <param name="start_index">The node address to start read. Used to jump between recursion levels</param>
/// <returns></returns>
Vertice<File*>* VDisk::LoadHierarchy(uint32_t start_index)
{
	/* Algorithm:
	1. Add root Vertice for this NC.
	2. Remember the NC (NodeCode) of node[start_index]: similar NC == children of the same node.
	3. Go through nodes, starting from start_index, while NC is similar.
	4.1 If is file, add File.
	4.2 If is dir, add Dir
	4.2.1 Call LoadHierarchy(<child's node addr as parameter>) and bind the result to root.
	*/

	Vertice<File*>* root = new Vertice<File*>();	// [1]
	uint32_t curr_nc = GetNodeCode(start_index);	// [2]

	for (uint32_t i = start_index; i != maxNode - freeNodes && curr_nc == GetNodeCode(i); ++i)  // [3]
	{
		File* node = nullptr;
		char* filename = ReadInfo(Sect::nd_name, i);
		uint32_t addr = GetNodeChild(i);

		bool isFile = GetIsFile(i);
		if (isFile)
		{
			node = new File(addr, filename, this->name);
			LoadFile(node);
		}
		root->Add(std::string{ filename }, node);	// [4]
		if (!isFile)
			root->BindNewTreeToChild(filename, LoadHierarchy(addr));
	}

	return root;
}
/// <summary>
/// Converts tree fo plain array of Node data
/// </summary>
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
void VDisk::UpdateTBs(File* f)
{
	uint32_t cur = f->GetMainTB(), next, last = f->GetLastTB();
	size_t pos, len;

	if (last == cur)	// The case when MainTB is initialized
	{
		std::tie(pos, len) = GetPosLen(Sect::fd_nextTB, f->GetMainTB());
		disk.SetBytes(pos, IntToChar(cur), len);
	}

	uint32_t totalBlocks = f->CountDataBlocks();
	uint32_t j, i = 0;
	while (true)
	{
		uint32_t count = std::min(totalBlocks-i,uint32_t(File::STBCapacity()));
		pos = std::get<0>(GetPosLen(Sect::fd_s_firstDB, cur));
		if (cur == f->GetMainTB()) 
		{ 
			pos += TBDIFF * ADDR; 
			if (count == File::STBCapacity()) count -= TBDIFF;
		}

		for (j = 0; j < count; ++j)
			disk.SetBytes(pos + j * ADDR, IntToChar(f->GetDataBlock(i+j)), ADDR);
		i += count; 

		next = CharToInt32(ReadInfo(Sect::fd_nextTB, cur));
		if (cur == next)
		{
			if (j != File::STBCapacity()) disk.SetBytes(pos + j * ADDR, IntToChar(cur), ADDR);
			break;
		}
		cur = next;
	}
}
/// <summary>
/// Saves the VDisk stats
/// </summary>
void VDisk::UpdateDisk()
{
	disk.SetBytes(addrMap[Sect::dd_fNodes], IntToChar(freeNodes), ADDR);
	disk.SetBytes(addrMap[Sect::dd_fBlks], IntToChar(freeBlocks), ADDR);
	disk.SetBytes(addrMap[Sect::dd_maxNode], IntToChar(maxNode), ADDR);
	disk.SetBytes(addrMap[Sect::dd_nextFreeBlk], IntToChar(nextFreeBlock), ADDR);
	WriteHierarchy();
	// Debug: the fstream is reopened in order to update values in hex editor
	disk.Close();
	disk.Open(name);
	// <- eof Debug
	std::cout << "Disk \"" << name << "\" updated\n";
}
/// <summary>
/// Returns the absolute address and length of the specific data
/// </summary>
/// <param name="info">VDisk::Sect alias</param>
/// <param name="i"></param>
/// <returns>0 - absolute position, 1 - length of stored variable</returns>
std::tuple<uint32_t, uint32_t> VDisk::GetPosLen(Sect info, uint32_t offset)
{
	uint32_t pos, len;
	switch (info)
	{
	case Sect::s_nodes:
		pos = addrMap[info] + offset * NODEDATA;
		len = NODEDATA;
		break;
	case Sect::s_blocks:
		pos = addrMap[info] + offset * BLOCK;
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
		pos = addrMap[Sect::s_nodes] + offset * NODEDATA + addrMap[info];
		len = ADDR;
		break;
	case Sect::nd_meta:
		pos = addrMap[Sect::s_nodes] + offset * NODEDATA + addrMap[info];
		len = NODEMETA;
		break;
	case Sect::nd_name:
		pos = addrMap[Sect::s_nodes] + offset * NODEDATA + addrMap[info];
		len = NODENAME;
		break;
	case Sect::fd_nextTB:
	case Sect::fd_firstDB:
	case Sect::fd_s_firstDB:
		pos = addrMap[Sect::s_blocks] + offset * BLOCK + addrMap[info];
		len = ADDR;
		break;
	case Sect::fd_realSize:
		pos = addrMap[Sect::s_blocks] + offset * BLOCK + addrMap[info];
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
/// <returns>The first free node id</returns>
uint32_t VDisk::TakeNode(uint32_t size)
{
	if (freeNodes < size) throw std::logic_error("Failed to find a free node");
	uint32_t curFree = maxNode - freeNodes;
	freeNodes -= size;
	return curFree;
}

// To rework: doesn't subtracts nodes that already exist
uint32_t VDisk::EstimateNodes(const std::string_view path)
{
	uint32_t count = 1;
	for (uint32_t i = 0; i < path.size(); i++)
		if (path[i] == '\\') count++;
	return count;
}

void VDisk::UpdateBlockCounters(uint32_t count)
{
	nextFreeBlock+= count;
	freeBlocks-=count;
}

/// <summary>
/// Tries to add as many Data Blocks for File f, as specified.
/// </summary>
/// <returns>The number of blocks really allocated</returns>
uint32_t VDisk::RequestDBlocks(File* f, uint32_t number)
{
	uint32_t firstfree, maxTB = 0, maxDB = 0;
	maxTB = (std::min(number, freeBlocks) - std::min(std::min(f->CountSlotsInTB(), number), freeBlocks)) / File::STBCapacity();
	maxDB = std::min(std::min(number, freeBlocks - maxTB), f->CountSlotsInTB() + maxTB * (File::STBCapacity() + 1));
	if (number > maxDB && freeBlocks - maxDB - maxTB > 1) { ++maxTB; maxDB += std::min(freeBlocks - maxDB - maxTB, number - maxDB); }
	if (maxDB) firstfree = nextFreeBlock;
	UpdateBlockCounters(maxDB);
	for (uint32_t i = 0; i < maxDB; ++i) f->AddDataBlock(firstfree + i);
	for (uint32_t i = 0; i < maxTB; ++i) AppendTB(f, ReserveOneBlock());
	return maxDB;
}
/// <summary>
/// Allocates one free block
/// </summary>
/// <returns>The id of the block </returns>
uint32_t VDisk::ReserveOneBlock()
{
	if (!freeBlocks) throw std::runtime_error("Failed to get a free block");
	uint32_t firstfree = nextFreeBlock;
	UpdateBlockCounters();
	return firstfree;
}

void VDisk::AppendTB(File* file, uint32_t addr)
{
	disk.SetBytes(addrMap[Sect::s_blocks] + file->GetLastTB() * BLOCK + addrMap[Sect::fd_nextTB], IntToChar(addr), ADDR);
	file->SetLastTB(addr);
	disk.SetBytes(addrMap[Sect::s_blocks] + addr * BLOCK + addrMap[Sect::fd_nextTB], IntToChar(addr), ADDR);
}

bool VDisk::GetIsFile(short index)
{
	return (*ReadInfo(Sect::nd_meta, index) & 0b1000'0000) == 0;
}
uint32_t VDisk::GetNodeCode(short index)
{
	return CharToInt32(ReadInfo(Sect::nd_ncode, index));
}
uint32_t VDisk::GetNodeChild(short index)
{
	return CharToInt32(ReadInfo(Sect::nd_addr, index));
}
/// <summary>
/// Initializes TB by writing file data to the VDisk
/// </summary>
/// TO REWORK
void VDisk::FillTBs(File* file, uint32_t newTBAddr)
{
	AppendTB(file, newTBAddr);
	uint32_t totalBlocks = file->CountDataBlocks();
	if (file->GetMainTB() == newTBAddr)
	{
		disk.SetBytes(addrMap[Sect::s_blocks] + newTBAddr * BLOCK + addrMap[Sect::fd_realSize], IntToChar(file->GetSize()), 2 * ADDR);
		for (short i = 0; uint32_t(i) < totalBlocks; ++i)
			disk.SetBytes(addrMap[Sect::s_blocks] + newTBAddr * BLOCK + addrMap[Sect::fd_firstDB] + i * ADDR, IntToChar(file->GetDataBlock(i)), ADDR);
	}
	disk.SetBytes(addrMap[Sect::s_blocks] + newTBAddr * BLOCK + addrMap[Sect::fd_firstDB] + totalBlocks * ADDR, IntToChar(newTBAddr), ADDR);
}
/// <summary>
/// Evaluates blocks needed to fit [len] bytes and allocates them
/// </summary>
/// <returns>True, if the file was expanded</returns>
bool VDisk::ExpandIfLT(File* f, size_t len)
{
	if (f->GetRemainingSize() < len)
	{
		bool changed = RequestDBlocks(f, f->EstimateBlocksNeeded(len)) != 0;
		UpdateTBs(f);
		return changed;
	}
	return false;
}

/*
-------------------------------------------
VDisk {name} usage:
Nodes used:		{x} of {x} ({x.xx}%)
Blocks used:	{x} of {x} ({x.xx}%)

Free dataspace:	{x} of {x} bytes ({x.xx}%)
-------------------------------------------
*/
std::ostream& operator<<(std::ostream& s, const VDisk& disk)
{
	const short ALIGN = 9;
	const std::string SEP = " of ";
	size_t freeSpace = disk.freeBlocks * BLOCK;
	s.precision(2);
	s << std::fixed;
	s << "*************************************************\n";
	s << "VDisk \"" << disk.name << "\" usage:\n";
	s << " Free space  :\t" << Aligned(freeSpace, ALIGN) << SEP << disk.sizeInBytes << " bytes (" << ((freeSpace * 1.0) / disk.sizeInBytes) * 100 << "%)\n";
	s << " Nodes used  :\t" << Aligned(disk.maxNode - disk.freeNodes, ALIGN) << SEP << disk.maxNode << " (" << ((disk.maxNode - disk.freeNodes) * 1.0 / disk.maxNode) * 100 << "%)\n";
	s << " Blocks used :\t" << Aligned(disk.maxBlock - disk.freeBlocks, ALIGN) << SEP << disk.maxBlock << " (" << ((disk.maxBlock - disk.freeBlocks) * 1.0 / disk.maxBlock) * 100 << "%)\n";
	s << "-------->              TREE             <--------\n";
	s << disk.root->PrintVerticeTree(true);
	s << "*************************************************\n";

	return s;
}

/// <summary>
/// Checks if the file exists on disk, tracking down path through the Node section
/// </summary>
/// <returns>File* if file exists, nullptr otherwise</returns>
File* VDisk::SeekFile(const char* path) const
{
	try
	{
		return (File*)root->GetData(path);
	}
	catch (std::logic_error& e)
	{
		std::cout << e.what() << std::endl;
		return nullptr;
	}
}

/// <summary>
/// Loads File struct's blocks, size and TB addresses from the binary data
/// </summary>
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
			if (main == last && i < TBDIFF) continue;

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

bool VDisk::CanCreateFile(uint32_t nodes, uint32_t blocks)
{
	return freeNodes >= nodes && freeBlocks >= blocks;
}

/// <summary>
/// Reserves and initializes space for a new file. 
/// Writes all data required to the disk.
/// </summary>
File* VDisk::CreateFile(const char* path)
{
	File* f;
	uint32_t reqNodes = EstimateNodes(path);
	try
	{
		std::lock_guard<std::mutex> lockn(nodeReserve);
		std::lock_guard<std::mutex> lockb(blockReserve);
		std::lock_guard<std::mutex> lockt(tree);
		if (CanCreateFile(reqNodes))
		{
			TakeNode(reqNodes);
			f = new File(ReserveOneBlock(), path, this->name);
			root->Add(path, f);
			RequestDBlocks(f, CLUSTER-1);
			UpdateTBs(f);
			UpdateDisk();
		}
		else
			f = nullptr;
	}
	catch (std::logic_error& e)
	{
		std::cout << e.what();
		f = nullptr;
	}
	return f;
}

size_t VDisk::WriteInFile(File* f, char* buff, size_t len)
{
	ExpandIfLT(f, len);
	len = std::min(f->GetRemainingSize(), len);

	size_t pos, wrote = 0;
	while (wrote!=len)
	{
		size_t ilen = std::min(size_t(BLOCK - f->Fseekp()), len-wrote);
		pos = std::get<0>(GetPosLen(Sect::s_blocks, f->GetCurDataBlock())) + f->Fseekp();
		disk.SetBytes(pos, &buff[wrote], ilen);
		f->IncreaseSize(ilen);
		wrote += ilen;
	}
	pos = std::get<0>(GetPosLen(Sect::fd_realSize, f->GetMainTB()));
	disk.SetBytes(pos, IntToChar(f->GetSize()), 2 * ADDR);
	return wrote;
}

size_t VDisk::ReadFromFile(File* f, char* buff, size_t len)
{
	len = std::min(f->GetSize(), len);
	size_t read = 0;
	int i = 0;
	while (read != len)
	{
		size_t pos = addrMap[Sect::s_blocks] + f->GetDataBlock(i) * BLOCK;
		size_t ilen = std::min(size_t(BLOCK), len - read);
		disk.GetBytes(pos, &buff[read], ilen);
		read += ilen;
		++i;
	}
	return read;
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
	freeNodes = CharToInt32(ReadInfo(Sect::dd_fNodes));
	freeBlocks = CharToInt32(ReadInfo(Sect::dd_fBlks));
	nextFreeBlock = CharToInt32(ReadInfo(Sect::dd_nextFreeBlk));
	root = LoadHierarchy();

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
	root = new Vertice<File*>();

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
			throw std::invalid_argument("Bad input. Try again -> ");
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
	// todo: unsafe; the function doesn't check for full paths whcih may or may not require more than 1 node
	VDisk* chosenDisk = nullptr;
	if (disks.size())
	{
		uint32_t max_blocks = 0;
		std::lock_guard<std::mutex> lock(diskSelection);
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
	std::cout << "* Trying to open " << name << " (read mode) -> ";
	File* file = nullptr;
	for (const auto& disk : disks)
	{
		file = disk->SeekFile(name);
		std::lock_guard<std::mutex> lock(readAccessCheck);
		if (file && !(file->IsWriteMode()) && file->GetReaders() < file->MAX_READERS)
		{
			file->AddReader();
			std::cout << "opened" << std::endl;
			return file;
		}
	}
	std::cout << "failed" << std::endl;
	return nullptr;
}
/// <summary>
/// Opens or creates the file in writeonly mode. Creates transit directories.
/// </summary>
/// <returns>nullptr if the file is already open.</returns>
File* VFS::Create(const char* name)
{
	std::cout << "* Trying to open " << name << " (write mode) -> ";
	if (disks.empty()) throw std::out_of_range("No disks mounted");

	File* file = nullptr;

	// Searching for an existing file
	for (const auto& disk : disks)
	{
		file = disk->SeekFile(name);
		if (file) break;
	}

	// Creating new file
	if (!file)
	{
		std::cout << "file not found -> ";
		VDisk* mostFreeDisk = GetMostFreeDisk();
		if (mostFreeDisk)
		{
			file = mostFreeDisk->CreateFile(name);

			if (file) std::cout << "created -> ";
			else std::cout << "failed to create\n";
		}
		else std::cout << "no space left\n";
	}
	else std::cout << "file found -> ";

	// Flagging write access
	if (file)
	{
		std::lock_guard<std::mutex> waccess(writeAccessCheck);
		if (!(file->IsBusy()))
		{
			file->FlipWriteMode();
			std::cout << "opened\n";
		}
		else
		{
			file = nullptr;
			std::cout << "is already busy\n";
		}
	}

	return file;
}
/// <summary>
/// Reads bytes starting from the beginning of the file.
/// </summary>
/// <returns>Number of bytes actually read</returns>
size_t VFS::Read(File* f, char* buff, size_t len)
{
	std::cout << "* Reading file: " << f->GetName() << " -> ";
	if (f->IsWriteMode()) {
		std::cout << "File is open in writemode" << std::endl;
		return 0;
	}
	VDisk* vd = (*GetDisk(f->GetFather()));
	if (!vd) throw std::runtime_error("No disk found for the file");
	return vd->ReadFromFile(f, buff, len);
}
/// <summary>
/// Writes bytes to the end of the file.
/// </summary>
/// <returns>Number  of bytes actually written</returns>
size_t VFS::Write(File* f, char* buff, size_t len)
{
	std::cout << "* Writing in file: " << f->GetName() << " -> ";
	VDisk* vd = (*GetDisk(f->GetFather()));
	if (!vd) throw std::runtime_error("No disk found for the file");
	return vd->WriteInFile(f, buff, len);
}
void VFS::Close(File* f)
{
	std::cout << "* Closing file: " << f->GetName() << " -> ";
	if (f->IsBusy())
	{
		if (f->IsWriteMode())
		{
			f->FlipWriteMode();
			std::cout << "writing closed\n";
		}
		else
		{
			f->RemoveReader();
			std::cout << f->GetReaders() <<" readers remain\n";
		}
	}
}

void VFS::PrintAll()
{
	for (auto iter = disks.begin(); iter != disks.end(); ++iter)
		std::cout << *(*iter);
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
	const int mask = 0xFF;
	uint64_t data = 0;

	for (int i = 0; i != sizeof(uint64_t); ++i)
	{
		data *= 0b100000000;
		data += (bytes[i] & mask);
	}
	return data;
}
/// <summary>
/// Dirty hack used to initialize VDisk constants which values are stored in the file which should be opened only after the initialization.
/// </summary>
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
/// <summary>
/// Opens an existing file and calculates the size. 
/// </summary>
uint64_t GetDiskSize(std::string filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}
/// <summary>
/// Dirty hack for making two classes that depend on each other - Vertice and VDisk - communicate.
/// </summary>
void TreeToPlain(std::vector<char*>& info, Vertice<File*>& tree, uint32_t& nodecode)
{
	// !todo: think of better implementation - tree logic should be hidden
	if (!tree._children.empty())
	{
		std::queue<uint32_t> dirs;
		for (auto iter = tree._children.begin(); iter != tree._children.end(); ++iter)
		{
			char* plainnode;
			if ((*iter).second.first)	// == is file
			{
				plainnode = &(*(*((*iter).second.first))->NodeToChar(nodecode));
			}
			else
			{
				plainnode = nullptr;	// have to update childaddr later
				dirs.push(uint32_t(info.size()));
			}
			info.push_back(plainnode);
		}
		uint32_t curdirnode = nodecode;
		for (auto iter = tree._children.begin(); iter != tree._children.end(); ++iter)
			if ((*iter).second.second)
			{
				auto dirnode = DirToChar(curdirnode, (*iter).first, uint32_t(info.size()));
				++nodecode;
				TreeToPlain(info, *(*iter).second.second, nodecode);
				info.at(dirs.front()) = dirnode;
				dirs.pop();
			}
	}
}

char* DirToChar(uint32_t nodecode, std::string name, uint32_t firstchild)
{
	char* newNode = new char[NODEDATA];
	short i, ibyte = 0;
	// Node code
	for (i = 0; i < ADDR; ++i, ++ibyte)
		newNode[ibyte] = IntToChar(nodecode)[i];
	// Metadata
	newNode[ibyte] = char(0b10000000); ++ibyte;
	// Name
	name.resize(NODENAME);
	for (i = 0; i < NODENAME; ++i, ++ibyte)
		newNode[ibyte] = name[i];
	// File address
	for (i = 0; i < ADDR; ++i, ++ibyte)
		newNode[ibyte] = IntToChar(firstchild)[i];
	return newNode;
}

std::string Aligned(size_t number, short length)
{
	std::ostringstream s;
	size_t t = number;
	short count = 1;
	while (t /= 10) ++count;
	s << std::string(length>=count?length-count:0,' ') << number;
	return s.str();
}