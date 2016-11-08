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
	
	return true;
}

string Ext2FS::info()
{
	return info_;
}

void Ext2FS::destroy()
{

}

bool Ext2FS::ls(fid_t fid, vector<FileInfo>& files)
{
	return true;
}

bool Ext2FS::cat(fid_t fid, string& output)
{
	return true;
}
