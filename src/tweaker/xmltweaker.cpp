#include "global.h"
#include "xmltweaker_internal.h"
#include <stdio.h>
#include <fstream>
#include <unordered_set>
#include <set>
#include <string.h>
#include "util/util.h"

extern "C" {
#include "wren.h"
}

using namespace std;
using namespace pd2hook;
using namespace tweaker;
using blt::idstring;
using blt::idfile;

static unordered_set<char*> buffers;
static set<idfile> ignored_files;

// The file we last parsed. If we try to parse the same file more than
// once, nothing should happen as a file from the filesystem is being loaded.
idfile last_parsed;

void* tweaker::tweak_pd2_xml(char* text, int text_length)
{
	idfile file = idfile(*blt::platform::last_loaded_name, *blt::platform::last_loaded_ext);

	// Don't parse the same file more than once, as it's not actually the same file.
	if (last_parsed == file)
	{
		return text;
	}

	last_parsed = file;

	// Don't bother with .model or .texture files
	if (
	    file.ext == 0xaf612bbc207e00bd ||	// idstring("model")
	    file.ext == 0x5368e150b05a5b8c		// idstring("texture")
	)
	{
		return text;
	}

	// Check the exclusion list
	if (ignored_files.count(file))
	{
		return text;
	}

	// Make sure the file uses XML. All the files I've seen have no whitespace before their first element,
	// so unless someone has a problem I'll ignore all files that don't have that.
	// TODO make this a bit more thorough, as theres's a small chance a bianry file will start with a '<'
	if (text[0] != '<')
	{
		return text;
	}

	const char* new_text = transform_file(text);

	// If the text is not to be altered, we can return it as is.
	if (new_text == text) return text;

	// Otherwise, copy it so it's not invalidated by another Wren call

	size_t length = strlen(new_text) + 1; // +1 for the null

	char* buffer = (char*)malloc(length);
	buffers.insert(buffer);

	portable_strncpy(buffer, new_text, length);

	//if (!strncmp(new_text, "<network>", 9)) {
	//	std::ofstream out("output.txt");
	//	out << new_text;
	//	out.close();
	//}

	/*static int counter = 0;
	if (counter++ == 1) {
		printf("%s\n", buffer);

		const int kMaxCallers = 62;
		void* callers[kMaxCallers];
		int count = CaptureStackBackTrace(0, kMaxCallers, callers, NULL);
		for (int i = 0; i < count; i++)
			printf("*** %d called from .text:%08X\n", i, callers[i]);
		Sleep(20000);
	}*/

	return (char*)buffer;
}

void tweaker::free_tweaked_pd2_xml(char* text)
{
	if (buffers.erase(text))
	{
		free(text);
	}
}

void pd2hook::tweaker::ignore_file(idfile file)
{
	ignored_files.insert(file);
}

typedef  unsigned long  long ub8;   /* unsigned 8-byte quantities */
typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;

#define hashsize(n) ((ub8)1<<(n))
#define hashmask(n) (hashsize(n)-1)

#define mix64(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>43); \
  b -= c; b -= a; b ^= (a<<9); \
  c -= a; c -= b; c ^= (b>>8); \
  a -= b; a -= c; a ^= (c>>38); \
  b -= c; b -= a; b ^= (a<<23); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>35); \
  b -= c; b -= a; b ^= (a<<49); \
  c -= a; c -= b; c ^= (b>>11); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<18); \
  c -= a; c -= b; c ^= (b>>22); \
}

static idstring Hash64(const ub1* k, ub8 length, ub8 level)
{
	register ub8 a, b, c, len;

	/* Set up the internal state */
	len = length;
	a = b = level;                         /* the previous hash value */
	c = 0x9e3779b97f4a7c13LL; /* the golden ratio; an arbitrary value */

	/*---------------------------------------- handle most of the key */
	while (len >= 24)
	{
		a += (k[0] + ((ub8)k[1] << 8) + ((ub8)k[2] << 16) + ((ub8)k[3] << 24)
		      + ((ub8)k[4] << 32) + ((ub8)k[5] << 40) + ((ub8)k[6] << 48) + ((ub8)k[7] << 56));
		b += (k[8] + ((ub8)k[9] << 8) + ((ub8)k[10] << 16) + ((ub8)k[11] << 24)
		      + ((ub8)k[12] << 32) + ((ub8)k[13] << 40) + ((ub8)k[14] << 48) + ((ub8)k[15] << 56));
		c += (k[16] + ((ub8)k[17] << 8) + ((ub8)k[18] << 16) + ((ub8)k[19] << 24)
		      + ((ub8)k[20] << 32) + ((ub8)k[21] << 40) + ((ub8)k[22] << 48) + ((ub8)k[23] << 56));
		mix64(a, b, c);
		k += 24;
		len -= 24;
	}

	/*------------------------------------- handle the last 23 bytes */
	c += length;
	switch (len)              /* all the case statements fall through */
	{
	case 23:
		c += ((ub8)k[22] << 56);
	case 22:
		c += ((ub8)k[21] << 48);
	case 21:
		c += ((ub8)k[20] << 40);
	case 20:
		c += ((ub8)k[19] << 32);
	case 19:
		c += ((ub8)k[18] << 24);
	case 18:
		c += ((ub8)k[17] << 16);
	case 17:
		c += ((ub8)k[16] << 8);
	/* the first byte of c is reserved for the length */
	case 16:
		b += ((ub8)k[15] << 56);
	case 15:
		b += ((ub8)k[14] << 48);
	case 14:
		b += ((ub8)k[13] << 40);
	case 13:
		b += ((ub8)k[12] << 32);
	case 12:
		b += ((ub8)k[11] << 24);
	case 11:
		b += ((ub8)k[10] << 16);
	case 10:
		b += ((ub8)k[9] << 8);
	case  9:
		b += ((ub8)k[8]);
	case  8:
		a += ((ub8)k[7] << 56);
	case  7:
		a += ((ub8)k[6] << 48);
	case  6:
		a += ((ub8)k[5] << 40);
	case  5:
		a += ((ub8)k[4] << 32);
	case  4:
		a += ((ub8)k[3] << 24);
	case  3:
		a += ((ub8)k[2] << 16);
	case  2:
		a += ((ub8)k[1] << 8);
	case  1:
		a += ((ub8)k[0]);
		/* case 0: nothing left to add */
	}
	mix64(a, b, c);
	/*-------------------------------------------- report the result */
	return c;
}

idstring tweaker::idstring_hash(string text)
{
	return Hash64((const unsigned char*)text.c_str(), text.length(), 0);
}
