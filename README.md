# VirtualFileSystem
The VFS (Virtual File System) is used for managing thousands of files packed in a few physical ones, which can be useful in systems with read/write limitations. This implementation is being developed mostly for educational purposes.

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



