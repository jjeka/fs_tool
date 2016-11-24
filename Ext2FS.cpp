#include "Ext2FS.h"

string Ext2FS::get_name()
{
	return "ext2";
}

int Ext2FS::get_version()
{
	return EXT2_VERSION;
}

bool Ext2FS::init(FILE* file)
{
	file_ = file;

	ext2_super_block block;
	fseek(file, 1024, SEEK_SET);
	fread(&block, sizeof (block), 1, file);

	if (block.s_magic != 0xEF53)
	{
		info_ = "Not ext2 fs. Magic is not 0xEF53: " + std::to_string(block.s_magic);
		return false;
	}
	if (block.s_feature_incompat != EXT2_FEATURE_INCOMPAT_FILETYPE)
	{
		info_ = "ext2 has incompatible extentions: " + std::to_string(block.s_feature_incompat);
		return false;
	}

	char volume[EXT2_LABEL_LEN + 1] = "";
	strncpy(volume, (char*) block.s_volume_name, EXT2_LABEL_LEN);
	info_ = "Volume name: "s + volume + "\n";

	char mount[65] = "";
	strncpy(mount, (char*) block.s_last_mounted, 64);
	info_ += "Last mounted point: "s + mount + "\n";
	info_ += "Inodes count: " + std::to_string(block.s_inodes_count) + "\n"; 
	info_ += "Blocks count: " + std::to_string(block.s_blocks_count); 

	block_size_ = 1 << (10 + block.s_log_block_size);
	inodes_per_group_ = block.s_inodes_per_group;

	return true;
}

string Ext2FS::info()
{
	return info_;
}

void Ext2FS::destroy()
{

}

ext2_inode Ext2FS::get_inode_(int64_t n)
{
	int64_t group = (n - 1) / inodes_per_group_;
	int64_t index = (n - 1) % inodes_per_group_;

	ext2_group_desc desc_table;
	fseek(file_, 1024 + sizeof (ext2_super_block) + sizeof (ext2_group_desc) * group, SEEK_SET);
	fread(&desc_table, 1, sizeof (ext2_group_desc), file_);

	fseek(file_, (desc_table.bg_inode_table * block_size_ + index * sizeof (ext2_inode)), SEEK_SET);
	
	ext2_inode inode;
	fread(&inode, sizeof (ext2_inode), 1, file_);

	return inode;
}

string Ext2FS::get_time_string_(int32_t time)
{
	time_t t = time;
	tm* tm_struct = localtime(&t);
	char date[256];
	strftime(date, sizeof (date), "%d.%m.%Y %H:%M:%S", tm_struct);
	return date;
}


int64_t Ext2FS::get_block_pos_(ext2_inode& inode, int64_t n)
{
	int addresses_per_block = block_size_ / sizeof (int32_t);

	int64_t n2 = n - EXT2_IND_BLOCK - addresses_per_block;
	int64_t n3 = n2 - addresses_per_block * addresses_per_block;
	if (n < EXT2_IND_BLOCK)
	{
		return inode.i_block[n] * block_size_;
	}
	else if (n - EXT2_IND_BLOCK <= addresses_per_block)
	{
		fseek(file_, inode.i_block[EXT2_IND_BLOCK] * block_size_ + (n - EXT2_IND_BLOCK) * sizeof (int32_t), SEEK_SET);

		int32_t pos;
		fread(&pos, sizeof (int32_t), 1, file_);
		return int64_t(pos) * block_size_;
	}
	else if (n2 <= addresses_per_block * addresses_per_block)
	{
		fseek(file_, inode.i_block[EXT2_DIND_BLOCK] * block_size_ + (n2 / addresses_per_block) * sizeof (int32_t), SEEK_SET);
		int32_t pos1;
		fread(&pos1, sizeof (int32_t), 1, file_);

		fseek(file_, pos1 * block_size_ + (n2 % addresses_per_block) * sizeof (int32_t), SEEK_SET);
		int32_t pos2;
		fread(&pos2, sizeof (int32_t), 1, file_);

		return int64_t(pos2) * block_size_;
	}
	else if (n3 <= addresses_per_block * addresses_per_block)
	{
		fseek(file_, inode.i_block[EXT2_TIND_BLOCK] * block_size_ + (n3 / addresses_per_block / addresses_per_block) * sizeof (int32_t), SEEK_SET);
		int32_t pos1;
		fread(&pos1, sizeof (int32_t), 1, file_);

		fseek(file_, pos1 * block_size_ + ((n3 / addresses_per_block) % addresses_per_block) * sizeof (int32_t), SEEK_SET);
		int32_t pos2;
		fread(&pos2, sizeof (int32_t), 1, file_);

		fseek(file_, pos2 * block_size_ + (n3 % addresses_per_block) * sizeof (int32_t), SEEK_SET);
		int32_t pos3;
		fread(&pos3, sizeof (int32_t), 1, file_);

		return int64_t(pos3) * block_size_;
	}
	else
		return -1;
}

