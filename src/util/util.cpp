#include "util.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <openssl/sha.h>

namespace pd2hook
{
	namespace Util
	{

		Exception::Exception(const char *file, int line) :
			mFile(file), mLine(line)
		{}

		Exception::Exception(std::string msg, const char *file, int line) :
			mFile(file), mLine(line), mMsg(std::move(msg))
		{}

		const char *Exception::what() const throw()
		{
			if (!mMsg.empty())
			{
				return mMsg.c_str();
			}

			return std::exception::what();
		}

		const char *Exception::exceptionName() const
		{
			return "An exception";
		}

		void Exception::writeToStream(std::ostream& os) const
		{
			os << exceptionName() << " occurred @ (" << mFile << ':' << mLine << "). " << what();
		}

		std::string sha256(const std::string str)
		{
			unsigned char hash[SHA256_DIGEST_LENGTH];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, str.c_str(), str.size());
			SHA256_Final(hash, &sha256);
			std::stringstream ss;
			for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
			{
				ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
			}
			return ss.str();
		}

		void RecurseDirectoryPaths(std::vector<std::string>& paths, std::string directory, bool ignore_versioning)
		{
			std::vector<std::string> dirs = pd2hook::Util::GetDirectoryContents(directory, true);
			std::vector<std::string> files = pd2hook::Util::GetDirectoryContents(directory);
			for (auto it = files.begin(); it < files.end(); it++)
			{
				std::string fpath = directory + *it;

				// Add the path on the list
				paths.push_back(fpath);
			}
			for (auto it = dirs.begin(); it < dirs.end(); it++)
			{
				if (*it == "." || *it == "..") continue;
				if (ignore_versioning && (*it == ".hg" || *it == ".git")) continue;
				RecurseDirectoryPaths(paths, directory + *it + "/", false);
			}
		}

		static bool CompareStringsCaseInsensitive(std::string a, std::string b)
		{
			std::transform(a.begin(), a.end(), a.begin(), ::tolower);
			std::transform(b.begin(), b.end(), b.begin(), ::tolower);

			return a < b;
		}

		std::string GetDirectoryHash(std::string directory)
		{
			std::vector<std::string> paths;
			RecurseDirectoryPaths(paths, directory, true);

			// Case-insensitive sort, since that's how it was always done.
			// (on Windows, the filenames were previously downcased in RecurseDirectoryPaths, but that
			//  obviously won't work with a case-sensitive filesystem)
			// If I were to rewrite BLT from scratch I'd certainly make this case-sensitive, but there's no good
			//  way to change this without breaking hashing on previous versions.
			std::sort(paths.begin(), paths.end(), CompareStringsCaseInsensitive);

			std::string hashconcat;

			for (auto it = paths.begin(); it < paths.end(); it++)
			{
				std::string hashstr = sha256(pd2hook::Util::GetFileContents(*it));
				hashconcat += hashstr;
			}

			return sha256(hashconcat);
		}

		std::string GetFileHash(std::string file)
		{
			// This has to be hashed twice otherwise it won't be the same hash if we're checking against a file uploaded to the server
			std::string hash = sha256(pd2hook::Util::GetFileContents(file));
			return sha256(hash);
		}

	}
}
