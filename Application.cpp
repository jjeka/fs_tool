#include "Application.h"

Application::Application()
{
	fs_.push_back(std::make_unique<FatFS>());
	fs_.push_back(std::make_unique<Ext2FS>());
}

void Application::main()
{
	printf("Welcome to fs tool by Nikitenko Evgeny!\n");
	printf("Supported FS:\n");

	string supported_fs;
	for (auto& fs : fs_)
	{
		printf("%s (version %d)\n", fs->get_name().c_str(), fs->get_version());
		supported_fs.append(fs->get_name() + " ");
	}
	supported_fs.pop_back();

	printf("\n");

	for (;;)
	{
		printf("Enter disk image path (or ctrl+d to exit):\n");

		string filename;
		if (!(std::cin >> filename))
			break;

		FILE* file = fopen(filename.c_str(), "r");
		if (!file)
		{
			printf("Can't open file %s\n", filename.c_str());
			continue;
		}

		FSInterface* fs = nullptr;
		for (;;)
		{
			printf("Enter FS name ('-' to select disk image). Supported fs: %s\n", supported_fs.c_str());
			string fs_name;
			if (!(std::cin >> fs_name) || fs_name == "-")
				break;

			for (auto& filesystem : fs_)
			{
				if (filesystem->get_name() == fs_name)
				{
					fs = filesystem.get();
					break;
				}
			}

			if (fs)
				break;

			printf("Invalid FS name\n");
		}
		if (!fs)
			continue;

		printf("\nUsing %s (version %d) for %s\n", fs->get_name().c_str(), fs->get_version(), filename.c_str());
		printf("Commands: exit, ls, cat [filename in current directory], cd [dirname], pwd\n");

		work_(fs, file);

		fclose(file);

		printf("Finished working!\n");
	}
}

string Application::get_dir_name_(Path& path)
{
	string name;

	bool empty = true;
	for (string& dir : path)
	{
		name += "/" + dir;
		empty = false;
	}
	if (empty)
		name = "/";

	return name;
}

void Application::work_(FSInterface* fs, FILE* file)
{
	if (!fs->init(file))
	{
		printf("ERROR: Failed open image file!\nInfo: %s\n", fs->info().c_str());
		return;
	}
	printf("FILESYSTEM INFO:\n%s\n", fs->info().c_str());

	Path path;
	vector<FileInfo> dir;
	vector<fid_t> fids;
	if (!fs->ls(ROOT_ID, dir))
	{
		printf("ERROR: Can't ls root directory!\nInfo: %s\n", fs->info().c_str());
		return;
	}

	char cmd_str[MAX_CMD_SIZE] = "";
	fgets(cmd_str, MAX_CMD_SIZE - 1, stdin);

	for (;;)
	{
		printf("> ");
		fgets(cmd_str, MAX_CMD_SIZE - 1, stdin);
		if (feof(stdin))
			break;
		string cmd = cmd_str;

		cmd = trim(cmd);
		size_t pos = cmd.find_first_of(' ', 0);

		string arg;
		string cmdname;

		if (pos != string::npos)
		{
			cmdname = cmd.substr(0, pos);
			arg = cmd.substr(pos + 1, cmd.size());
		}
		else
			cmdname = cmd;
		if (cmd.empty())
			continue;

		if (cmdname == "exit")
			break;
		else if (cmdname == "pwd")
		{
			printf("%s\n", get_dir_name_(path).c_str());
		}
		else if (cmdname == "ls")
		{
			printf("List of %s\n", get_dir_name_(path).c_str());
			bool empty = true;
			for (FileInfo& file : dir)
			{
				empty = false;
				printf("%20s %c %5d | %s\n", file.name.c_str(), file.dir ? 'D' : 'F', file.size,file.info.c_str());
			}
			if (empty)
				printf("Empty directory\n");
		}
		else if (cmdname == "cat")
		{
			if (arg.empty())
			{
				printf("Argument required\n");
				continue;
			}
			printf("Content of %s%s (if file contains \\0 you only see only beginning of file)\n", get_dir_name_(path).c_str(), arg.c_str());
			
			fid_t fid;
			bool found = false;
			bool ok = true;
			for (FileInfo& file : dir)
			{
				if (file.name == arg)
				{
					if (file.dir)
					{
						printf("ERROR: %s is a directory\n", arg.c_str());
						ok = false;
					}
					found = true;
					fid = file.id;
					break;
				}
			}
			if (!found && ok)
				printf("File %s not found in directory\n", arg.c_str());
			else if (ok)
			{
				string output;
				if (!fs->cat(fid, output))
					printf("ERROR: can't cat file! Info: %s\n", fs->info().c_str());
				else
					printf("%s", output.c_str());
			}
		}
		else if (cmdname == "cd")
		{
			if (arg == ".")
				printf("Directory changed to %s\n", get_dir_name_(path).c_str());
			else if (arg == "..")
			{
				if (path.size() == 0)
					printf("Already in root directory\n");
				else
				{
					vector<FileInfo> newdir;
					if (!fs->ls((path.size() != 1) ? fids[fids.size() - 2] : ROOT_ID, newdir))
					{
						printf("ERROR: Can't ls directory %s!\nInfo: %s\n", get_dir_name_(path).c_str(), fs->info().c_str());
					}
					else
					{
						dir = newdir;

						path.pop_back();
						fids.pop_back();

						printf("Directory changed to %s\n", get_dir_name_(path).c_str());
					}
				}
			}
			else
			{
				fid_t fid;
				bool found = false;
				bool ok = true;
				for (FileInfo& file : dir)
				{
					if (file.name == arg)
					{
						if (!file.dir)
						{
							printf("ERROR: %s is not a directory\n", arg.c_str());
							ok = false;
						}
						found = true;
						fid = file.id;
						break;
					}
				}
				if (!found && ok)
					printf("Directory %s not found in directory\n", arg.c_str());
				else if (ok)
				{
					vector<FileInfo> newdir;
					if (!fs->ls(fid, newdir))
					{
						printf("ERROR: Can't ls directory %s!\nInfo: %s\n", get_dir_name_(path).c_str(), fs->info().c_str());
					}
					else
					{
						dir = newdir;

						path.push_back(arg);
						fids.push_back(fid);

						printf("Directory changed to %s\n", get_dir_name_(path).c_str());
					}
				}
			}
		}
		else
		{
			printf("Invalid command. Commands: exit, ls, cat [filename in current directory], cd [dirname], pwd\n");
		}
	}

	fs->destroy();
}
