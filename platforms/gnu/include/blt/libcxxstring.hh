#pragma once
#include <string>

/*
 * Basic reimplementation of LibCXX's string class, for interop with PAYDAY 2
 *
 * @author ZNix
 */

namespace blt
{
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
		char *str;
		size_t size;
		size_t capacity;

		void set_contents(const char *value, size_t length);
	};

	static_assert(sizeof(libcxxstring) == 24);
};

