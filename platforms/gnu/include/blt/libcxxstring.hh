#pragma once
#include <string>

/*
 * Basic reimplementation of LibCXX's string class, for interop with PAYDAY 2
 *
 * @author ZNix
 */

namespace blt
{
	//typedef std::string libcxxstring;
	//*
	// Note: PAYDAY 2 was compiled with _LIBCPP_BIG_ENDIAN set, and _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT not set
	class libcxxstring
	{
	public:
		// Main constructors
		libcxxstring(const char *value);
		libcxxstring(const char *value, size_t length);
		libcxxstring(std::string value);

		// Destructor
		~libcxxstring();

		// Copy constructor and assignment operator
		libcxxstring(libcxxstring &other);
		libcxxstring& operator=(const libcxxstring &other);

		// Conversion to C and stdlib strings
		operator std::string() const;
		const char* c_str() const;

	private:
		size_t capacity_and_flags;
		size_t size;
		char *str;

		void set_contents(const char *value, size_t length);
	};

	static_assert(sizeof(libcxxstring) == 24);
};

