#pragma once

#include <dsl/idstring.hh>
#include <dsl/FileSystem.hh>
#include <blt/libcxxstring.hh>

namespace dsl
{
   class DB
   {
   public:
      char null1[184];
      dsl::FileSystemStack *stack;
   };
};

/* vim: set ts=3 softtabstop=0 sw=3 expandtab: */
