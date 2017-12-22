#pragma once

#include <unordered_set>

#include "mxml.h"

extern "C" {
#include "wren.h"
}

namespace pd2hook {
	namespace tweaker {
		namespace wrenxml {
			struct WXMLNode;

			// Note that having nodes open does not prevent the main object from being GC'd
			struct WXMLNode {
				WXMLNode *root;
				mxml_node_t *handle;
			};

			WrenForeignMethodFn bind_wxml_method(
				WrenVM* vm,
				const char* module,
				const char* className,
				bool isStatic,
				const char* signature);

			WrenForeignClassMethods get_XML_class_def(
				WrenVM* vm,
				const char* module,
				const char* class_name);
		};
	};
};
