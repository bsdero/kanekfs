# Kanek File System
___

## A file system organized in a graph.

This project is about a file system organized in a cyclic/weighted graph, 
rather than a hierarchical tree.

The filesystems scheme based on directories and files, organized in a 
hierarchical tree has been around for quite some decades now. This design has 
been successful. The directories and files approach became the 'de facto' 
standard for all the existing storage solutions, at the point that is hard to 
imagine another way to organize a file system. Standards like posix included
the file systems organized in hierarchical tree more than three decades ago.

This system has some limitations, but we are so used to this way, that we 
don't mind.

For example, we tend to drop files in one directory per subject. After a while
we end with lot of files scattered for all over the directory and we dont 
know how are one related to the other. Professionals eventually ends like 
this:
- crime investigations
- medical registers
- papers, photos and documents for researchers
- legal contracts, documents

Even more, guides about how to handle lot of files does exist. 

I think the operating systems and storage technologies should continue 
evolving, explore new approaches and expanding the capabilities.

For this project I am trying to explore a Graph file system design, and this 
is the repo of this experimental approach. In this project we will address 
this, from a file system running in the user space.


___
### Hierarchical tree file systems 101

Before go deep in the concepts and design of a file system organized in a 
graph, we should review the hierarchical tree approach for organize files in a 
file system.

In this hierarchical tree, each "leaf" is what we call as "inode". Each inode 
may be an actual file, a directory, a link, a device file or another 
functional element (pipe, socket, soft link, etc). We have a unique, 
sequential number for identify each inode. 

Everything is organized inside a "block device", which can be a disk, cdrom, 
RAM, usb pen drive, a SSD, diskette, and other media. When the kernel 
performs IO into a block device, it writes whole block of bytes with data.
The blocks may have sizes like 512 bytes, 1k, 2k, 4k and in some cases, 8k.
Usually the block 0 of the device, holds the "superblock" which holds metadata
of the file system. 

In terms of file systems, an inode may be: 

- A standard file, an inode with pointers to some blocks with the file data.
- A directory, an inode with pointers to data blocks, but it holds 
"directory entries". Each directory entry has a name and an inode number, 
thus, it may point to another directory, a file, a link or other. We may 
imagine a directory as a "folder", which holds files inside.
- A link is a directory entry to an object, which already has another directory entry pointing to it.
- A device file, sockets, pipes, they are IO channels mapped to a file in the 
file system.

The way to identify an object in a hierarchical tree file system, is using a 
"path", like /bin/ls, /home/obiwan/document.txt, or /dev/sd0. We can easily 
locate an object, starting by the root directory "/" and the we traverse the 
directories, one by one, until we find our file.

The superblock, also holds a pointer to the inode 0, which is the 
"root directory" of the file system. It is identified by a slash "/". The 
root directory has entries pointing to other directories like "dev", "etc", 
"bin", "sbin" or "home".


