#include "xmltweaker_internal.h"
#include <stdio.h>
#include <fstream>
#include <unordered_set>
#include "util/util.h"
#include "signatures/sigdef.h"

extern "C" {
#include "wren.h"
}

using namespace std;
using namespace pd2hook;
using namespace tweaker;

static unordered_set<char*> buffers;

idstring *tweaker::last_loaded_name = NULL, *tweaker::last_loaded_ext = NULL;

void tweaker::init_xml_tweaker() {
	char *tmp;

	if (try_open_base_vr) {
		tmp = (char*)try_open_base_vr;
		tmp += 0x3D;
		last_loaded_name = *((idstring**)tmp);

		tmp = (char*)try_open_base_vr;
		tmp += 0x23;
		last_loaded_ext = *((idstring**)tmp);
	}
	else {
		tmp = (char*)try_open_base;
		tmp += 0x26;
		last_loaded_name = *((idstring**)tmp);

		tmp = (char*)try_open_base;
		tmp += 0x14;
		last_loaded_ext = *((idstring**)tmp);
	}
}

void* __cdecl tweaker::tweak_pd2_xml(char* text) {
	const char* new_text = transform_file(text);
	size_t length = strlen(new_text) + 1; // +1 for the null

	char* buffer = (char*)malloc(length);
	buffers.insert(buffer);

	strcpy_s(buffer, length, new_text);

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

void __cdecl tweaker::free_tweaked_pd2_xml(char* text) {
	if (buffers.erase(text)) {
		free(text);
	}
}

static void convert_block(const char* str, uint64_t* var, int* start_ptr, int len, int val) {
	int start = *start_ptr;
	*start_ptr -= len;

	uint64_t alt = *var;

	int i = 0;
	for (int possibleVal = start; possibleVal > start - len; possibleVal--) {
		i++;

		if (!(val >= possibleVal)) continue;

		int pow = 8 * (8 - i);

		*var += (uint64_t)(uint8_t)str[possibleVal - 1] << pow;
	}
}

idstring idstring_hash_cstr(const char* str, size_t len) {
	uint64_t a3 = 0;

	// Decompiled hashing method
	// Thanks, IDA
	uint64_t v3; // rcx@1
	uint64_t v4; // rax@1
	uint64_t v5; // r8@2
	uint64_t v6; // r10@2
	const char *v7; // r11@2 // NOTE: Set to const
	uint64_t v8; // rcx@3
	uint64_t v9; // rax@3
	uint64_t v10; // rbx@3
	uint64_t v11; // rdx@3
	uint64_t v12; // rcx@3
	uint64_t v13; // rax@3
	uint64_t v14; // rbx@3
	uint64_t v15; // rdx@3
	uint64_t v16; // rcx@3
	uint64_t v17; // rax@3
	uint64_t v18; // rbx@3
	uint64_t v20; // rsi@30
	uint64_t v21; // rdx@30
	uint64_t v22; // rcx@30
	uint64_t v23; // rax@30
	uint64_t v24; // rsi@30
	uint64_t v25; // rdx@30
	uint64_t v26; // rcx@30
	uint64_t v27; // rdi@30
	uint64_t v28; // rsi@30
	uint64_t v29; // rax@30

	v3 = a3;
	v4 = -7046029254386353133LL;

	// Compress our string dow nto 23 characters if necessary
	if (len < 24)
	{
		v5 = len;
	}
	else
	{
		v5 = len - 24 - 24 * ((uint64_t)(0x0AAAAAAAAAAAAAAABLL * (len - 24) >> 64) >> 4); // (uint128_t)
		v4 = -7046029254386353133LL;
		v6 = len;
		v7 = str;
		do
		{
			v8 = *((int64_t *)v7 + 1) + v3; // (_QWORD *)
			v9 = *((int64_t *)v7 + 2) + v4;
			v10 = (*(int64_t *)v7 + a3 - v8 - v9) ^ (v9 >> 43);
			v11 = (v8 - v9 - v10) ^ (v10 << 9);
			v12 = (v9 - v10 - v11) ^ (v11 >> 8);
			v13 = (v10 - v11 - v12) ^ (v12 >> 38);
			v14 = (v11 - v12 - v13) ^ (v13 << 23);
			v15 = (v12 - v13 - v14) ^ (v14 >> 5);
			v16 = (v13 - v14 - v15) ^ (v15 >> 35);
			v17 = (v14 - v15 - v16) ^ (v16 << 49);
			v18 = (v15 - v16 - v17) ^ (v17 >> 11);
			a3 = (v16 - v17 - v18) ^ (v18 >> 12);
			v3 = (v17 - v18 - a3) ^ (a3 << 18);
			v4 = (v18 - a3 - v3) ^ (v3 >> 22);
			v6 -= 24LL;
			v7 += 24;
		} while (v6 > 0x17);
		str += 24 * ((uint64_t)(0x0AAAAAAAAAAAAAAABLL * (len - 24) >> 64) >> 4) + 24; // (uint128_t)
	}

	uint64_t v19 = len + v4; // The top 7 char buffer

							 // Encode the string into v19, v3, a3 as needed.
	int starting = 23;
	convert_block(str, &v19, &starting, 7, v5);
	convert_block(str, &v3, &starting, 8, v5);
	convert_block(str, &a3, &starting, 8, v5);

	v20 = (a3 - v3 - v19) ^ (v19 >> 43);
	v21 = (v3 - v19 - v20) ^ (v20 << 9);
	v22 = (v19 - v20 - v21) ^ (v21 >> 8);
	v23 = (v20 - v21 - v22) ^ (v22 >> 38);
	v24 = (v21 - v22 - v23) ^ (v23 << 23);
	v25 = (v22 - v23 - v24) ^ (v24 >> 5);
	v26 = (v23 - v24 - v25) ^ (v25 >> 35);
	v27 = (v24 - v25 - v26) ^ (v26 << 49);
	v28 = (v25 - v26 - v27) ^ (v27 >> 11);
	v29 = (v26 - v27 - v28) ^ (v28 >> 12);
	return (v28 - v29 - ((v27 - v28 - v29) ^ (v29 << 18))) ^ (((v27 - v28 - v29) ^ (v29 << 18)) >> 22);
}

idstring tweaker::idstring_hash(string text) {
	return idstring_hash_cstr(text.c_str(), text.length());
}
