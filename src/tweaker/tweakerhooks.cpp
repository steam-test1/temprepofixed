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
