#include <iostream>
#include <fstream>
#include <map>
#include <Windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include "subhook.h"

#define SIG_INCLUDE_MAIN
#include "sigdef.h"
#undef SIG_INCLUDE_MAIN

#include "util/util.h"
#include "signatures.h"

using std::string;
using std::to_string;

class SignatureCacheDB
{
public:
	SignatureCacheDB(string filename) : filename(filename)
	{
		std::ifstream infile(filename, std::ios::binary);
		if (!infile.good())
		{
			PD2HOOK_LOG_WARN("Could not open signature cache file");
			return;
		}

#define READ_BIN(var) infile.read((char*) &var, sizeof(var));

		uint32_t revision;
		READ_BIN(revision); // TODO if the file is EOF, exit
		if (revision != CACHEDB_REVISION)
		{
			// Using a differnt revision, can't safely use it.
			// Not a big deal, just search properly for signatures this time.
			PD2HOOK_LOG_WARN("Discarding signature cache data, different revision");
			return;
		}

		uint32_t count;
		READ_BIN(count); // TODO if the file is EOF, exit

		for (size_t i = 0; i < count; i++)
		{
			uint32_t length;
			READ_BIN(length);
			if (length > BUFF_LEN)
			{
				PD2HOOK_LOG_ERROR("Cannot read long signature name!");
				locations.clear();
				return;
			}

			char name[BUFF_LEN];
			infile.read(name, length);
			string name_str = string(name, length);

			DWORD address;
			READ_BIN(address);

			locations[name_str] = address;
		}

#undef READ_BIN
	}

	DWORD GetAddress(string name)
	{
		if (locations.count(name))
		{
			return locations[name];
		}
		else
		{
			return -1;
		}
	}

	void UpdateAddress(string name, DWORD address)
	{
		if (name.length() > BUFF_LEN)
		{
			string msg = "Cannot write long signature name!";
			PD2HOOK_LOG_ERROR(msg);
			throw msg;
		}
		locations[name] = address;
	}

	void Save()
	{
		std::ofstream outfile(filename, std::ios::binary);
		if (!outfile.good())
		{
			PD2HOOK_LOG_ERROR("Could not open signature cachefile for saving");
			return;
		}

#define WRITE_BIN(var) outfile.write((char*) &var, sizeof(var))

		uint32_t revision = CACHEDB_REVISION;
		WRITE_BIN(revision);

		uint32_t count = locations.size();
		WRITE_BIN(count);

		PD2HOOK_LOG_LOG(string("Saving ") + to_string(count) + string(" signatures"));

		for (auto const &sig : locations)
		{
			// name length
			uint32_t length = sig.first.length();
			WRITE_BIN(length);

			// name
			outfile.write(sig.first.c_str(), length);

			// address
			DWORD address = sig.second;
			WRITE_BIN(address);
		}

		PD2HOOK_LOG_LOG("Done saving signatures");

#undef READ_BIN
	}
private:
	const string filename;
	std::map<string, DWORD> locations;

	static const uint32_t CACHEDB_REVISION = 1;
	static const uint32_t BUFF_LEN = 1024;
};

MODULEINFO GetModuleInfo(string szModule)
{
	MODULEINFO modinfo = { 0 };
	HMODULE hModule = GetModuleHandle(szModule.c_str());
	if (hModule == 0)
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}

static bool CheckSignature(const char* pattern, DWORD patternLength, const char* mask, DWORD base, DWORD size, DWORD i, DWORD *result)
{
	bool found = true;
	for (DWORD j = 0; j < patternLength; j++)
	{
		found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
	}
	if (found)
	{
		// printf("Found %s: 0x%p\n", funcname, base + i);
		*result = base + i;
		return true;
	}

	return false;
}

DWORD FindPattern(char* module, const char* funcname, const char* pattern, const char* mask, DWORD hint, bool *hintCorrect, DWORD *hintOut)
{
	*hintOut = NULL;

	MODULEINFO mInfo = GetModuleInfo(module);
	DWORD base = (DWORD)mInfo.lpBaseOfDll;
	DWORD size = (DWORD)mInfo.SizeOfImage;
	DWORD patternLength = (DWORD)strlen(mask);

	if (hint >= 0 && hint < size - patternLength)
	{
		DWORD result;
		*hintCorrect = CheckSignature(pattern, patternLength, mask, base, size, hint, &result);
		if (*hintCorrect) return result;
	}
	else
	{
		*hintCorrect = false;
	}

	for (DWORD i = 0; i < size - patternLength; i++)
	{
		DWORD result;
		bool correct = CheckSignature(pattern, patternLength, mask, base, size, i, &result);
		if (correct)
		{
#ifdef CHECK_DUPLICATE_SIGNATURES
			// Sigdup checking
			for (DWORD ci = i + 1; ci < size - patternLength; ci++)
			{
				DWORD addr;
				bool correct = CheckSignature(pattern, patternLength, mask, base, size, ci, &addr);
				if (correct)
				{
					string err = string("Found duplicate signature for ") + string(funcname) + string(" at ") + to_string(result) + string(",") + to_string(addr);
					PD2HOOK_LOG_WARN(err);
					hintOut = NULL; // Don't cache sigs with errors
				}
			}
#endif

			if (hintOut) *hintOut = i;
			return result;
		}
	}
	PD2HOOK_LOG_WARN(string("Failed to locate function ") + string(funcname));
	return NULL;
}

