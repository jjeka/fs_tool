#ifndef FAT_FS
#define FAT_FS

#include "FSInterface.h"
#include <msdos_fs.h>

// Version notes:
// 1 - first version
// 2 - added long file names
// 3 - added fat32
#define FAT_VERSION 3

class FatFS : public FSInterface
{
public:

	virtual string get_name() const override;
	virtual int get_version() const override;

	virtual bool init(FILE* file) override;
	virtual string info() override;
	virtual void destroy() override;

	virtual bool ls(fid_t id, vector<FileInfo>& files) override;
	virtual bool cat(fid_t id, std::vector<char>& output) override;

private:

	string get_date_(uint16_t date);
	string get_time_(uint16_t time);
	bool parse_entry_(const msdos_dir_entry& entry, vector<FileInfo>& files);

	FILE* file_;
	string info_;
	int cluster_size_;
	int sector_size_;
	int root_entries_;
	int fat_size_;
	int fat_start_;
	int root_start_;
	int data_start_;
	bool fat32_;
	int root_cluster_;
	int fat_entry_size_;

	string name_;
	bool long_name_;

};

#endif // FAT_FS
