#include "platform.h"

#include "console/console.h"
#include "vr/vr.h"

#include <fstream>
#include <string>

using namespace std;
using namespace pd2hook;

static CConsole* console = NULL;

void blt::platform::PreInitPlatform() {
	// Set up logging first, so we can see messages from the signature search process
#ifdef INJECTABLE_BLT
	gbl_mConsole = new CConsole();
#else
	ifstream infile("mods/developer.txt");
	string debug_mode;
	if (infile.good()) {
		debug_mode = "post"; // default value
		infile >> debug_mode;
	}
	else {
		debug_mode = "disabled";
	}

	if (debug_mode != "disabled")
		console = new CConsole();
#endif
}

void blt::platform::InitPlatform() {
	VRManager::CheckAndLoad(); // TODO enable me
}

void blt::platform::ClosePlatform() {
	// Okay... let's not do that.
	// I don't want to keep this in memory, but it CRASHES THE SHIT OUT if you delete this after all is said and done.
	if (console) delete console;
}

void blt::platform::win32::OpenConsole() {
	if (!console) {
		console = new CConsole();
	}
}
