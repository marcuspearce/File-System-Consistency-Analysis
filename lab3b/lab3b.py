#!/usr/bin/env python

# NAME: Marcus Pearce, Ryan Lam
# EMAIL: marcuspearce7@g.ucla.edu, ryan.lam53@gmail.com
# ID: 305115848, 705124474
# CS 111 Project 3a


import sys
import csv


# DATA STRUCTURES

blockToInodeDict = {}                  # map block number -> list  [inode that reference it, indirection level, offset]

inodes = []
blocks = []
freeInodeList = set()           # all free inodes
freeBlockList = set()           # all free blocks
allocatedInodeList = set()      # all allocated inodes
dirents = []
inodeLinkCount = {}             # dictionary inode number -> link count

inodeToParentDict = {}            # maps inodeNum -> parent inodeNum

sb = None                       # superblock 




class Superblock:
    def __init__(self, data):
        self.numBlocks = int(data[1])
        self.numInodes = int(data[2])
        self.inodeTableSize = int(data[2])*int(data[4]) / int(data[3])

class Inode:
    def __init__(self, data, free):
        self.num = int(data[1])
        self.free = free
        self.fileType = data[2]
        self.blockPointers = {} # map block number to logical offset
        self.linksCount = data[6]

class Block:
    def __init__(self, num):
        self.num = num

class Group:
    def __init__(self, data, inodeTableSize):
        self.dataBlockStart = int(data[8])+inodeTableSize

class Dirent:
    def __init__(self, data):
        self.parentInodeNum = int(data[1])
        self.inode = int(data[3])
        self.name = data[6]

# inode name inode_num





