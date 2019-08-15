import csv

allocatedInodes = dict()
allocatedBlocks = dict()
unallocatedInodes = dict()

indirectMap = dict()
directoryMap = dict()

inodeBitmaps = []
blockBitmaps = []
freeInodeList = []
freeBlockList = []

invalidBlockPointers = []
incorrectDirectoryEntries = []

outputFile = open('lab3b_check.txt', 'w')

class Inode(object):
	def __init__(self, inodeNumber, nLinks):
		self.inodeNumber = inodeNumber
		self.referenceList = []
		self.nLinks = nLinks
		self.ptrs = []
		return

class Block(object):
	def __init__(self, blockNumber):
		self.blockNumber = blockNumber
		self.referenceList = []
		return

def readSuperblock():
	superblock = csv.reader(open("super.csv", 'r'), delimiter=',', quotechar='|')
	for row in superblock:
		global inodeCount
		global blockCount
		global blockSize
		global blocksPerGroup
		global inodesPerGroup
		inodeCount = int(row[1]);
		blockCount = int(row[2]);
		blockSize = int(row[3]);
		blocksPerGroup = int(row[5]);
		inodesPerGroup = int(row[6]);
	return

def readGroup():
	group = csv.reader(open("group.csv", 'r'), delimiter=',', quotechar='|')
	for row in group:
		global inodeBitmaps
		global blockBitmaps
		global allocatedBlocks
		inodeBitmaps.append(int(row[4], 16))
		blockBitmaps.append(int(row[5], 16))
		allocatedBlocks[int(row[4], 16)] = Block(int(row[4], 16))
		allocatedBlocks[int(row[5], 16)] = Block(int(row[5], 16))
	return

def readBitmap():
	bitmap = csv.reader(open("bitmap.csv", 'r'), delimiter=',', quotechar='|')
	for row in bitmap:
		global freeInodeList
		global freeBlockList
		number = int(row[0], 16)
		if number in inodeBitmaps:
			freeInodeList.append(int(row[1]))
		elif number in blockBitmaps:
			freeBlockList.append(int(row[1]))
		else:
			print "the number isn't in either bitmap file, error"
	return

def updateBlock(blockNumber, inodeNumber, indirectBlockNumber, entryNumber):
	global invalidBlockPointers
	global blockCount
	global allocatedBlocks
	if blockNumber == 0 or blockNumber >= blockCount:
		invalidBlockPointers.append((blockNumber, inodeNumber, indirectBlockNumber, entryNumber))
	elif blockNumber in allocatedBlocks:
		allocatedBlocks[blockNumber].referenceList.append((inodeNumber, indirectBlockNumber, entryNumber))
	else:
		allocatedBlocks[blockNumber] = Block(blockNumber)
		allocatedBlocks[blockNumber].referenceList.append((inodeNumber, indirectBlockNumber, entryNumber))
	return

def readIndirect():
	global indirectMap
	indirect = csv.reader(open("indirect.csv", 'r'), delimiter=',', quotechar='|')
	for row in indirect:
		block = int(row[0], 16)
		if block in indirectMap:
			indirectMap[block].append((int(row[1]), int(row[2], 16)))
		else:
			indirectMap[block] = [(int(row[1]), int(row[2], 16))]
	return

def singleIndirect(blockNumber, inodeNumber, indirectBlockNumber, entryNumber):
	global invalidBlockPointers
	counter = 1
	updateBlock(blockNumber, inodeNumber, indirectBlockNumber, entryNumber)
	if blockNumber in indirectMap:
        	for x in indirectMap[blockNumber]:
        		updateBlock(x[1], inodeNumber, blockNumber, x[0])
                        counter +=1
	else:
		invalidBlockPointers.append((blockNumber, inodeNumber, indirectBlockNumber, entryNumber))
	return counter

