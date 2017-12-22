#include "wrenxml.h"

#include <string>
#include <util/util.h>

#define WIN32_LEAN_AND_MEAN 1 // Uh, Microsoft?
#include <Windows.h>

using namespace std;
using namespace pd2hook::tweaker;
using namespace wrenxml;

const char *MODULE = "base/native";

static void finalizeXMLNode(void* data);

#define WXML_ERR(err) { \
	PD2HOOK_LOG_ERROR(err) \
	MessageBox(0, "A WXML Error has occured - details are logged in the console", "WXML Error", MB_OK); \
	exit(1); \
}

#define THIS_WXML(vm) \
WXML *wxml = (WXML*)wrenGetSlotForeign(vm, 0); \
if(!wxml->open) { \
	MessageBox(0, "Cannot use closed Wren XML Instance", "Wren Error", MB_OK); \
	exit(1); \
}

#define THIS_WXML_NODE(vm) \
WXMLNode *wxml = (WXMLNode*)wrenGetSlotForeign(vm, 0); \
if(wxml->root == NULL) { \
	MessageBox(0, "Cannot use closed Wren XML Instance", "Wren Error", MB_OK); \
	exit(1); \
}

static char *					/* O - Allocated string */
mxmlToAllocStringSafe(
	mxml_node_t    *node,		/* I - Node to write */
	mxml_save_cb_t cb)			/* I - Whitespace callback or @code MXML_NO_CALLBACK@ */
{
	int	bytes;				/* Required bytes */
	char	buffer[8192];			/* Temporary buffer */
	char	*s;				/* Allocated string */


							/*
							* Write the node to the temporary buffer...
							*/

	bytes = mxmlSaveString(node, buffer, sizeof(buffer), cb);

	if (bytes <= 0)
		return _strdup(""); // Empty string case

	if (bytes < (int)(sizeof(buffer) - 1))
	{
		/*
		* Node fit inside the buffer, so just duplicate that string and
		* return...
		*/

		return _strdup(buffer);
	}

	/*
	* Allocate a buffer of the required size and save the node to the
	* new buffer...
	*/

	if ((s = (char*)malloc(bytes + 1)) == NULL) {
		MessageBox(0, "[WREN/WXML] XML.string: Cannot allocate enough space for XML string", "SuperBLT OutOfMemory", MB_OK);
		exit(1);
	}

	mxmlSaveString(node, s, bytes + 1, cb);

	/*
	* Return the allocated string...
	*/

	return (s);
}

static const char *last_loaded_xml = NULL;
static char *mxml_last_error = NULL;

static void handle_mxml_error_crash(const char* error) {
	PD2HOOK_LOG_ERROR("Could not load XML: Error and original file below");
	PD2HOOK_LOG_ERROR(error);
	PD2HOOK_LOG_ERROR("Input XML:");
	PD2HOOK_LOG_ERROR(last_loaded_xml);
	MessageBox(0, "[WREN/WXML] XML.new: parse error - see log for details", "XML Parse Error", MB_OK);
	exit(1);
}

static void handle_mxml_error_note(const char* error) {
	mxml_last_error = _strdup(error);
}

static WXML* attemptAllocateXML(WrenVM* vm) {
	WXML *wxml = (WXML*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WXML));

	const char* text = wrenGetSlotString(vm, 1);
	last_loaded_xml = text;

	wxml->nodes = new unordered_set<WXMLNode*>();

	wxml->root_node = mxmlLoadString(NULL, text, MXML_TEXT_CALLBACK);
	wxml->open = wxml->root_node != NULL;

	// Use the crash callback for anything else
	mxmlSetErrorCallback(&handle_mxml_error_crash);

	return wxml;
}

static void allocateXML(WrenVM* vm) {
	mxmlSetErrorCallback(&handle_mxml_error_crash);
	WXML *wxml = attemptAllocateXML(vm);

	if (!wxml->open) {
		WXML_ERR("Uncaught parse error!");
	}
}

static void finalizeXML(void* data) {
	WXML *wxml = (WXML*)data;
	if (!wxml->open) return;
	wxml->open = false;

	// free resources
	mxmlDelete(wxml->root_node);

	unordered_set<WXMLNode*> *nodes = wxml->nodes;
	wxml->nodes = NULL;

	for (WXMLNode *node : *nodes) {
		finalizeXMLNode(node);
	}

	delete nodes;
}

static void XMLtry_parse(WrenVM* vm) {
	mxmlSetErrorCallback(&handle_mxml_error_note);
	WXML *wxml = attemptAllocateXML(vm);

	if (mxml_last_error) {
		finalizeXML(wxml);
		wrenSetSlotNull(vm, 0);

		free(mxml_last_error);
		mxml_last_error = NULL;
	}
}

static void XMLfree(WrenVM* vm) {
	// Make sure we don't crash if already freed, so don't use THIS_WXML
	WXML *wxml = (WXML*)wrenGetSlotForeign(vm, 0);
	finalizeXML(wxml);
}

