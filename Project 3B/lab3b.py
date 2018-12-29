#!/usr/bin/python

# NAME: Angela Wu,Candice Zhang
# EMAIL: angelawu.123456789@gmail.com,candicezhang1997@gmail.com
# ID: 604763501,604757623

import sys
import os.path
import csv

EXIT_SUCCESS=0
EXIT_FAIL=1	
EXIT_INCONSISTENSIES=2

file=None
superblock_=None
group_=None
free_blocks_=[]
inodes_=[]
indirects_=[]
blocks_count=0
block_num_first_inode=None
inodes_count_in_group=None
inode_size=None
reserved_blocks=None
block_size=None
error=0
allocated_blocks_count=[]
allocated_blocks={}

inodes_count=0
ifree_list=[]    #list of free inode numbers. get from IFREE
dirent_list=[]    #list of lines belonging to DIRENT summary
ialloc_dict={}   #dictionary of (inum,link count) pairs. get from INODE
dir_dict={}      #dictionary of (inum,actual link count) pairs. get from DIRENT
parent_dict={}    #dictionary of (inum,parent inum) pairs. get from DIRENT

def usage_error():
	sys.stderr.write("Correct Usage: ./lab3b filename\n")
	exit(EXIT_FAIL)

def csv_reader():
	global superblock_, group_, free_blocks_, \
		inodes_, indirects_, inodes_count, blocks_count,\
		block_num_first_inode, inodes_count_in_group, inode_size,\
		reserved_blocks, block_size, file

	reader = csv.reader(file)
	for row in reader:
		column = row[0]
		if column == 'SUPERBLOCK':
			superblock_ = row
			blocks_count = int(superblock_[1])
			inode_size = int(superblock_[4])
			block_size = int(superblock_[3])
			inodes_count = int(row[2])
		elif column == 'GROUP':
			group_ = row
			block_num_first_inode = int(group_[8])
			inodes_count_in_group = int(group_[3])
			reserved_blocks = block_num_first_inode + int((inodes_count_in_group*inode_size)/block_size)
		elif column == 'BFREE':
			free_blocks_.append(int(row[1]))
		elif column == 'IFREE':
			global ifree_list
			inum = int(row[1])
			ifree_list.append(inum)
		elif column == 'INODE':
			inodes_.append(row)
			global ialloc_dict
			inum = int(row[1])
			ialloc_dict[inum] = int(row[6])
		elif column == 'DIRENT':
			global dirent_list
			dirent_list.append(row)
		elif column == 'INDIRECT':
			indirects_.append(row)
		else:
			sys.stderr.write("Error: unrecognized contents in csv file\n")

def get_level_str(level):
	if level == 0:
		level_str = "BLOCK"
	elif level == 1:
		level_str = "INDIRECT BLOCK"
	elif level == 2:
		level_str = "DOUBLE INDIRECT BLOCK"
	elif level == 3:
		level_str = "TRIPLE INDIRECT BLOCK"
	return level_str


def invalid_reserved_blocks(block_num, inode_num, offset, level):
	global error
	level_str = get_level_str(level)
	if block_num != 0:
		if (block_num >= blocks_count) or (block_num < 0):
			print("INVALID {} {} IN INODE {} AT OFFSET {}".format(level_str, block_num, inode_num, offset))
			error = 1
		elif block_num < reserved_blocks:
			print("RESERVED {} {} IN INODE {} AT OFFSET {}".format(level_str, block_num, inode_num, offset))
			error = 1
		elif allocated_blocks_count[block_num] > 0:
			allocated_blocks_count[block_num] += 1
			allocated_blocks[block_num].append((block_num, inode_num, offset, level))
		else:
			allocated_blocks_count[block_num] += 1
			allocated_blocks[block_num] = [(block_num, inode_num, offset, level)]

def duplicate_blocks(block_num, inode_num, offset, level):
	level_str = get_level_str(level)
	print("DUPLICATE {} {} IN INODE {} AT OFFSET {}".format(level_str, block_num, inode_num, offset))