Some sources:
- [https://en.m.wikipedia.org/wiki/File_system](https://en.m.wikipedia.org/wiki/File_system)
- [https://en.m.wikipedia.org/wiki/Hierarchical_file_system](https://en.m.wikipedia.org/wiki/Hierarchical_file_system)
- [https://en.m.wikipedia.org/wiki/Root_directory](https://en.m.wikipedia.org/wiki/Root_directory)
- [https://en.m.wikipedia.org/wiki/Device_file#BLOCKDEV](https://en.m.wikipedia.org/wiki/Device_file#BLOCKDEV)
- [https://en.m.wikipedia.org/wiki/Unix_file_types](https://en.m.wikipedia.org/wiki/Unix_file_types)

---
## Kanek File System design 
### Graph File system design
Good. So, how is a file system organized in a graph, rather than a 
hierarchical tree? The next section will explain that question.


#### Super Inode
Simple. All the nodes in the graph are files and may store data. But the nodes
can also have "edges", which points to other nodes in the graph.

So, a node not only behaves like the good old files, it also behaves like a
directory. Is a mix between a standard file and a directory. The data
structure corresponding to the inode would have byte blocks for both edges
(like directory entries) and data. No difference between nodes, except
functionality (data, socket, pipe, device, etc), as they would also have
edges.

No distintion between nodes in the file system graph.

We may call this inode variant as a "super node".


#### Edges of the nodes
In the classical file system approach, each directory have 
"directory entries". They have an entry name, and an inode number, among 
other data. Also entries for "." and "..", pointing to the current and 
parent directories are included.

Well, the edges works exactly like directory entries. But differences are:

1. for each edge in a node A pointing to a node B, we have another edge 
pointing from B to A. 
2. The ~directory~ edge entries have a shared unique ID (LinkID) for both 
nodes. We call a couple of edges as a "link". The LinkID is unique in the
context of connecting nodes, so we can have the same LinkID connecting two 
different nodes. 
3. the dot and dotdot ( "." and "..") entries are not used at this 
implementation level.
4. The edges have a flag "traversable". If the edge has this flag enabled, 
the edge may be traversed through, like in a directory. If not, that means 
the connecting nodes have just a relation.
5. The edges may have a flag "visible". If disabled, the link connecting 
nodes is visible only for administrators.

#### Extended attributes/metadata
We reserve extra disk space to store key-values for the graph. So, we can 
have an array of key-values, pretty much like the "hashes" in perl, 
"collections" in mongoDB or "dictionaries" in python. We will call a set of 
"key-values" a "dictionary" or "metadata".

We may assign dictionaries to inodes/files/super nodes. In this context, the 
metadata works pretty much like the "extended attributes" in traditional 
file systems.

Also the edges may have metadata. However the metadata is assigned to one 
link, mean, a couple of edges connecting two super nodes. So, the two 
connecting nodes -ongoing and returning- shares the same metadata and linkID.

The superblock may also have metadata, for specific information related to 
the file system. This may help to solve dependancies of the graph file system
like remote mounts, action to do, file system information, 
or scripts to run at mount/umount.

All of this information needs to be stored in disk, so we have a dedicated
disk space for store the metadata in "slots".

Slots are numbered logical spaces for store dictionaries, one per slot, and
they can be located by using a "slot index" or "slot table" in the very same 
way than the "inodes table" used very extensively by the traditional file 
systems.

In the slot index, the slot 0 is reserved for the superblock, and the others
may be used by links

#### Files and paths representation
A files graph may be represented by using the traditional hierarchical tree 
representation, like /boot/kernel-3.4.5/vmlinuz, /home/obiwan/taxes.doc, or 
/etc/system/named.conf. We may have also a root inode.

Any node in the graph file may be a root inode. But we are in a graph, that 
means we still have access to the whole file system. 

While the standard scheme is useful to access the files in the file graph, 
we still need to use a special notation for the edges.

We can use the next approach to specify a link:
```
/root/kernel-3.4.5->vmlinuz
/root/kernel-3.4.5:[LinkID]
```

#### Super block changes
For the superblock, most of the information from traditional file systems is 
preserved, except:
1. We need to store the extent address information for the Slots Table. 
2. The root inode for system operation can be any node. 

#### With this design we may:

1. we have a graph. So any node may be the root node, and we can still have 
access to the whole file system. No exceptions.
2. relations may be stablished between nodes, and the relations may have 
metadata.
3. also the super block may have its own dictionary/metadata.

All the features may be used for the next capabilities:
1. one single global filesystem with different root inodes, kernel and 
binaries, as desired. Useful for clusters, networking, containers, virtual 
machines.
2. software and data formats composed of many files, like 3d rendering 
software, package managers, AI models, video games.
3. Better organization of files for professional working with different 
files related by work units.
4. Can still be mounted into a standard file system. This solution may support
all the standard posix operations easily.
5. Vice-versa is also possible, although only the functionality provided by
the hierarchical file system is possible.


## GOFS Software Design

### 1       Graph Organized File System (GOFS) Design

Based on the general architecture, we will discuss about the software design 
for a Graph Organized File System. I will put an emphasis in functionality and
robustness. 

Is a bit complicated to cover every aspect of the graph file system in general
file system tools, like mkfs, fsck and so on. So, the file system software 
will be divided in modules, which would be in charge of specific 
functionalities and features of the file system.


So some of the planned modules:
1. **Foundation libraries (FL)**
   This libraries includes a cache framework, bitmap tools, hashing tools, 
   IO tools and lot of low level foundation functions. 
2. **Extents Caching Library (ECL).** 
   This will be the basis of the GOFS. We need to have a library and 
   also will be providing storage organization in extents and extents 
   caching. 
3. **Metadata Foundations (MetaF)**
   Basic software funcionality for CRUD operations with Slots of key-values.
   Includes tools for metadata storage creation, analysis, debugging, check.
4. **Metadata service (MetaServ).**
   This service will provide metadata storage caching.
5. **Kanek Object Storage Foundations (KOSF)**
   Basic Software functionality for CRUD operations for Object Storage and
   the corresponding objects metadata
6. **Kanek Object Storage Service (KOS)**
   This service will provide Object Storage IO caching. 
7. **Kanek Object Storage tools and utilities ( KOSTools)**
   Tools for Object Storage (creation, check, debugging)
8. **Graph Storage Foundations (KFS)**
   Elementary software functionality for Graph File System data structures
9. **Graph FS service (KFS service)**
   This service will handle the graph data structure for the file system. 
10. **Graph FS shell and commands (KFStools)**
   This will be binaries supporting the file system graph data structure. 


So, basically the software will be interacting like this with the OS/Kernel.


```
+----------------------------------------+
|                KFS Tools               |
|                    +-------------------+
|                    |  KFS service      |
+-----------+--------+-------------------+
| KOSTools  |    KFS                     |
+           +--------+                   |
|           |  KOS   |                   |
+-----------+--------+                   |
|        OSTF        |                   |
+      +-------------+------------+      |
|      |        MetaService       |      |
+      +--------------------------+      |
|      |           MetaF          |      |
+------+--------------------------+------+
|                  ECL                   |
+----------------------------------------+
|                   FL                   |
+----------------------------------------+
```


### 2       Foundation Libraries (FL)
A lot of different libraries needs to be designed. 
- Tracing and logging macros and libraries.
- Hashing
- Memory management/Garbage collection
- Bit map management for blocks and extents.
- Fast 64bit random generator
- IO for block devices/files by blocks and extents, also using bit maps
- Dictionary library
- Configuration files parser
- Hexadecimal dumps
- Stack dump display for debugging
- Cache framework


#### 2.1     Trace and Logging macros 
Trace facilities are just trace messages functionality, for debugging
and error analysis. Logging just registers in logs important, relevant 
notification messages. 
    
#### 2.2     Hashing for faster word searches and ordering. 
In order to make faster string searchs, a hash function which returns
an unsigned 64-bit integer has been implemented. The 64-bit value
matches string order, so the hash created by the word "cat" will be
always lesser than "dog", and both hashes will be lesser than "duck"
and "pig" respectively. 

#### 2.3     Garbage Collecting
This library allows to keep track of garbage collecting, to avoid
memory leaks. Possibly it can use the cache framework facilities.

#### 2.4     Bitmap management
Code exist already for mark bits and contiguous bit groups. This
functions are useful for bitmap management in block storage. 

#### 2.5     Fast random 64bit unsigned integers generator
Code for this does exist already
    
#### 2.6     EIO (IO library for read/write extents/blocks into block devices and files.)
An extent is a group of contiguous blocks. In order to get access to an 
extent we need starting block address and the number of blocks. 
The starting block address and the number of blocks, together are called an
"extent descriptor". 

"User Extents" are extents with user data. 
An "extent index" is a contiguous array of extents descriptors. 
A "blocks bitmap" is a bitmap which helps to know which blocks in the device
are in use or free. By default the block size is 8KBytes, although 16, 32 or
64 KBytes length per block is also selectable. 
An "Extents storage descriptor" describes:
- Block size 
- Extent index start block address and number of blocks
- Block bitmap start block address and number of blocks
- User extents start block address and number of blocks


The extents disk format help to store extents in a better organized way in 
block storage. It includes the items:
- Extents storage descriptor (optional, also can be stored elsewhere). 
  If stored, it should be stored in the block 0.
- Extents index (optional, can be stored elsewhere). If stored should 
  start at block 1
- Block bitmap (optional, can be stored elsewhere). If stored, should be
  in the last block of the device/file. 
- User extents block address

All of these, is called "extents storage". 

The bitmap is stored at the end of the device, in the last blocks. The next 
interfaces will be provided.

- Create extent storage on file or device.
- Open an existing extent storage file or device.
- Reserve extents 
- Read extents
- Write extents
- Free extents
- Close extents


Some of the interfaces:
- Open a file 
- Close a file 
- Read extents into memory
- write extents to disk
- Alloc extent into memory


Below we have a representation of extents storage.
```
        Extent Storage      
+-----------------------------+
| Extent Storage descriptor   |<--This descripts extents storage
+-----------------------------+
| Extents index               |<--This holds extents descriptors
+-----------------------------+
| User extents                |<--User stores data here
|                             |
|                             |
|                             |
|                             |
+-----------------------------+
| Block bitmap                |<--This helps to know which blocks are 
| 010000110011101010101000000 |   in use or free.
+-----------------------------+
```


#### 2.7     Dictionary library.
We need to add support to a new data structure based on key, values
with the same functionality than python dictionaries and perl hashes.
This will be named "dictionaries" and will handle strings as keys, 
and values in int, strings, floats, arrays and nested dictionaries.
Items will be a combo of key and it respective value.  

Functions supported will be 
- Creation of dictionaries
- Remotion of dictionaries and associated key-values
- Assignation of key and values
- Read key and values from dictionaries
- Read values from a dictionary, given a key.
- Remotion of items
- Display of items
- Packing and unpacking dictionaries, for data transfer and save/load
to and from storage.
Also macros will be provided for iterate through items and keys. 



#### 2.8     A configuration files parser.
The parser should support BASH style comments. Also variables, assignations of 
Python style arrays, strings, and operators like '=', '+' and '+=".

Support for reserved words like "include" should be added.
When the parser finds this word, it should proceed to parse the
specified file, and once it completes that file parsing, continue with
the current file parsing. 

Also support for "display" reserved word should be added, and it
should display strings, variables and all. 

The dictionaries facilities descripted in 2.7 should be used for the 
parser. It should create a whole new dictionary with the key-values 
parsed from the file. 

Example of configuration file:
```
# this is a conf file

########
##
# some comments


VARIABLE=123
MY_SRING="hello world "
MY_ARRAY=[ 0, 1, 2, 3, "four", "five" ]

MY_VAR0 = MY_ARRAY[0] # should assign 0
MY_VARS = MY_ARRAY[4] # should assign "four"

############ notice the next operators should be supported
MY_NEW_STRING = MY_STRING + MY_VARS # it should be a concatenation, the value
                                    # "hello world four" is expected

MY_NEW_STRING += " six" # it should be a concatenation, and expected
                        # value is "hello world four six"

MY_NEW_STRING_AGAIN=MY_STRING+" "+"mars"+" "+VARIABLE # should be 
                                                      # "hello world mars 123"

BANNER2="GOFS FILE SYSTEM"

VERSION="1"


# MOST EXAMPLE LINES BELOW
# default configurations for logging
DATE_FORMAT="+%m%d%Y_%H%M%S"

# directories of product
PRODUCT_DIR="/opt/gofs/"
LOG_DIR=PRODUCT_DIR + "log/"
CONF_DIR = PRODUCT_DIR + "etc/"
```


#### 2.9     Hexadecimal dump facilities
Functions and code already exist for hexadecimal dumps of memory. 

#### 2.10    Stackdump functionalities
Functions and code for get the stackdump for debugging purposes is on plan. 


#### 2.11    Cache Framework
A simple cache framework is needed to support all the other caches will be 
added.

The simple cache framework design will provide generic flags for
operation, status and a simple posix thread, which will process the 
cache elements, running operations according with cache elements
flags. The cache data structure consists on:
- One list of elements 
- One list of dirty elements. 
- Cache operation and status Flags
- A thread ID
- A thread mutex
- Callbacks for the on_map(), on_evict(), on_flush().

Each element in the cache will have the next fields:
- 64bit ID
- thread mutex
- flags
- access count
- a pointer to the parent cache data structure.

The framework make a distintion between the cache data structure and
the cache element data structure. Both are two separated entities. 
The cache structure interface:
- Alloc a cache
- Init cache ( populate cache with default values)
- Cache disable (flush and evict cache elements, stop thread)
- Cache enable  ( start thread)
- Cache sync ( evict all the elements, except pinned elements)
- Cache pause (pauses thread)
- Cache unpause
- Cache wait for flags ( wait for an specific flag)
- Cache lookup ( look for an element)

Cache elements:
- Element map ( map an element into a cache)
- Mark for eviction
- Mark dirty
- element pin
- element unpin
- element wait for flags
- evict 

So, most of those functions will be exported, some of them may 
be reimplemented by other caches built above this library. 


### 3       ECL Design
For the design of the Extents Caching Library, a Block size for the 
graph file system will be 8 Kbytes in size. So this is the standard 
block size. 


####        3.1 Extents Cache
Extents are groups of contiguous disk blocks. So, in order to make
faster IO operations, an extents cache will be used. An extent needs
a device block address and a number of contiguous block 
so this is the extent representation. This is mapped to memory. 

It works in a similar way to a page cache, but we are caching extents
in memory and not only pages. So ideally a thread would be in charge 
of this cache, flushing out extents marked as "dirty" and keep in 
memory any "pinned" extent. Also operations like "sync" should flush
out all the cached extents in memory.

This extents cache makes lot of use or EIO, the framework cache and 
the bitmap library.

So, it provides most of the functionalities of the EIO library, plus the
caching functionalities of the cache framework and updating the corresponding
bitmap as needed. 

Some of the interfaces:
- Make an extents block 
- Open/Read an specific extent (read an extent into memory)
- Flush extent ( write into storage, mark the bitmap as occupied)
- Reserve extent ( mark the bitmap as occupied, 


### 4       Metadata foundations (MetaF)
The metadata are key-value pairs. So the metadata foundations provides 
metadata storage functionalities. The metadata is organized in slots, and
each slot is identified by an unsigned number, being the 0 reserved and 
exclusive for upper layers of the system.

The slots are indexed into a big contiguous table, named the "slot index" 
which is an extent with a description of the slot, like:
- slot ID ( a unique number)
- slot inode owner
- slot edge owner 
- slot flags  ( which kind of owner, super block, inode of edge)
- extent address with the slot data ( the dictionary corresponding to this 
slot)  

The slot index uses a bit map for register which slots are in use, in a 
similar way to an inodes index bitmap, very common in traditional file systems.
The slot index helps to locate the effective block address where the 
corresponding dictionary is stored. The dictionary on disk is also called 
"slot data". 

This on-disk data structure, including the slot index, and slot data, is 
called as "slots storage". The data structure for locate the slot index and 
slot data on disk is called as "metadata storage descriptor". Is expected
that the metadata storage descriptor to be stored in disk.
The user can select how much portion of the file or block device will be
used for slots storage, or maybe, all of it, considering the layer below, 
which is ECL. This data is stored in the metadata storage descriptor.
The metada storage descriptor includes:
- Starting block of the slots index
- Starting block of the slots data ( dictionaries of all the slots )
- size in blocks of the slot index and the blocks data
- how many slots are stored
- the corresponding bitmap address for the used and free slots. 
 
Below we have a representation of slots storage.
```
+-----------------------------+
| Metadata Storage Descriptor |<-- this describes the slots storage
+-----------------------------+ 
| Slots Index                 |<-- this helps to locate key-values based from
+-----------------------------+    a slot number.
| Slots data ( dictonaries)   |        
|                             |<-- this are key-values.
|                             |
|                             |
+-----------------------------+
| Slots bitmap                |<-- this helps to know which slots are in use
+-----------------------------+    or free
```

Functionality for create, read and update slots storage is also provided. 
    
The metadata foundations makes use of the Extents cache, the Dictionary
library descripted in 2.7, and the bit map library. Also uses the EIO capacity
for make extent disk format. 

some of the interfaces:
- Create slots storage ( user must specify which file or device to use, 
  and specify the metadata storage descriptor )
- Open a slot storage file or device, user must specify where in the
  device is the metadata storage descriptor. 
- Create a slot
- Open a slot, given the slot number
- Read a slot descriptor
- Read slot data
- Update slot descriptor
- Update slot data
- Close a slot
- Reserve a slot from non used slots ( this operation also open the slot)
- Evict a slot ( remove slot data and descriptor, and mark the slot as free)

Includes some tools for:
- create slots storage
- dump slots data and dictionaries
- dump metadata storage descriptor and extents storage descriptors.
- Add data to an specific slot

### 5       Metadata service
Provides caching functionality to MetaF. 
Also provides the next interfaces:
- Open slot storage 
- Get a new, empty slot ( Reserve a slot)
- Open a slot
- Read slot
- Flush slot data
- empty slot data
- update slot descriptor
- update slot data

it uses the Metadata Foundations library, a lot. 


### 6       Kanek Object Storage Foundations (KOSF)


#### 6.1        Super Inode
The inode is a data structure which is very well known in traditional file 
systems. In Kanek Object Storage and Kanek Graph File system, inodes will be
used. Inodes are very well suited for Object Storage, however we would be 
adding an extra pointer, as the inodes not only stores data of a file, but
also edges for this graph node. 

That means we will be using extra pointer(s). This expanded inode is called 
as Super Inode. 

will have the next fields by default, all of them stored in the inode table:
- User Id of owner
- Group Id of owner
- permissions 
- size of file in bytes
- timestamps ( creation time, last modification time, last access time)
- slot ID 
- Flags
- data extent pointer
- edges extent pointer ( notice we have both, not just one)
- Inode link count (not useful for OFS but it is for Graph Storage)

#### 6.2        Object Storage Buckets
The object storage foundations is basically minimal functionality for 
Object Storage. The object storage allows "buckets" which are used for 
create object storage with in. The buckets works pretty much like directories
in standard file system, except that bucket can not store nested data, means,
a bucket can not be stored in another bucket. 


The metadata of the bucket: 
- Name
- User ID of owner
- Group ID of owner
- Permissions
- Time Stamp
- timestamps ( creation time, last modification time, last access time)
- slot ID 
- Flags

A super inode is used to store a bucket. Additional metadata of a bucket
can be stored in slots. Same for Object Storage Files.

The file entries of a bucket containes object files names, and super inodes
numbers. This darta can be stored in an extent or blocks, pointed
by the edges extent pointer, in a very similar fashion to directories in 
traditional file systems. 

#### 6.3        Object Storage API
We should be able to:
- Create buckets
- Create File Objects in a bucket
- Read File Objects 
- Update File Objects 
- Delete File Objects
- List objects in a bucket
- Remove bucket ( this operation removes all the objects in the bucket, 
                  and the associated metadata)



#### 6.4        Object Storage Block Device Organization
Object storage, in our context has areas on block storage which are very 
similar to the standard areas in traditional computer file systems. 

1. Root inode. the root super inode works pretty much like the root
inode in traditional file systems. It basically content entries to all the
buckets available in the Object Storage. 
2. super Inodes table ( maintains super inodes data structures table)
3. Super Inodes bitmap  ( a bitmap to keep used and free super inodes)
4. File objects blocks ( one or more extents per object can be assigned)
5. Free blocks bitmap ( underlying ECL free blocks bitmap can be used)
6. Slots Storage ( Metadata service will be used)
7. Object Storage SuperBlock. Pretty much like the traditional file systems.
Contents block pointert to all of this data, usually in the first block of 
the device. 

The data with the on-disk location for this information will be called as an
"Object Storage superblock". This layer makes usage of the Metadata Services
and the Extents cache. 

Below we have a representation of Object Storage.
```

+------------------------------+
|  Object Storage SuperBlock   |
+------------------------------+
| Slots Storage                |
+------------------------------+
| Inodes table                 |
+------------------------------+
| Inodes bitmap                |
+------------------------------+
| Object Files                 |
|                              |
+------------------------------+
| Blocks bitmap                |
+------------------------------+
```

#### 6.3    Root Inode

#### 6.2    Inode Table 
Maintains super inodes data structure.
#### 6.3    Inodes bit map. 
Is a bit map which helps to know which inodes are in use and which are free.

#### 6.4    File objects blocks
Is a storage area used for object storage files data. Also graph data can be 
stored. 

#### 6.5    Free blocks bitmap
Keeps track of the used and free blocks. Underlying ECL can be used here. 

#### 6.6    Slots storage 
Extra metadata which the user creates for object files will be stored in 
slots storage. 

### 7       Kanek Object Storage Service (KOS) 
Provides a service for Kanek Object Storage. 
It includes a daemon, and exportable directory less file system. 



### 8       Kanek Object Storage Tools (KOSTools)
Some tools for object storage will be created here, including:
- mount kanek object storage given a bucket
- umount kanek object storage
- S3 API libraries and commands 


### 8       Graph Storage Foundations (KFS)
Elementary software functionality for Graph File System data structures.
At this point, the Graph structure of the GOFS 


### 9       Graph FS service (KFS service)
This service will handle the graph data structure for the file system. 

### 10      Graph FS shell and commands (KFStools)
shell, and some programs supporting the file system graph data structure. 






