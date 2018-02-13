//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011,
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Symbol Table class optimised for lookup speed.
//
// =============================================================================

#ifndef INC_UTIL_SYMBOLTABLE_H_
#define INC_UTIL_SYMBOLTABLE_H_

#include "api/types.h"

#include <string>

#include "ioc/ContextItemInterface.h"

class IELFISymbolTable;

namespace archsim
{
	namespace util
	{

// Forward declare SymEntry struct
//
		struct SymEntry;

		class SymbolTable : public archsim::ioc::ContextItemInterface
		{
		public:
			static const uint32_t kSymbolTableMaxName = 256;

			explicit SymbolTable(const char* name);
			~SymbolTable();

			const uint8_t* get_name() const
			{
				return name_;
			}
			const Type get_type() const
			{
				return archsim::ioc::ContextItemInterface::kTSymbolTable;
			}

			void create(const IELFISymbolTable* tab);
			void destroy();

			// -----------------------------------------------------------------------
			// Query methods
			//
			bool get_symbol(uint32_t addr, std::string& name);

		private:
			// pre-sorted symbol table for efficient and fast lookup
			SymEntry* tab_;
			int tab_size_;

			// read only target symbol table
			const IELFISymbolTable* elf_tab_;

			uint8_t name_[kSymbolTableMaxName];
		};
	}
}  // archsim::util

#endif  // INC_UTIL_SYMBOLTABLE_H_
