#include "FatFS.h"

string FatFS::get_name()
{
	return "fat";
}

int FatFS::get_version()
{
	return FAT_VERSION;
}

bool FatFS::init(FILE* file)
{
	file_ = file;

	fat_boot_sector sector;
	fread(&sector, sizeof (sector), 1, file);

	char sysid[32] = "";
	strncpy(sysid, (char*) sector.system_id, 8);

	sector_size_ = sector.sector_size[0] + sector.sector_size[1] * 256;
	cluster_size_ = sector.sec_per_clus * sector_size_;
	root_entries_ = sector.dir_entries[0] + sector.dir_entries[1] * 256;
	fat32_ = (root_entries_ == 0);
	fat_size_ = sector_size_ * (fat32_ ? sector.fat32.length : sector.fat_length);
	fat_start_ = sector_size_ * sector.reserved;
	root_start_ = fat_start_ + fat_size_ * sector.fats;
	data_start_ = root_start_ + root_entries_ * 32;
	fat_entry_size_ = (fat32_ ? 4 : 2);
	if (fat32_)
		root_cluster_ = sector.fat32.root_cluster;

	info_ = fat32_ ? "FAT32 filesystem\n" : "FAT16 filesystem\n";
	info_ += "System id: "s + sysid + "\n";
	info_ += "Number of fats: "s + std::to_string(sector.fats) + "\n";
	if (!fat32_)
	{
		char label[32] = "";
		strncpy(label, (char*) sector.fat16.vol_label, 11);
		info_ += "Volume label: "s + (char*) label + "\n";
	}
	else
	{
		char label[32] = "";
		strncpy(label, (char*) sector.fat32.vol_label, 11);
		info_ += "Volume label: "s + (char*) label + "\n";
		char type[32] = "";
		strncpy(type, (char*) sector.fat32.fs_type, 8);
		info_ += "FS type: "s + (char*) type + "\n";
	}
	info_ += "Cluster size: "s + std::to_string(cluster_size_) + " bytes\n";
	return true;
}

string FatFS::info()
{
	return info_;
}

void FatFS::destroy()
{

}

string FatFS::get_date_(uint16_t date)
{
	return std::to_string(date & ((1 << 5) - 1)) + "." + std::to_string((date >> 5) & ((1 << 4) - 1)) + "." + 
		std::to_string((date >> 9) + 1980);
}

string FatFS::get_time_(uint16_t time)
{
	return to_string_time(time >> 11) + ":" + to_string_time((time >> 5) & ((1 << 6) - 1)) + ":" + 
		to_string_time((time & ((1 << 5) - 1)) * 2);
}

bool FatFS::ls(fid_t fid, vector<FileInfo>& files)
{
	name_ = "";
	long_name_ = false;

	if (fid == ROOT_ID && !fat32_)
	{
		fseek(file_, root_start_, SEEK_SET);
		for (int i = 0; i < root_entries_; i++)
		{
			msdos_dir_entry entry;
			fread(&entry, sizeof (entry), 1, file_);
			if (!parse_entry_(entry, files))
				break;
		}
	}
	else
	{
		msdos_dir_entry entry;

		int cluster;
		if (fat32_ && fid == ROOT_ID)
			cluster = root_cluster_ - 2;
		else
		{
			fseek(file_, fid, SEEK_SET);
			fread(&entry, sizeof (entry), 1, file_);
			if (entry.size < 0)
			{
				info_ = "invalid size value (needed 0): " + std::to_string(entry.size);
				return false;
			}

			cluster = entry.start + (fat32_ ? (entry.starthi << 16) : 0) - 2;
		}

		bool ready = false;
		while (!ready)
		{
			size_t seek = data_start_ + cluster * cluster_size_;
			fseek(file_, seek, SEEK_SET);

			for (int i = 0; i < cluster_size_ / 32; i++)
			{
				msdos_dir_entry entry;
				fread(&entry, sizeof (entry), 1, file_);
				if (!parse_entry_(entry, files))
				{
					ready = true;
					break;
				}
			}

			if (!ready)
			{
				fseek(file_, fat_start_ + (2 + cluster) * fat_entry_size_, SEEK_SET);
				int new_cluster;
				if (fat32_)
				{
					fread(&new_cluster, 4, 1, file_);
					new_cluster &= (1 << 28) - 1;
				}
				else
					fread(&new_cluster, 2, 1, file_);

				cluster = new_cluster - 2;
			}
		}
	}
	return true;
}

