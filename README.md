# tryC

```cmake -S . -B build```

# RAID

Say u havea set of disks, each having a set of sectors (sectors being the smallest piece the disk itself can write or read via an operation)

D1 D2 D3 D4 ... Dn
s1 s1 s1 s1 ... s1
s2 s2 ...       s2
.
.
.
sZ ...          sZ

As u can see we are talking about a set of disks of the same size. 
From now on we will abstract the biggests sector size to of all the disk to be called the block size of the logical block for our RAID system.
Also in this view we can look at a each row being a block on each disk stated on the same offset. we will look at the per row concept agin in a bit.
Conceptually if we flatten the dual axis view into a singular line by combining one row after the other, we will have a bigger "singular disk".

D1s1 D2s1 D3s1 ... Dns1 D1s2 D2s2 D3s2 ... Dns2 ... D1sZ D2sZ ... DnsZ

Now if this was a singular disk then keeping the LB size as it is would be okay, 
But in reallity this are a bunch of different disks and making the CPU switch between them on every sector write and read will be making to much use of the CPU.
Of course this depednts if there a re more disks then cpu threads (becuase raid already wants to parallelize reads and writes).
So we now have a chunk/strip instead which is a group of LBs.

Chunk Size = 4:
    * LBA 0,1,2,3 sit on the same chunk/strip on disk 1
    * LBA 4,5,6,7 sit on the same chunk/strip on disk 2

Now each row of strips/chunks is called a Stripe.

Why keep them all at the same offset and write one disk after the other?
I am guessing it would be better for continous writes becuase the loaded disks are already pointed at the prior write offset and the jump to next write possition is one step.
You could say that by flattening the write order by column (by disk) basically this would have the same effect, and i dont think that would be wrong, but
the filesystem that is running on this is a regular filesystem that cna have reads as well on multiple processes and thus it doesnt distribute the load of ops between the avilable disks...
Additionally its about write and read parallelism -> self explanatory

RAID is also about disaster recovery and redundancy.
Each RAID level is a standard of its own on how to do that, and it can include parity LBs on a specific disk or a set of disks or interchanging disks etc...
Still usefull to have the parity LBs being in the same offset across disks for finding the parity objects or in reconstruction of failed disks and its ordering.

# Bootstrap

First we initialize a disk_t for every disk availble and then filter them and group them via their superblock.

The superblock is the first LB of each disk and includes its metadata regarding disk id in a raid cluster along with the raid id and the raid level -> self explanatory.

# Sectors vs Machine

Machine is working on 32 and 64 bits read/writes. 
Then why have sectors of 512 bytes or 4096 bytes (4KB)?
A sector needs extra space for for physcal disk logic and error correction code parity managed by the disk.

Doing that every 32 or 64 bits is a lot of wasted space.
U can do that every 4KB and still be backward compatible with 512 emulation for older machines.

RAID is the layer on top of the physical disks and that includes its sectoring.

# What about Concurency?

The disks are loaded and they get a fd for the live and direct access which includes the ability to continue from where the disk is alredy pointing to.
This is what makes the continous writes also effective along with the paralessim across the disks...
We use pread and pwrite becuase it works via the fd and writes in one system operation instead of open and seek and read and write...

Making the writes and reads atomic meaning we only need a queue for the IO desired and that is provided to us by default via the kernel across all the sessions.
We can make it better by overriding that queue with one that takes into account the current position on each disk and the known next queued ops.

# Parity vs Mirroring

mirroring is straight forward to undesrstand that another disk has the same data.
parity is math calculations for raid 4,5,6 and more...

parity is not mirroring.
parity disks are set on a specific disk or interchnage per stripe

raid 4 -> has a set parity disk
raid 5 -> has left symteric rotating parity disks per according to stripe idx

# RAID LIB Usage

the ouput of this project is a raid lib. it is used by stuff like a raid OS having a filesystem ontop of it.
When initializing the filesystem the raid os asks the raid lib what is its capacity and it answers the toatl amount of data capacity not including the redundancy disks...
in raid 5 with 4 100gb disks it will answer that it has 300gb.

that is why there is the general LB but it also is split into two, the phys_lb and the user_lb or better named the redundancy_lb and the data_lb
