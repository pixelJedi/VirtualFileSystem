# VirtualFileSystem
The **VFS** (Virtual File System) is used for managing thousands of files packed in a few physical ones, which can be useful in systems with read/write limitations. This implementation is being developed mostly for educational purposes.

> :radioactive: The system is currently under development, use it on your own risk. The repo is public only because stars wanted it to be this way. All recent changes commited for for educational purposes only.<br/>
> The description below describes the general concept. If you want to know more, jump to the [FAQ](https://github.com/pixelJedi/VirtualFileSystem#faq).

The current implementation uses *command line interface* to prompt parameters and log intermediate results.

Currently, the following constants are used:

- `BYTE` = 8 bits;
- `BLOCK` = 1024 bytes: memory for files is allocated in blocks;
- `CLUSTER` = 16 blocks: cluster is reserved per file when created;
- `ADDR` = 4 bytes: 4-byte addresses are used, stored as uint32_t;
- `DISKDATA` = 4*ADDR bytes (see below);
- `NODEDATA` = 64 bytes = ADDR + NODEMETA + NODEDATA + ADDR (see below);
- `NODEMETA` = 1 byte;
- `NODENAME` = 55 bytes.

## VFS
![VDisk internals](/VirtualFileSystem_Description/VFS.png)

### Operating
The VFS class inherits IVFS interface which supports the following actions:

- [x] `Open`:   open an existing file for reading;
- [x] `Create`: open a new file for writing;
- [x] `Read`:   load bytes from the file to the buffer;
- [x] `Write`:  load bytes from the buffer to the file;
- [x] `Close`:  close file.

### VDisk handling
VFS can manage multiple [VDisks](https://github.com/pixelJedi/VirtualFileSystem#VDisk), stored in std::vector
Each VDisk has an assigned physical file. Before working with the file, the user should first `MountOrCreate` it.
It's possible to mount and unmount VDisks by providing names of corresponding files:
- [x] `MountOrCreate(string filename)`:		if not found, asks 1) if the new VDisk should be created, and 2) the size. The size can be truncated to accommodate an integer number of blocks calculated during the initial estimation;
- [x] `Unmount(string filename)`: 		closes the disk.


### Multithreading

VFS operations may be used by multiple threads. The shared data should be protected against collisions.
- The protected are VDisk variables: **freeBlocks**, **freeNodes** and **nextFreeBlock** as functions rely on these counters when allocating data. The protection is implemented as a simple mutex guard lock.
- Another thing to concern is the **file access status**. The guard wraps code where it's checked and changed.
- And also the file tree (VDisk::root) becomes locked when a new file is added. 
- Access to file blocks is not intended to be protected with mutex, as it's already safe with access flags. 

## VDisk
Each VDisk is assosiated with a physical file in the underlying file system.

