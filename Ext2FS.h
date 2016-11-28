#ifndef EXT2_FS
#define EXT2_FS

#include "FSInterface.h"
#include <ext2_fs.h>

// Version notes:
// 1 - first version
#define EXT2_VERSION 1

class Ext2FS : public FSInterface
{
public:

	virtual string get_name() const override;
	virtual int get_version() const override;

	virtual bool init(FILE* file) override;
	virtual string info() override;
	virtual void destroy() override;

	virtual bool ls(fid_t id, vector<FileInfo>& files) override;
	virtual bool cat(fid_t id, string& output) override;

private:

	string get_time_string_(int32_t time);
	ext2_inode get_inode_(int64_t n);
	int64_t get_block_pos_(const ext2_inode& inode, int64_t n);

	FILE* file_;
	string info_;
	int block_size_;
	int inodes_per_group_;

};

#endif // EXT2_FS