std::vector<SignatureF>* allSignatures = NULL;

SignatureSearch::SignatureSearch(const char* funcname, void* adress, const char* signature, const char* mask, int offset, SignatureVR vr)
{
	// lazy-init, container gets 'emptied' when initialized on compile.
	if (!allSignatures)
	{
		allSignatures = new std::vector<SignatureF>();
	}

	SignatureF ins = { funcname, signature, mask, offset, adress, vr };
	allSignatures->push_back(ins);
}

void SignatureSearch::Search()
{
	// Find the name of the current EXE
	TCHAR processPath[MAX_PATH + 1];
	GetModuleFileName(NULL, processPath, MAX_PATH + 1); // Get the path
	TCHAR filename[MAX_PATH + 1];
	_splitpath_s( // Find the filename part of the path
	    processPath, // Input
	    NULL, 0, // Don't care about the drive letter
	    NULL, 0, // Don't care about the directory
	    filename, MAX_PATH, // Grab the filename
	    NULL, 0 // Extension is always .exe
	);

	string basename = filename;

	// Check if the user is in VR (EXE ends with _vr)
	// This is used to only load functions relevant to that version, as
	//  there are a couple of functions that have different signatures in
	//  VR as they do in the desktop binary.
	bool is_in_vr = basename.rfind("_vr") == basename.length() - 3;

	// Add the .exe back on
	strcat_s(filename, MAX_PATH, ".exe");

	unsigned long ms_start = GetTickCount();
	SignatureCacheDB cache(string("sigcache_") + basename + string(".db"));
	PD2HOOK_LOG_LOG(string("Scanning for signatures in ") + string(filename));

	int cacheMisses = 0;
	std::vector<SignatureF>::iterator it;
	for (it = allSignatures->begin(); it < allSignatures->end(); it++)
	{
		// If the function is desktop-only and we're in VR (or vise-versa), skip it
		// This *significantly* improves loading times - on my system it cut loading times
		//  by ~2.5 seconds.
		if (it->vr == (is_in_vr ? SignatureVR_Desktop : SignatureVR_VR)) {
			*(void**)it->address = NULL;
			continue;
		}

		string funcname = it->funcname;
		DWORD hint = cache.GetAddress(funcname);

		bool hintCorrect;
		DWORD hintOut = NULL;
		DWORD addr = (FindPattern(filename, it->funcname, it->signature, it->mask, hint, &hintCorrect, &hintOut) + it->offset);
		*((void**)it->address) = (void*)addr;

		if (addr == NULL)
		{
			hintCorrect = true; // If the signature doesn't exist at all, it's not the cache's fault
		}
		else if (hint == -1 && addr != NULL)
		{
			PD2HOOK_LOG_LOG(string("Sigcache hit failed for function ") + funcname);
		}
		else if (!hintCorrect)
		{
			PD2HOOK_LOG_WARN(string("Sigcache for function ") + funcname + " incorrect (" + to_string(hint) + " vs " + to_string(hintOut) + ")!");
		}

		if (!hintCorrect && hintOut != NULL)
		{
			cache.UpdateAddress(funcname, hintOut);
			cacheMisses++;
		}
	}

	unsigned long ms_end = GetTickCount();

	PD2HOOK_LOG_LOG(string("Scanned for ") + to_string(allSignatures->size()) + string(" signatures in ")
	                + to_string((int)(ms_end - ms_start)) + string(" milliseconds with ") + to_string(cacheMisses)
	                + string(" cache misses"));

	if (cacheMisses > 0)
	{
		PD2HOOK_LOG_LOG("Saving signature cache");
		cache.Save();
	}
}

void* SignatureSearch::GetFunctionByName(const char* name)
{
	if (!allSignatures) return NULL;

	for (const auto& sig : *allSignatures)
	{
		if (!strcmp(sig.funcname, name))
		{
			return *(void**)sig.address;
		}
	}

	return NULL;
}
