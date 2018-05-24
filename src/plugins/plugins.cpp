#include "plugins.h"
#include "util/util.h"

using namespace std;

namespace blt
{
	namespace plugins
	{

		list<Plugin*> plugins_list;

		PluginLoadResult LoadPlugin(string file)
		{
			// Major TODO before this is publicly released as stable:
			// Add some kind of security system to avoid loading untrusted plugins
			// (particularly those being used for the purpose of hiding a mod's
			//   source code - doing so is against the GPLv3 license SuperBLT is
			//   licensed under, but just in case)
			// Also require an exported function confirming GPL compliance, to make
			// plugin authors aware of the license - this could also provide a URL for
			// obtaining the plugin source code.

			for (const Plugin* plugin : plugins_list)
			{
				// TODO use some kind of ID or UUID embedded into the binary for identification, not filename
				if(file == plugin->GetFile())
					return plr_AlreadyLoaded;
			}

			PD2HOOK_LOG_LOG(string("Loading binary extension ") + file);

			try
			{
				Plugin *plugin = new Plugin(file);
				plugins_list.push_back(plugin);

				// Set up the already-running states
				RegisterPluginForActiveStates(plugin);
			}
			catch (const char* err)
			{
				throw string(err);
			}

			return plr_Success;
		}

		const list<Plugin*>& GetPlugins()
		{
			return plugins_list;
		}


	}
}
