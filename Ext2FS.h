#ifndef EXT2_FS
#define EXT2_FS

#include "FSInterface.h"

// Version notes:
// 1 - first version
#define EXT2_VERSION 1

class Ext2FS : public FSInterface
{
public:

	virtual string get_name() override;
	virtual int get_version() override;

	virtual bool init(FILE* file) override;
	virtual string info() override;
	virtual void destroy() override;

	virtual bool ls(fid_t id, vector<FileInfo>& files) override;
	virtual bool cat(fid_t id, string& output) override;

private:

	string info_;

};

#endif // EXT2_FS