def block_audits():
	global error
	global allocated_blocks_count

	allocated_blocks_count = [0] * blocks_count

	for i in range(0, reserved_blocks):
		allocated_blocks_count[i] = 1

	# for each inode
	for inode in inodes_:
		inode_num = inode[1]

		# for each direct block pointer in each inode
		for number in range(12, 27):
			offset = number - 12
			block_num = int(inode[number])
			level = None
			if number < 24:
				level = 0
			elif number == 24:
				offset = 12
				level = 1
			elif number == 25:
				offset = 268
				level = 2
			elif number == 26:
				offset = 65804
				level = 3

			invalid_reserved_blocks(block_num, inode_num, offset, level)

	# for each indirect block pointer
	for indirect in indirects_:
		block_num = int(indirect[5])
		inode_num = indirect[1]
		offset = indirect[3]
		invalid_reserved_blocks(block_num, inode_num, offset, int(indirect[2]))

	# for each block
	for i in range(0, blocks_count):
		if allocated_blocks_count[i] == 0 and i not in free_blocks_:
			error = 1
			print("UNREFERENCED BLOCK {}".format(i))
		if allocated_blocks_count[i] > 0 and i in free_blocks_:
			error = 1
			print("ALLOCATED BLOCK {} ON FREELIST".format(i))
		if allocated_blocks_count[i] > 1:
			for inode in allocated_blocks[i]:
				duplicate_blocks(inode[0], inode[1], inode[2], inode[3])

def Inode_Audit():
	global error
	global inodes_count
	global ifree_list
	global ialloc_dict
	for inum in range(1,inodes_count+1):
		if inum in ifree_list:
			if inum in ialloc_dict:
				error=1
				print("ALLOCATED INODE {} ON FREELIST".format(inum))
		else:
			if inum not in ialloc_dict and inum!=1 and inum!=3 and inum!=4 and inum!=5 and inum!=6 and inum!=7 and inum!=8 and inum != 9 and inum != 10:
				error=1
				print("UNALLOCATED INODE {} NOT ON FREELIST".format(inum))

def Dir_Audit():
	global dirent_list
	global inodes_count
	global ialloc_dict
	global dir_dict
	global parent_dict
	parent_dict[2] = 2
        
	for line in dirent_list:
		
		parent_inum = int(line[1])
		ref_inum = int(line[3])
		ref_name = line[6].rstrip()

                #print "parent_inum:",parent_inum," ref_inum:",ref_inum," ref_name:",ref_name
		if ref_name != "'.'" and ref_name != "'..'":
			parent_dict[ref_inum] = parent_inum
		if ref_inum < 1 or ref_inum > inodes_count:
			error=1
			print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(parent_inum, ref_name, ref_inum))
		elif ref_inum not in ialloc_dict:
			error=1
			print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(parent_inum, ref_name, ref_inum))
		else:
			if ref_inum in dir_dict:
				dir_dict[ref_inum] = dir_dict[ref_inum]+1
			else:
				dir_dict[ref_inum]=1

        #for inum in parent_dict:
                #print "parent of ",inum,"is ",parent_dict[inum]
        #for inum in dir_dict:
                #print "actual links of ", inum,"is ",dir_dict[inum]

        
	for inum in ialloc_dict:
		if inum not in dir_dict and ialloc_dict[inum] != 0:
			error=1
			print("INODE {} HAS 0 LINKS BUT LINKCOUNT IS {}".format(inum, ialloc_dict[inum]))
		elif ialloc_dict[inum] != dir_dict[inum]:
			error=1
			print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(inum, dir_dict[inum], ialloc_dict[inum]))

	for line in dirent_list:
		
		parent_inum = int(line[1])
		ref_inum = int(line[3])
		ref_name = line[6].rstrip()
                #print "parent inum: ",parent_inum, "ref_inum: ",ref_inum, "ref_name: ",ref_name
		if ref_name == "'.'":
			if parent_inum != ref_inum:
				error=1
				print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(parent_inum, ref_inum, parent_inum))
		elif ref_name == "'..'":
			if ref_inum != parent_dict[parent_inum]:
				error=1
				print("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(parent_inum, ref_inum, parent_dict[parent_inum]))


if __name__ == "__main__":
	if len(sys.argv) != 2:
		usage_error()

	filename = sys.argv[1]

	if not os.path.isfile(filename):
		sys.stderr.write('Error: {} is not a file\n'.format(filename))
		exit(EXIT_FAIL)

	try:
		file = open(filename, "r")
	except IOError:
		sys.stderr.write('Error: can\'t open {}\n'.format(filename))
		exit(EXIT_FAIL)

	csv_reader()
	block_audits()
	Inode_Audit()
	Dir_Audit()

	if error == 1:
		exit(EXIT_INCONSISTENSIES)
	exit(EXIT_SUCCESS)
	