### Key members
- `BinDisk disk`. BinDisk is derived from std::fstream and simplifies access to binary data. See [BinDisk](https://github.com/pixelJedi/VirtualFileSystem#BinDisk)
- `Vertice<File*>* root`. Represents the root of the file hierarhy. Read more on [Vertices](https://github.com/pixelJedi/VirtualFileSystem#Vertice) and [Nodes](https://github.com/pixelJedi/VirtualFileSystem#Node);
- `std::map<Sect, uint32_t> addrMap`. Stores all offsets to important data sections. Sect\[ion\] is a private enumerator.

### Constructors
- `VDisk(const std::string fileName)`: for existing files. The file is checked and specific addresses and data are loaded into VDisk object;
- `VDisk(const std::string fileName, const uint64_t size)`: initializes a new file. The size is estimated and truncated.
 
> Estimation rule: `DISKDATA + X*NODEDATA + (X*CLUSTER+C)*BLOCK` should fit in without remainder

### Data Sections
![VDisk internals](/VirtualFileSystem_Description/VDisk.png)

The VDisk consists of 3 main data sections:

1. **Disk Data**. Saves the general data about the VDisk:
	- `Free Nodes`: 	the number of new files/directory that can be created on VDisk (each Node represents a file or directory);
	- `Free Blocks`: 	how many data blocks the VDisk is ready to allocate;
	- `Max Node`: 		the limit on nodes is estimated during the initialization (~16 Kbytes per file estimated);
	- `Next Free Block`:	the design assumes that data is written to sequential blocks where possible, so that variable can speed up memory allocation. It equals the relative address of 1st free block.
2. **Node Data**. Saves the file hierarchy in a plain form. Each entry represents a parent-child relation:
	- `Node Code, or NC`:	a numeric 4-byte value. Nodes with similar NC belong to the same "parent";
	- `Metadata` bitset [8 bits][^1]:
		- [1] folder (1) or file (0)
		- [2] ---
		- [1] writeonly flag
		- [4] readonly counter (multiple threads can read the same file) 
	- `File Name`:		filename of a child. Starts with \0 when node is empty;
	- `File Address`:	if child is a dir, stores the child's node code, else the file title block's address.
3. **Block Data**. 
	- Each BLOCK is a fixed number of bytes;
		- A File can take multiple blocks (sequential if possible, but that's not obligatory);
		- First block of the file is a Title block that stores name and block adresses of the file. Additional title blocks can be provided;
	- Each CLUSTER is a fixed number of blocks. A whole cluster is reserved per file even if less space is actually required.
[^1]: Rework candidate. Currenty, only the folder/file flag is used. Accesses are handled by Nodes during runtime.

## BinDisk
Is a class derived from std::fstream.

### Read/write operations
Two low-level functions are responsible for the data:

- `GetBytes(size_t position, const char* data, size_t length)`
- `SetBytes(size_t position, const char* data, size_t length)`

Both use `seekg` and `seekp` pointers of the VDisk's fstream to get or owerwrite `data` of `length` starting from `position` in the file.
They're not const as changing pointers changes the fstream obj.

### Open/close operations
- `Open(const std::string fileName, bool asNew = false)`
- `Close()`

Both wrap corresponding fstream functions. `Open` accepts `asNew` parameter which defines if std::ios_base::trunc should be applied. 

### Also
- `MakeZeroFile(size_t size)`: fills the stream with null-terminator '\0'

## File
Is an struct which represents the data that is sufficient to manipulate a particular file in hierarchy.[^2]

Members:
- `std::string _fathername`: 	the name of hosting VDisk;
- `std::string _name`: 	node name;
- `realSize`: is used to calculate current pointer for writing, remaining space, etc.;
- `writemode` flag and `readmode` counter: are used to prevent threading collisions;
- `mainTB` and `lastTB` addresses: stored for quick access;
- `blocks`: loaded when the File is opened and updated during the runtime.
- `virtual char* NodeToChar(uint32_t nodeCode) = 0`: transforms node to binary, taking nodeCode from the outside.

`NodeToChar` is a key part in transforming the multidemensional file hierarchy into a linear list of nodes.
![Tree to Plain node correlation](/VirtualFileSystem_Description/TreeToPlain.png)

* Node Code is calculated during the conversion, it is not supposed to be stored during runtime.
* Addresses are needed when the data is loaded. During the runtime, the tree is stored in Vertices.

[^2]: Previously, an abstract Node class has been uses with File and Dir as children. It's been reworked on schedule, as soon as such a division proved to be excessive. 

### Concept
Physically, a File is stored in VDisk::disk in multiple blocks of data. The 1st one is addressed by the file's node in Node data section.
A File consists of title and data blocks.

- **Title blocks**, or TBs: list addresses of data blocks. First title block is the main one, it also stores size;
- **Data blocks**, or DBs: store data.

#### Title block
![VDisk internals](/VirtualFileSystem_Description/TitleBlocks.png)

Title blocks are made this way:
1. Info area:
	- `Next TB`:		address of the succeeding TB. If there is no one, the TB addresses itself;
	- `Size`:		real size (in bytes) of the data written in the file. Stored in the MainTB only.
2. DB list:
	- The address after the last one equals current TB address;
	- If there is no space for new addresses, a new TB is created, its address is written to the `Next TB`.
	
#### Data block
Data block stores binary data.
A __CLUSTER__ of blocks is allocated to a new file. __One__ block is allocated when an existing file runs out of space.

> **Note.** Is the task doesn't require deleting files, currently the blocks are allocated linearly and never released. Implementing Delete function will require an effective way to track available blocks, e.g. bitmaps.

## Vertice
Is a container node with named children, based on std::map, which is based on [RB tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree), so it's easy to:
- Search and add an element relatively fast (O(log)) within a single node
- Access files by their names
- Keep the lexicographic order which is good for hierarchy display

**Key member**
`std::map<std::string, std::pair<std::unique_ptr<T>, Vertice*>> _children` 

- The data is wrapped into the `std::unique_ptr` to handle memory allocation for different types of Nodes (Dirs and Files)

## FAQ
> **1. Can I read, write, open or create files using your VFS?**

Finally, yes! Some bugs are most likely hiding somewhere, but I can guarantee you're able to run the `runtest1` code in the project's main. Feel free to alter parameters commented with arrow (<--).

> **2. Why do you show us such a raw code?**

We all like to show off. I mean, hey, look at this awesome thing that I developed without knowing where to start a month ago! Joking. Check the updates.

> **3. Why so slow, dude?**

The main reason is lack of time and skill, as always are. I've overestimated my programming skills; the overall idea of VFS was clear, but I've drowned in low-level syntax debugging. Also I work and study full-time, but I think the skill issue here is the main one.

> **4. Okay, can I do something with VFS?**

Oh yeah! After some weeks you can not only mount and unmount VDisks, but also write and read data, save it between sessions, and also run a bunch of mid-level functions (although you're not supposed to use them directly, but anyway). You can run the test code several times, check the console output or open the hex editor and see that the data is on its place. Still pretty inspiring to see it working, y'know.

> **5. What about multithreading?**

There are trivial mutexes (wrapped in std::lock_guard) to prevent working with outdated data. I'm rather new to it, so maybe I'll learn a more effective way (I've heard mutex causes system calls which slow down the process). Mutexed protect block/node allocation, access changes and tree changes.

> **6. What was this about learning? Didn't you know what are you dealing with?**

Honestly, I've learned and updated a ton of things. I didn't use C++ since university so I've read a lot about working with pointers, mutexes, their different kinds, streams (expecially file streams), some syntax sugar of C++, etc., but I still have much to learn, and of course I have to train.
I know other languages, but this task is still more complex than I usually do.
Plus, I've refreshed my understanding of file systems, learned working with Github and writing READMEs.

> **7. All addresses are `uint32_t`, why?**

The task was to store "hundreds of thousand files". Assuming a file has 16 blocks in average (I've noted the number while surfing in random EXT4 overviews and found it quite handy) 3 bytes could be enough to address each. Initially, I was thinking of a custom 3-byte type, but after all it wasn't worth it. Classic 4-byte integer won (and I specified the size explicitly).

> **8. Name main issues within the VFS.**

Still not in better condition:
- Memory leaks (not quite following RAII) - still to be fixed.
- Handling exceptions - still not sure, what's the best way to use them, so currently I'm reworking the approach.
- Input validation - barely present; it's expected that paths and parameters are validated by the user. Thinking of changing that.

> **9. Any plans or ideas?**

1. Currently all the Files in the tree are loaded by default. That means, they waste time and memory even if they are not going to be used. I'm going to move the LoadFile() function to the SeekFile(), so that the missing data will be parsed from BinDisk only when it's needed.
2. I'm thinking of hiding block and node logic to separate classes nested in VDisk. Nodes will possible be moved to a static decorator derived from Vertice, and blocks will require some BlockManager (which can later be used for tracking free blocks).
3. Deleting files could be useful.
4. I'd like to rework Files. Currently, saving names is not necessary (names are tracked within the Vertice), and the metadata is used for dir/file flagging - that also can be tracked through Vertice (leaves are always files, other nodes are always dirs). 