static WXMLNode* XMLNode_create(WrenVM *vm, WXML *root, mxml_node_t *xnode, int slot) {
	if (xnode == NULL) WXML_ERR("Cannot create null XMLNode");

	wrenGetVariable(vm, MODULE, "XMLNode", slot);

	WXMLNode *node = (WXMLNode*)wrenSetSlotNewForeign(vm, slot, slot, sizeof(WXMLNode));
	node->root = root;
	node->handle = xnode;
	root->nodes->insert(node);

	return node;
}

static void XMLroot_node(WrenVM* vm) {
	THIS_WXML(vm);
	XMLNode_create(vm, wxml, wxml->root_node, 0);
}

static void finalizeXMLNode(void* data) {
	WXMLNode *node = (WXMLNode*)data;
	if (node->root == NULL) return;
	if (node->root->nodes) node->root->nodes->erase(node);
	node->root = NULL;
}

static void XMLNode_type(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	wrenSetSlotDouble(vm, 0, mxmlGetType(wxml->handle));
}

#define XMLNODE_REQUIRE_TYPE(type, name) \
if (mxmlGetType(wxml->handle) != type) \
WXML_ERR("Can only perform ." #name " on " #type " nodes - ID:" + to_string(mxmlGetType(wxml->handle)));

static void XMLNode_text(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_TEXT, name);

	wrenSetSlotString(vm, 0, mxmlGetText(wxml->handle, NULL));
}

static void XMLNode_string(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, string);

	mxmlSetWrapMargin(0);
	char* str = mxmlToAllocStringSafe(wxml->handle, MXML_NO_CALLBACK);
	wrenSetSlotString(vm, 0, str);
	free(str);
}

static void XMLNode_name(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	wrenSetSlotString(vm, 0, mxmlGetElement(wxml->handle));
}

static void XMLNode_attribute(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	const char *value = mxmlElementGetAttr(wxml->handle, wrenGetSlotString(vm, 1));
	if (value)
		wrenSetSlotString(vm, 0, value);
	else
		wrenSetSlotNull(vm, 0);
}

static void XMLNode_attribute_names(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	wrenEnsureSlots(vm, 2);

	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i < mxmlElementGetAttrCount(wxml->handle); i++) {
		const char *name;
		mxmlElementGetAttrByIndex(wxml->handle, i, &name);
		wrenSetSlotString(vm, 1, name);
		wrenInsertInList(vm, 0, -1, 1);
	}
}

#define XMLNODE_FUNC(getter, name) \
static void XMLNode_ ## name(WrenVM* vm) { \
	THIS_WXML_NODE(vm); \
	mxml_node_t *node = mxmlGet ## getter(wxml->handle); \
	if(node) { \
		XMLNode_create(vm, wxml->root, node, 0); \
	} \
	else { \
		wrenSetSlotNull(vm, 0); \
	} \
}

#define XMLNODE_FUNC_SET(func) \
func(NextSibling, next) \
func(PrevSibling, prev) \
func(Parent, parent) \
func(FirstChild, child) \
func(LastChild, last_child)

XMLNODE_FUNC_SET(XMLNODE_FUNC)

WrenForeignMethodFn wrenxml::bind_wxml_method(
	WrenVM* vm,
	const char* module,
	const char* class_name_s,
	bool is_static,
	const char* signature_c)
{
	if (strcmp(module, MODULE) != 0) return NULL;

	string class_name = class_name_s;
	string signature = signature_c;

	if (class_name == "XML") {
		if (is_static && signature == "try_parse(_)") {
			return XMLtry_parse;
		}
		else if (!is_static && signature == "free()") {
			return XMLfree;
		}
		else if (!is_static && signature == "root_node") {
			return XMLroot_node;
		}
	}
	else if (class_name == "XMLNode") {
#define XMLNODE_CHECK_FUNC(getter, name) \
		if (!is_static && signature == #name) { \
			return XMLNode_ ## name; \
		}

		XMLNODE_CHECK_FUNC(z, type);
		XMLNODE_CHECK_FUNC(z, text);
		XMLNODE_CHECK_FUNC(z, string);
		XMLNODE_CHECK_FUNC(z, name);
		XMLNODE_CHECK_FUNC(z, attribute_names);

		if (!is_static && signature == "[_]") {
			return XMLNode_attribute;
		}

		XMLNODE_FUNC_SET(XMLNODE_CHECK_FUNC);
	}

	return NULL;
}

WrenForeignClassMethods wrenxml::get_XML_class_def(WrenVM* vm, const char* module, const char* class_name) {
	if (string(module) == "base/native") {
		if (string(class_name) == "XML") {
			WrenForeignClassMethods def;
			def.allocate = allocateXML;
			def.finalize = finalizeXML;
			return def;
		}
		else if (string(class_name) == "XMLBase") {
			WrenForeignClassMethods def;
			def.allocate = NULL;
			def.finalize = finalizeXMLNode;
			return def;
		}
	}
	return { NULL,NULL };
}
