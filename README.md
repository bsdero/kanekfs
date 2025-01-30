# Kanek File System
___

### A file system organized in a graph.

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
is the repo of this experimental approach. In this project we will address this, from a file system running in the user space.


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

1. we have a graph. So any node may be the root node, and we can still have access to the whole file system. No exceptions.
2. relations may be stablished between nodes, and the relations may have 
metadata.
3. also the super block may have its own dictionary/metadata.

All the features may be used for the next capabilities:
1. one single global filesystem with different root inodes, kernel and 
binaries, as desired. Useful for clusters, networking, containers, vms.
2. software and data formats composed of many files, like 3d rendering 
software, package managers, AI models, video games.
3. Better organization of files for professional working with different 
files related by work units.
4. Can still be mounted into a standard file system. This solution may support
all the standard posix operations easily.
5. Vice-versa is also possible, although only the functionality provided by
the hierarchical file system is possible.


#### Software Design
Is a bit complicated to cover every aspect of the graph file system in general file system tools, 
like mkfs, fsck and so on. So, the file system software will be divided in modules, which would be in charge of
specific functionalities and features of the file system.

For practicity, an object storage service is on plan, and there will be a metadata service for handle the metadata required by the super block, inodes or edges.

So some of the planned modules and tools:

-kfs_slotd: a metadata service with direct access to block storage
-kfs_mkslots: a tool for build metadata slots in block storage
-kfs_obstrd: an object storage service for store and retrieve files in block storage
-kfs_mkostor: a tool for build object storage in block storage
-kfs_graphd: a graph daemon, with the graph file system structure and also is the interface with user apps and graph command line interface
-kfs_mkgraph: a tool for build the graph file system
-kaneksh: a CLI shell which interfaces with both posix and graph file system
-kfs_mkfs a tool for build slots, storage and graph in a single block storage device

Also diagnostics tools and scripts are in the roadmap.