def doubleIndirect(blockNumber, inodeNumber, indirectBlockNumber, entryNumber):
	counter = 1
	updateBlock(blockNumber, inodeNumber, indirectBlockNumber, entryNumber)
	if blockNumber in indirectMap:
		for x in indirectMap[blockNumber]:
			counter += singleIndirect(x[1], inodeNumber, blockNumber, x[0])
	else:
		invalidBlockPointers.append((blockNumber, inodeNumber, indirectBlockNumber, entryNumber))
	return counter

def tripleIndirect(blockNumber, inodeNumber, indirectBlockNumber, entryNumber):
	counter = 1
	updateBlock(blockNumber, inodeNumber, indirectBlockNumber, entryNumber)
	if blockNumber in indirectMap:
		for x in indirectMap[blockNumber]:
			counter += doubleIndirect(x[1],inodeNumber, blockNumber, x[0])
	else:
		invalidBlockPointers.append((blockNumber, inodeNumber, indirectBlockNumber, entryNumber))
	return counter

def readInode():
	global blockCount
	global allocatedInodes
	global invalidBlockPointers
	inode = csv.reader(open("inode.csv", 'r'), delimiter=',', quotechar='|')
	for row in inode:
		allocatedInodes[int(row[0])] = Inode(int(row[0]), int(row[5]))
		if int(row[10]) <= 12:
			for i in range(11, int(row[10]) + 11):
				updateBlock(int(row[i],16), int(row[0]), 0, i - 11)
		else:
			nBlocks = int(row[10]) - 12
			indirectBlocks = int(row[23], 16)
			if indirectBlocks == 0 or indirectBlocks >= blockCount:
				invalidBlockPointers.append((indirectBlocks, int(row[0]), 0, 12))
			else:
				counter = singleIndirect(indirectBlocks, int(row[0]), 0, 12)
				nBlocks -= counter
			
			if nBlocks > 0:
				indirectBlocks = int(row[24], 16)
				if indirectBlocks == 0 or indirectBlocks >= blockCount:
					invalidBlockPointers.append((indirectBlocks, int(row[0]), 0, 13))
				else:
					counter = doubleIndirect(indirectBlocks, int(row[0]), 0, 13)
					nBlocks -= counter
			
			if nBlocks > 0:
				indirectBlocks = int(row[25], 16)
				if indirectBlocks == 0 or indirectBlocks >= blockCount:
					invalidBlockPointers.append((indirectBlocks, int(row[0]), 0, 14))
				else:
					counter = singleIndirect(indirectBlocks, int(row[0]), 0, 14)
					nBlocks -= counter
	return

def readDirectory():
	global directoryMap
	global allocatedInodes
	global unallocatedInodes
	directory = csv.reader(open("directory.csv", 'r'), delimiter=',', quotechar='\"')
	for row in directory:
		pModeNumber = int(row[0])
		cModeNumber = int(row[4])
		entryNumber = int(row[1])
		if pModeNumber != cModeNumber or pModeNumber == 2:
			directoryMap[cModeNumber] = pModeNumber
		if cModeNumber in allocatedInodes:
			allocatedInodes[cModeNumber].referenceList.append((pModeNumber, entryNumber))
		else:
			if cModeNumber in unallocatedInodes:
				unallocatedInodes[cModeNumber].append((pModeNumber, entryNumber))
			else:
				unallocatedInodes[cModeNumber] = [(pModeNumber, entryNumber)]
		if entryNumber == 0:
			if cModeNumber != pModeNumber:
				incorrectDirectoryEntries.append((pModeNumber, row[5], cModeNumber, pModeNumber))
		elif entryNumber == 1:
			if pModeNumber not in directoryMap or cModeNumber != directoryMap[pModeNumber]:
				incorrectDirectoryEntries.append((pModeNumber, row[5], cModeNumber, directoryMap[pModeNumber]))
	return

