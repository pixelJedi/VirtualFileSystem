# VirtualFileSystem
The VFS (Virtual File System) is used for managing thousands of files packed in a few physical ones, which can be useful in systems with read/write limitations. This implementation is being developed mostly for educational purposes.

Currently, the following constants are used:
```
BYTE		= 8 bits
BLOCK		= 1024 bytes	memory for files is allocated in blocks
CLUSTER		= 16 blocks		cluster is reserved per file when created
ADDR		= 4 bytes		4-byte addresses are used, stored as uint32_t
DISKDATA	= 4*ADDR bytes	(see below)
NODEDATA	= 64 bytes		(see below)
```

## VFS
![VDisk internals](/VirtualFileSystem_Description/VFS.png)

### Operating
The VFS class inherits IVFS interface which supports the following actions:
- Open:		open an existing file for reading
- Create:	open a new file for writing
- Read:		load bytes from the file to the buffer
- Write:	load bytes from the buffer to the file
- Close:	close file

### VDisk handling
Each VFS can manage multiple VDisks (each is a file that represents a separate file hierarhy), but the user doesn't have to be aware of the details. The VFS manages all VDisk operations.
It's possible to mount and unmount VDisks by providing names of corresponding files:
- MountOrCreate(string filename)
- Unmount(string filename)

## VDisk

Each VDisk is assosiated with a physical file in the underlying file system.

### Creating
- When created, the VDisk object checks the physical file provided and loads specific addresses and data, before it's ready for reading/writing files.
- If the file with the name provided doesn't exist yet, the VDisk creates it and initializes based on the size estimation.

### Implementation
![VDisk internals](/VirtualFileSystem_Description/VDisk.png)

The VDisk consists of 3 main data sections:
1. **Disk Data**. Saves the general data about the VDisk:
	- Free Nodes: 		the number of new files/directory that can be created on VDisk (each Node represents a file or directory)
	- Free Blocks: 		how many data blocks the VDisk is ready to allocate
	- Max Node: 		the limit on nodes is estimated during the initialization (~16 Kbytes per file estimated)
	- Next Free Block:	the design assumes that data is written to sequential blocks where possible, so that the variable here stores the address of the next free cluster (or block if clusters are unavailable)
2. **Node Data**. Saves the file hierarchy, address of title block and manages access. Each node represents a relation:
	- Node Code:		a numeric value in address format, represents a "parent"
	- Metadata bitset [8 bits]:
		- [1] folder (0) or file (1)
		- [2] ---
		- [1] writeonly flag
		- [4] readonly counter (multiple threads can read the same file)
	- File Name:		filename of a child 
	- File Address:		if child is a dir, stores the child's node code, else the file title block's address
3. **Block Data**. 
	- Each BLOCK is a fixed number of bytes.
		- A File can take multiple blocks (sequential if possible, but that's not obligatory)
		- First block of the file is a Title block that stores name and block adresses of the file
	- Each CLUSTER is a fixed number of blocks. A whole cluster is reserved per file even is less space is actually required

## File

Physically, a File is stored in blocks on VDisk. It consists of title and data blocks.
- **Title blocks** store addresses of data blocks. First title block is the main one; it also stores some other information
- **Data blocks** store data

As a struct, the File is intended to provide quick access to the data. When a File is opened, the vector of block addresses is loaded. To prevent collisions, flags are set in the file's node.

### Title block
![VDisk internals](/VirtualFileSystem_Description/TitleBlocks.png)

Title blocks [TB] are made this way:
1. File's info area:
	- `Next TB`:		address of the succeeding TB. If there is no one, the TB addresses itself
	- `Size`:			real size (in bytes) of the data written in the file
2. File's data addresses area.
	- The address after the last one used addresses TB
	- If there is no space for new addresses, a new TB is created, its address is written to the `Next TB` byte.
	
### Data block
`Note. Currently it's hard to effectively track available blocks. The idea is to use bitmaps and store it in the Disk Data section`
Data block stores binary data.
When a new block is required, the VDisk is checked for relevant info on available blocks.