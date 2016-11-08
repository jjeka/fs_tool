#ifndef APPLICATION
#define APPLICATION

#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <iostream>
#include "Utils.h"
#include "FSInterface.h"

#include "FatFS.h"
#include "Ext2FS.h"

using std::string;
using std::vector;
using std::unique_ptr;

#define MAX_CMD_SIZE	1024

typedef vector<string> Path;

class Application
{
public:

	Application();

	void main();

private:

	void work_(FSInterface* fs, FILE* file);
	string get_dir_name_(Path& path);
	vector<unique_ptr<FSInterface>> fs_;

};

#endif // APPLICATION