bool FatFS::parse_entry_(msdos_dir_entry& entry, vector<FileInfo>& files)
{
	if (entry.name[0] == 0x00)
		return false;
	if (entry.name[0] == 0xE5)
		return true;
	if (entry.attr == 0xF) // vfat long file name
	{
		long_name_ = true;
		const int addresses[] = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 }; // name addresses
		char str[14] = "";
		for (int i = 0; i < 13; i++)
			str[i] = ((char*) &entry)[addresses[i]];
		name_ = str + name_;

		return true;
	}

	FileInfo file;
	
	char tmp[10] = "";
	strncpy(tmp, (char*) entry.name, 5);
	for (int j = 5; j >= 0; j--)
		if (tmp[j] == ' ')
			tmp[j] = 0;
	if (!(entry.attr & (1 << 4))) // if not dir
		strcat(tmp, ".");
	strncat(tmp, (char*) &entry.name[5], 3);
	int s = strlen(tmp);
	for (int j = s - 1; j >= s - 3; j--)
		if (tmp[j] == ' ')
			tmp[j] = 0;
	if ((!(entry.attr & (1 << 4))) && entry.name[5] == ' ' && entry.name[6] == ' ' && entry.name[7] == ' ')
		tmp[strlen(tmp) - 1] = 0;

	file.info = "";
	if (!long_name_)
		file.name = tmp;
	else
		file.info = "short name: "s + tmp + "; ";
	
	if (long_name_)
	{
		file.name = name_;

		name_ = "";
		long_name_ = false;
	}

	file.size = entry.size;
	file.dir = entry.attr & (1 << 4);

	file.id = ftell(file_) - sizeof (entry);
	if (entry.attr & (1 << 0)) file.info += "ro ";
	if (entry.attr & (1 << 1)) file.info += "hidden ";
	if (entry.attr & (1 << 2)) file.info += "system ";
	if (entry.attr & (1 << 3)) file.info += "volumelabel ";
	if (entry.attr & (1 << 5)) file.info += "archive ";
	file.info += "creation time: " + get_date_(entry.cdate) + " " + get_time_(entry.ctime) + "; ";
	file.info += "modify time: " + get_date_(entry.date) + " " + get_time_(entry.time) + "; ";
	file.info += "access time: " + get_date_(entry.adate) + "; ";

	files.push_back(file);
	return true;
}

bool FatFS::cat(fid_t fid, string& output)
{
	msdos_dir_entry entry;
	fseek(file_, fid, SEEK_SET);
	fread(&entry, sizeof (entry), 1, file_);
	if (entry.size < 0)
	{
		info_ = "invalid size value: " + std::to_string(entry.size);
		return false;
	}

	char* buf = new char[entry.size + 1];
	memset(buf, 0, entry.size + 1);

	int num_clusters = entry.size / cluster_size_ + (entry.size % cluster_size_ ? 1 : 0);
	int cluster = entry.start + (fat32_ ? (entry.starthi << 16) : 0) - 2;

	for (int i = 0; i < num_clusters; i++)
	{
		int size = (i != num_clusters - 1) ? cluster_size_ : (entry.size % cluster_size_);
		if (size == 0)
			size = cluster_size_;

		size_t seek = data_start_ + cluster * cluster_size_;
		fseek(file_, seek, SEEK_SET);
		fread(&buf[i * cluster_size_], 1, size, file_);
		
		if (i != num_clusters - 1)
		{
			fseek(file_, fat_start_ + (2 + cluster) * fat_entry_size_, SEEK_SET);
			int new_cluster;
			if (fat32_)
			{
				fread(&new_cluster, 4, 1, file_);
				new_cluster &= (1 << 28) - 1;
			}
			else
				fread(&new_cluster, 2, 1, file_);

			cluster = new_cluster - 2;
		}
	}

	output = buf;
	delete[] buf;

	return true;
}
