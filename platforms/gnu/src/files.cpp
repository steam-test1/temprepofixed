#include "util/util.h"

#include <blt/fs.hh>
#include <blt/log.hh>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <ftw.h>
#include <string.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <iomanip>
#include <stack>
#include <algorithm>

#include <openssl/sha.h>

// PATH_MAX
#include <limits.h>

namespace pd2hook
{
	namespace Util
	{

		using std::string;
		using std::to_string;
		using std::stringstream;
		using std::vector;
		using std::stack;
		using std::ifstream;

		/**
		 * Compute the contents of a directory and return them in vector
		 */
		vector<string>
		GetDirectoryContents(const string &path, bool listDirs)
		{

			vector<string> result;

			DIR* directory = opendir(path.c_str());


			if (directory)
			{
				struct dirent* next = readdir(directory);

				while (next)
				{
					bool isDir;
					{
						if(next->d_type == DT_LNK || next->d_type == DT_UNKNOWN)
						{
							char current_path[PATH_MAX + 1];
							snprintf(current_path, PATH_MAX, "%s/%s", path.c_str(), next->d_name);

							// this assumes that stat will dereference N-deep symlinks rather than single symlinks
							struct stat estat;
							stat(current_path, &estat);

							isDir = S_ISDIR(estat.st_mode);
						}
						else
						{
							isDir = (next->d_type == DT_DIR);
						}
					}

					if ((listDirs && isDir) || (!listDirs && !isDir))
					{
						result.push_back(next->d_name);
					}

					next = readdir(directory);
				}

				closedir(directory);
			}


			return result;
		}

		/**
		 * Check if a path exists, and is a directory.
		 */
		bool
		DirectoryExists(const string &path)
		{
			struct stat fileInfo;

			if (stat(path.c_str(), &fileInfo) == 0)
			{
				return S_ISDIR(fileInfo.st_mode);
			}

			return false;
		}

		bool
		CreateDirectorySingle(const string &path)
		{
			return mkdir(path.c_str(), 0777) == 0;
		}

		bool
		RemoveEmptyDirectory(const string &path)
		{
			return remove(path.c_str());
		}

		/*
		 * delete_directory
		 */
		// Note that BLT4L didn't correctly implement the API in the first place - file.RemoveDirectory only removes an empty directory
		/*
		int
		fs_delete_dir(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
		{
		    return remove( fpath );
		}

		bool
		delete_directory(string path, bool descend)
		{
		    if (!descend)
		    {
		        return remove(path.c_str());
		    }
		    else
		    {
		        return nftw(path.c_str(), fs_delete_dir, / * max open fd's * / 128, FTW_DEPTH | FTW_PHYS);
		    }
		}
		*/

		bool
		MoveDirectory(const std::string & path, const std::string & destination)
		{
			return rename(path.c_str(), destination.c_str()) == 0;
		}

		FileType GetFileType(const std::string &path)
		{
			struct stat fileInfo;

			if (stat(path.c_str(), &fileInfo))
			{
				return FileType_None;
			}

			if(S_ISDIR(fileInfo.st_mode))
			{
				return FileType_Directory;
			}
			else
			{
				return FileType_File;
			}
		}
	}
}

/* vim: set ts=4 softtabstop=0 sw=4 expandtab: */