def main():
    # check for only one argument 
    if(len(sys.argv) != 2):
        print("Incorrect arguments")
        exit(2)

    with open(sys.argv[1], "r") as f:
        lineReader = csv.reader(f)
        for row in lineReader:
            if row[0] == 'SUPERBLOCK': # get superblock data
                sb = Superblock(row)

            elif row[0] == 'GROUP': # get group data
                gt = Group(row, sb.inodeTableSize)
                for i in range(gt.dataBlockStart, sb.numBlocks): # init block dictionary
                    blockToInodeDict.update({i: []})

            elif row[0] == 'BFREE': # record free blocks
                blocknum = int(row[1])
                freeBlockList.add(blocknum)

            elif row[0] == 'IFREE': # record free inodes
                inodenum = int(row[1])
                freeInodeList.add(inodenum)

            elif row[0] == 'INODE': # record allocated inodes
                ti = Inode(row, 0)
                allocatedInodeList.add(ti.num)

                # block consistency audit -- check invalid and reserved blocks 
                if(ti.fileType == 'f' or ti.fileType == 'd' or (ti.fileType == 's' and int(row[10]) > 60)):
                    for i in range(12, 27): # loop thru 15 block refs per inode
                        curblockref = int(row[i])
                        indirectlvl = 0
                        if(not curblockref == 0):
                            if(i == 24):
                                indirectlvl = 1
                                offset = 12
                                indirectstring = "INDIRECT "
                            elif(i == 25):
                                indirectlvl = 2
                                offset = 268
                                indirectstring = "DOUBLE INDIRECT "
                            elif(i == 26):
                                indirectlvl = 3
                                offset = 268 + (256*256)
                                indirectstring = "TRIPLE INDIRECT "
                            else:
                                offset = i - 12
                                indirectstring = ""

                            if(curblockref < 0 or curblockref > 63): # check invalid/reserved
                                print("INVALID " + indirectstring + "BLOCK " + str(curblockref) + " IN INODE " + row[1] 
                                + " AT OFFSET " + str(offset))
                            elif(curblockref < gt.dataBlockStart):
                                print("RESERVED " + indirectstring + "BLOCK " + str(curblockref) + " IN INODE " + row[1]
                                + " AT OFFSET " + str(offset))
                            else:
                                ti.blockPointers.update({curblockref: offset}) # update inode object's block refs
                                blockToInodeDict[curblockref].append([int(row[1]), indirectlvl, offset])  # add inode reference to block dict     

                inodes.append(ti) # add inode object to inodes list

            elif row[0] == 'INDIRECT': # record indirect block refs
                curblockref = int(row[5])
                indirectlvl = int(row[2])
                offset = 0
                if(indirectlvl == 1):
                    indirectstring = "INDIRECT"
                    offset = 12
                if(indirectlvl == 2):
                    indirectstring = "DOUBLE INDIRECT"
                    offset = 268
                if(indirectlvl == 3):
                    indirectstring = "TRIPLE INDIRECT"
                    offset = 268 + 256*256
                
                if(curblockref < 0 or curblockref > 63): # check invalid/reserved
                    print("INVALID " + indirectstring + " BLOCK " + str(curblockref) + " IN INODE " + row[1] 
                    + " AT OFFSET " + row[3])
                elif(curblockref < gt.dataBlockStart):
                    print("RESERVED " + indirectstring + " BLOCK " + str(curblockref) + " IN INODE " + row[1]
                    + " AT OFFSET " + row[3])
                else:
                    blockToInodeDict[curblockref].append([int(row[1]), indirectlvl, offset])  # add inode reference to block dict



                  

            elif row[0] == 'DIRENT':
                dirents.append(Dirent(row))

    f.close()





    # BLOCK consistency audit
            
    for x in blockToInodeDict:
        if (not len(blockToInodeDict[x]) == 0) and (x in freeBlockList):
            print("ALLOCATED BLOCK " + str(x) + " ON FREELIST")
        elif (len(blockToInodeDict[x]) == 0) and (not x in freeBlockList):
            print("UNREFERENCED BLOCK " + str(x))

    # process duplicate blocks
    for blockNum in blockToInodeDict:  
        if len(blockToInodeDict[blockNum]) > 1:         # if more than one thing in list -> duplicate
            for info in blockToInodeDict[blockNum]:
                inodeNum = info[0]
                indirectlvl = info[1]
                offset = info[2]
                indirectstring = ""

                if indirectlvl == 1:
                    indirectstring = "INDIRECT "
                elif indirectlvl == 2:
                    indirectstring = "DOUBLE INDIRECT "
                elif indirectlvl == 3:
                    indirectstring = "TRIPLE INDIRECT "
                print("DUPLICATE " + indirectstring + "BLOCK " + str(blockNum) + " IN INODE " + str(inodeNum) + " AT OFFSET " + str(offset))









    
    # INODE allocation audit

    if(2 in freeInodeList) and (2 in allocatedInodeList):
            print("ALLOCATED INODE " + str(2) + " ON FREELIST")
    elif(not 2 in freeInodeList) and (not 2 in allocatedInodeList):
        print("UNALLOCATED INODE " + str(2) + " NOT ON FREELIST")

    for i in range(11, sb.numInodes+1):
        if(i in freeInodeList) and (i in allocatedInodeList):
            print("ALLOCATED INODE " + str(i) + " ON FREELIST")
        elif(not i in freeInodeList) and (not i in allocatedInodeList):
            print("UNALLOCATED INODE " + str(i) + " NOT ON FREELIST")


    


    # DIRECTORY consistency audits 

    # zero out dictionaries to init
    for i in range(sb.numInodes):
        inodeLinkCount[i+1] = 0
    for i in range(len(dirents)):
        inodeToParentDict[i] = 0

    # scan all of directories to ennumerate all links, finding invalid and unallocated inodes 
    for dirent in dirents:
        if dirent.inode > sb.numInodes or dirent.inode < 1:
            print("DIRECTORY INODE " + str(dirent.parentInodeNum) + " NAME " + dirent.name + " INVALID INODE " + str(dirent.inode))
        elif dirent.inode not in allocatedInodeList:
            print("DIRECTORY INODE " + str(dirent.parentInodeNum) + " NAME " + dirent.name + " UNALLOCATED INODE " + str(dirent.inode))
        else:
            inodeLinkCount[dirent.inode] += 1

        # for ".." case, find parent of each inode
        if dirent.name != "'.'" and dirent.name != "'..'":
            inodeToParentDict[dirent.inode] = dirent.parentInodeNum

    inodeToParentDict[2] = 2        # special case -> root's parent is itself (inode 2)


    # check reference count with num discovered links
    for inode in inodes:
        if int(inodeLinkCount[inode.num]) != int(inode.linksCount):
            print("INODE " + str(inode.num) + " HAS " + str(inodeLinkCount[inode.num]) + " LINKS BUT LINKCOUNT IS " + str(inode.linksCount))

    # check . and ..
    for dirent in dirents:
        if dirent.name == "'..'" and int(inodeToParentDict[dirent.parentInodeNum]) != int(dirent.inode):
            print("DIRECTORY INODE " + str(dirent.parentInodeNum) + " NAME '..' LINK TO INODE " + str(dirent.inode) + " SHOULD BE " + str(dirent.parentInodeNum))
        if dirent.name == "'.'" and int(dirent.parentInodeNum) != int(dirent.inode):
            print("DIRECTORY INODE " + str(dirent.parentInodeNum) + " NAME '.' LINK TO INODE " + str(dirent.inode) + " SHOULD BE " + str(dirent.inode))

    # for i in inodeToParentDict:
    #     print(inodeToParentDict[i])

    

     




if __name__ == "__main__":
    main()