bool Ext2FS::ls(fid_t fid, vector<FileInfo>& files)
{
	ext2_inode inode = get_inode_((fid == ROOT_ID) ? 2 : fid);

	uint64_t read_bytes = 0;
	for (int64_t i = 0; i < inode.i_blocks; i++)
 	{
		int64_t block_pos = get_block_pos_(inode, i);
		fseek(file_, block_pos, SEEK_SET);

 		ext2_dir_entry_2 dir_entry;
		fread(&dir_entry, sizeof (uint8_t) * 8, 1, file_);
	 	
		int read_bytes_block = 0;
		while (read_bytes < inode.i_size && read_bytes_block < block_size_)
		{
	 		char name[EXT2_NAME_LEN] = "";
	 		fread(name, dir_entry.name_len, 1, file_);
			read_bytes += dir_entry.rec_len;
			read_bytes_block += dir_entry.rec_len;

			if (dir_entry.inode != 0)
			{
				ext2_inode entry_inode = get_inode_(dir_entry.inode);

				FileInfo file;
				file.name = name;
				file.id = dir_entry.inode;
				file.dir = (dir_entry.file_type & EXT2_FT_DIR) != 0;
				file.size = entry_inode.i_size;

				if ((dir_entry.file_type & (1 << EXT2_FT_CHRDEV )) != 0) file.info += "chardevice ";
				if ((dir_entry.file_type & (1 << EXT2_FT_BLKDEV )) != 0) file.info += "blockdevice ";
				if ((dir_entry.file_type & (1 << EXT2_FT_FIFO   )) != 0) file.info += "fifo ";
				if ((dir_entry.file_type & (1 << EXT2_FT_SOCK   )) != 0) file.info += "socket ";
				if ((dir_entry.file_type & (1 << EXT2_FT_SYMLINK)) != 0) file.info += "symlink ";

				file.info += "creation time: " + get_time_string_(entry_inode.i_ctime) + "; ";
				file.info += "modify time: " + get_time_string_(entry_inode.i_mtime) + "; ";
				file.info += "access time: " + get_time_string_(entry_inode.i_atime) + "; ";

				files.push_back(file);
			}

			fseek(file_, block_pos + read_bytes_block, SEEK_SET);
			fread(&dir_entry, sizeof (uint8_t) * 8, 1, file_);
		}
	}

	return true;
}

bool Ext2FS::cat(fid_t fid, string& output)
{
	ext2_inode inode = get_inode_(fid);

	char* buf = new char[inode.i_size + 1];
	memset(buf, 0, inode.i_size + 1);

	for (int64_t i = 0; i < inode.i_blocks && i * block_size_ < inode.i_size; i++)
 	{
		int64_t block_pos = get_block_pos_(inode, i);
		if (block_pos == -1)
		{
			info_ = "File size is too big";
			delete[] buf;
			return false;
		}

		fseek(file_, block_pos, SEEK_SET);

		int64_t size = (inode.i_size - i * block_size_ >= block_size_) ? block_size_ : inode.i_size % block_size_;
		fread(&buf[i * block_size_], 1, size, file_);
	}

	output = buf;
	delete[] buf;
	return true;
}
