#include "wrenxml.h"

#include <string>
#include <util/util.h>

#define WIN32_LEAN_AND_MEAN 1 // Uh, Microsoft?
#include <Windows.h>

using namespace std;
using namespace pd2hook::tweaker;
using namespace wrenxml;

const char *MODULE = "base/native";

#define WXML_ERR(err) { \
	PD2HOOK_LOG_ERROR(err) \
	MessageBox(0, "A WXML Error has occured - details are logged in the console", "WXML Error", MB_OK); \
	exit(1); \
}

#define THIS_WXML_NODE(vm) \
WXMLNode *wxml = *(WXMLNode**)wrenGetSlotForeign(vm, 0); \
if(wxml->root == NULL) { \
	MessageBox(0, "Cannot use closed Wren XML Instance", "Wren Error", MB_OK); \
	exit(1); \
} \
mxml_node_t *handle = wxml->handle

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

WXMLDocument::WXMLDocument(const char *text) {
	root_node = mxmlLoadString(NULL, text, MXML_TEXT_CALLBACK);
}

WXMLDocument::~WXMLDocument() {
	for (auto const &node : nodes) {
		node.second->root = NULL;
		node.second->handle = NULL;
	}

	mxmlDelete(root_node);
}

WXMLNode* WXMLDocument::GetNode(mxml_node_t *node) {
	if (nodes.count(node)) return nodes[node];

	return new WXMLNode(this, node);
}

WXMLNode::WXMLNode(WXMLDocument *root, mxml_node_t *handle) : root(root), handle(handle), usages(0) {
	root->nodes[handle] = this;
}

void WXMLNode::Release() {
	usages--;
	if (usages <= 0) {
		if (root != NULL) {
			root->nodes.erase(handle);

			if (root->nodes.empty()) {
				delete root;
			}
		}

		handle = NULL;
		root = NULL;
		delete this; // DANGER!
	}
}

static WXMLNode* attemptParseString(WrenVM* vm) {
	WXMLNode **node = (WXMLNode**)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WXMLNode*));

	const char* text = wrenGetSlotString(vm, 1);
	last_loaded_xml = text;

	WXMLDocument *doc = new WXMLDocument(text);

	*node = doc->GetRootNode();
	(*node)->Use();

	// Use the crash callback for anything else
	mxmlSetErrorCallback(&handle_mxml_error_crash);

	return *node;
}

static void allocateXML(WrenVM* vm) {
	mxmlSetErrorCallback(&handle_mxml_error_crash);
	WXMLNode *wxml = attemptParseString(vm);

	if (!wxml->handle) {
		WXML_ERR("Uncaught parse error!");
	}
}

static void finalizeXML(void* data) {
	WXMLNode *wxml = *(WXMLNode**)data;
	wxml->Release();
}

static void XMLtry_parse(WrenVM* vm) {
	mxmlSetErrorCallback(&handle_mxml_error_note);
	WXMLNode *wxml = attemptParseString(vm);

	if (mxml_last_error) {
		finalizeXML(wxml);
		wrenSetSlotNull(vm, 0);

		free(mxml_last_error);
		mxml_last_error = NULL;
	}
}

static void XMLdelete(WrenVM* vm) {
	// Make sure we don't crash if already freed, so don't use THIS_WXML
	WXMLNode *wxml = *(WXMLNode**)wrenGetSlotForeign(vm, 0);
	if (wxml->root != NULL) delete wxml->root;
}

static WXMLNode* XMLNode_create(WrenVM *vm, WXMLDocument *root, mxml_node_t *xnode, int slot) {
	if (xnode == NULL) WXML_ERR("Cannot create null XML Node");

	wrenGetVariable(vm, MODULE, "XML", slot);

	WXMLNode **node = (WXMLNode**)wrenSetSlotNewForeign(vm, slot, slot, sizeof(WXMLNode*));
	*node = root->GetNode(xnode);
	(*node)->Use();

	return *node;
}

static void XMLNode_type(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	wrenSetSlotDouble(vm, 0, mxmlGetType(handle));
}

#define XMLNODE_REQUIRE_TYPE(type, name) \
if (mxmlGetType(handle) != type) \
WXML_ERR("Can only perform ." #name " on " #type " nodes - ID:" + to_string(mxmlGetType(handle)));

static void XMLNode_text(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_TEXT, name);

	wrenSetSlotString(vm, 0, mxmlGetText(handle, NULL));
}

static void XMLNode_text_set(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_TEXT, name);

	mxmlSetText(handle, NULL, wrenGetSlotString(vm, 1));
}

static void XMLNode_string(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, string);

	mxmlSetWrapMargin(0);
	char* str = mxmlToAllocStringSafe(handle, MXML_NO_CALLBACK);
	wrenSetSlotString(vm, 0, str);
	free(str);
}

static void XMLNode_name(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	wrenSetSlotString(vm, 0, mxmlGetElement(handle));
}

static void XMLNode_name_set(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	mxmlSetElement(handle, wrenGetSlotString(vm, 1));
}

static void XMLNode_attribute(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	const char *value = mxmlElementGetAttr(handle, wrenGetSlotString(vm, 1));
	if (value)
		wrenSetSlotString(vm, 0, value);
	else
		wrenSetSlotNull(vm, 0);
}

static void XMLNode_attribute_set(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	const char *name = wrenGetSlotString(vm, 1);

	if (wrenGetSlotType(vm, 2) == WREN_TYPE_NULL) {
		mxmlElementDeleteAttr(handle, name);
	}
	else {
		const char *value = wrenGetSlotString(vm, 2);
		mxmlElementSetAttr(handle, name, value);
	}
}

static void XMLNode_attribute_names(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	wrenEnsureSlots(vm, 2);

	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i < mxmlElementGetAttrCount(handle); i++) {
		const char *name;
		mxmlElementGetAttrByIndex(handle, i, &name);
		wrenSetSlotString(vm, 1, name);
		wrenInsertInList(vm, 0, -1, 1);
	}
}

static void XMLNode_create_element(WrenVM* vm) {
	THIS_WXML_NODE(vm);

	XMLNODE_REQUIRE_TYPE(MXML_ELEMENT, name);

	const char *name = wrenGetSlotString(vm, 1);
	mxml_node_t *node = mxmlNewElement(handle, name);
	XMLNode_create(vm, wxml->root, node, 0);
}

#define XMLNODE_FUNC(getter, name) \
static void XMLNode_ ## name(WrenVM* vm) { \
	THIS_WXML_NODE(vm); \
	mxml_node_t *node = mxmlGet ## getter(handle); \
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

#define XMLNODE_CHECK_FUNC(getter, name) \
		if (!is_static && signature == #name) { \
			return XMLNode_ ## name; \
		}

#define XMLNODE_BI_FUNC(name) \
		XMLNODE_CHECK_FUNC(, name) \
		if (!is_static && signature == #name "=(_)") { \
			return XMLNode_ ## name ## _set; \
		}

		XMLNODE_CHECK_FUNC(, type);
		XMLNODE_BI_FUNC(text);
		XMLNODE_CHECK_FUNC(, string);
		XMLNODE_BI_FUNC(name);
		XMLNODE_CHECK_FUNC(, attribute_names);

		if (!is_static && signature == "[_]") {
			return XMLNode_attribute;
		}
		if (!is_static && signature == "[_]=(_)") {
			return XMLNode_attribute_set;
		}

		if (!is_static && signature == "create_element(_)") {
			return XMLNode_create_element;
		}

		if (!is_static && signature == "delete()") {
			return XMLdelete;
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
	}
	return { NULL,NULL };
}
