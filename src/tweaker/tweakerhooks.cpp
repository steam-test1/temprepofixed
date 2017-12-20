#include <stdio.h>
#include "util/util.h"
#include "signatures/sigdef.h"
#include "xmltweaker_internal.h"
#include "signatures/sigdef.h"

using namespace pd2hook;

void __declspec(naked) tweaker::node_from_xml_new() {
	__asm
	{
		push ecx
		push edx

		// Run the intercept method
		call tweak_pd2_xml

		pop edx
		pop ecx

		// Move across our string
		mov edx, eax

		// Save our value for later, so we can free it
		push eax

		// Bring the old top-of-the-stack back up
		mov eax, [esp + 4]
		push eax

		call node_from_xml

		add esp, 4 // Remove our temporary value from the stack

		// Currently, the stack is: [ arg3, txt
		// Call the free function, which uses the txt arg
		call free_tweaked_pd2_xml

		// Clear off the txt argument, and the stack is back to how we left it
		add esp, 4

		retn
	}
}

static void __cdecl note_loaded_file_wrapper(unsigned long long ext, unsigned long long name) {
	// printf("*VAL: %016llx, %016llx\n", name, ext);
	tweaker::note_loaded_file(name, ext);
}

int __declspec(naked) tweaker::try_open_base_hook()
{
	__asm
	{
		// Save ecx and edx - if they are modified, the game crashes
		push ecx
		push edx

		// Push 32 bytes of data
		// int=4,long=8,longlong=16 - we use 2x longlong, thus 32

		push[esp + 36] // 36 for the args + 8 for ecx/edx - 8 because the stack pointer hasn't advanced yet
		push[esp + 36]
		push[esp + 36]
		push[esp + 36]

		call note_loaded_file_wrapper

		add esp, 16 // remove the four items we pushed onto the stack

		// Restore ecx/edx
		pop edx
		pop ecx

		jmp try_open_base
	}
}
