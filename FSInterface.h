#ifndef FS_INTERFACE
#define FS_INTERFACE

#include <string>
#include <vector>
#include "Utils.h"

using std::string;
using std::vector;

typedef uint64_t fid_t;

struct FileInfo
{
	string name;
	bool dir;
	int size;
	string info;
	fid_t id;
};

#define ROOT_ID 0

class FSInterface
{
public:

	virtual string get_name() const = 0;
	virtual int get_version() const = 0;

	virtual bool init(FILE* file) = 0;
	virtual string info() = 0;
	virtual void destroy() = 0;

	virtual bool ls(fid_t id, vector<FileInfo>& files) = 0;
	virtual bool cat(fid_t id, string& output) = 0;
};

#endif // FS_INTERFACE