def writeUnallocatedBlocks():
	global allocatedBlocks
	global freeBlockList
	for item in sorted(allocatedBlocks):
		if item in freeBlockList:
			string = "UNALLOCATED BLOCK < " + str(item) + " > REFERENCED BY "
			for ref in sorted(allocatedBlocks[item].referenceList):
				if int(ref[1]) == 0:
					string += "INODE < " + str(ref[0]) + " > ENTRY < " + str(ref[2]) + " > "
				else:
					string += "INODE < " + str(ref[0]) + " > INDIRECT BLOCK < " + str(ref[1]) + " > ENTRY < " + str(ref[2]) + " > "
			outputFile.write(string.strip() + "\n")
	return

def writeDuplicateBlocks():
	global allocatedBlocks
	for x in sorted(allocatedBlocks):
		if len(allocatedBlocks[x].referenceList) > 1:
			string = "MULTIPLY REFERENCED BLOCK < " + str(x) + " > BY "
			for ref in sorted(allocatedBlocks[x].referenceList):
				if int(ref[1]) == 0:
					string += "INODE < " + str(ref[0]) + " > ENTRY < " + str(ref[2]) + " > "
				else:
					string += "INODE < " + str(ref[0]) + " > INDIRECT BLOCK < " + str(ref[1]) + " > ENTRY < " + str(ref[2]) + " > "
			outputFile.write(string.strip() + "\n")
	return

def writeMissingUnallocatedInodes():
	global allocatedInodes
	global inodesPerGroup
	global freeInodeList
	global freeBlockList
	global inodeBitmaps
	global blockBitmaps
	global inodeCount
	for inodeNumber in sorted(allocatedInodes):
		if inodeNumber > 10 and len(allocatedInodes[inodeNumber].referenceList) == 0:
			outputFile.write("MISSING INODE < " + str(inodeNumber) + " > SHOULD BE IN FREE LIST < " + str(inodeBitmaps[int(inodeNumber)/inodesPerGroup]) + " >\n")
		elif allocatedInodes[inodeNumber].nLinks != len(allocatedInodes[inodeNumber].referenceList):
			outputFile.write("LINKCOUNT < " + str(inodeNumber) + " > IS < " + str(allocatedInodes[inodeNumber].nLinks) + " > SHOULD BE < " + str(len(allocatedInodes[inodeNumber].referenceList)) + " >\n")
		if inodeNumber in freeInodeList:
			outputFile.write("UNALLOCATED INODE < " + str(inodeNumber) + " >\n")
	
	for x in range(11, inodeCount):
		if (x not in freeInodeList) and (x not in allocatedInodes):
			outputFile.write("MISSING INODE < " + str(x) + " > SHOULD BE IN FREE LIST < " + str(blockBitmaps[int(x)/inodesPerGroup]) + " >\n")
	return

def writeIncorrectDirectoryEntry():
	global incorrectDirectoryEntries
	for x in sorted(incorrectDirectoryEntries):
		outputFile.write("INCORRECT ENTRY IN < " + str(x[0]) + " > NAME < " + str(x[1]) + " > LINK TO < " + str(x[2]) + " > SHOULD BE < " + str(x[3]) + " >\n")
	return

def writeInvalidBlockPointer():
	global invalidBlockPointers
	for x in sorted(invalidBlockPointers):
		string = "INVALID BLOCK < " + str(x[0]) + " > IN INODE < " + str(x[1]) + " > "
		if int(x[2]) > 0:
			string += "INDIRECT BLOCK < " + str(x[2]) + " > "
		outputFile.write(string.strip() + " ENTRY < " + str(x[3]) + " >\n")
	return
	

if __name__ == "__main__":
	readSuperblock()
	readGroup()
	readBitmap()
	readIndirect()
	readInode()
	readDirectory()

	writeInvalidBlockPointer()
	writeIncorrectDirectoryEntry()
	writeMissingUnallocatedInodes()
	writeDuplicateBlocks()
	writeUnallocatedBlocks()
