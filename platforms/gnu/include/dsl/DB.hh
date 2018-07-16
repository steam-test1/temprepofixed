#pragma once

#include <dsl/idstring.hh>
#include <dsl/FileSystem.hh>
#include <blt/libcxxstring.hh>

namespace dsl
{
	class DB
	{
	public:
		void *ptr1;
		void *allocator_thingy;
		void *ptr2;
		void *another_allocator_thingy;
		blt::libcxxstring some_str_1;
		blt::libcxxstring some_str_2;
		void *ptr4;
		void *some_map;
		dsl::FileSystemStack *stack;
		void *ptr5;
		void *ptr6;
	};
};

/* vim: set ts=3 softtabstop=0 sw=3 expandtab: */
