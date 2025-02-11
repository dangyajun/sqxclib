/*
 *   Copyright (C) 2020-2023 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 * sqxclib is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef SQ_ENTRY_H
#define SQ_ENTRY_H

#include <stddef.h>    // NULL, offsetof()
#include <stdbool.h>   // bool, true, false

#include <SqDefine.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structure, macro, enumeration.

typedef struct SqType           SqType;
typedef struct SqEntry          SqEntry;
typedef struct SqReentry        SqReentry;


/* --- SqEntry::bit_field --- */
#define SQB_DYNAMIC        (1 << 0)   // C: entry can be changed and freed
#define SQB_POINTER        (1 << 1)   // C: entry's instance is pointer
#define SQB_RESERVE_0      (1 << 2)   // reserve
#define SQB_HIDDEN         (1 << 3)   // JSON: hidden entry
#define SQB_HIDDEN_NULL    (1 << 4)   // JSON: hidden entry if value is NULL
#define SQB_RESERVE_1      (1 << 5)   // reserve

// SQL common bit_field.
#define SQB_CHANGED        (1 << 6)   // SQL: column (or table) has been changed.
// SQL common bit_field (for internal use only. use it when SQLite recreate)
#define SQB_RENAMED        (1 << 7)   // SQL: column (or table) has been renamed.
// derived class/struct need this
#define SQB_RESERVE_END    (1 << 8)

/* --- SqReentry::bit_field (Column Modifiers) --- */
#define SQB_REENTRY_RENAMED        SQB_RENAMED

/* --- SqColumn::bit_field (Column Modifiers) ---
   SqColumn::bit_field must large then SQB_RESERVE_END
 */
#define SQB_PRIMARY        (1 << 9)   // SQL: PRIMARY KEY
#define SQB_FOREIGN        (1 << 10)  // SQL: FOREIGN KEY
#define SQB_UNIQUE         (1 << 11)  // SQL: UNIQUE
#define SQB_INCREMENT      (1 << 12)  // SQL: AUTOINCREMENT == SQB_AUTOINCREMENT
#define SQB_AUTOINCREMENT  (1 << 12)  // SQL: AUTOINCREMENT == SQB_INCREMENT
#define SQB_NULLABLE       (1 << 13)  // SQL: remove "NOT NULL"

// SqColumn::type == SQ_TYPE_TIME (use CURRENT_TIMESTAMP)
#define SQB_CURRENT              (1 << 14)    // SQL: DEFAULT CURRENT_TIMESTAMP
#define SQB_CURRENT_ON_UPDATE    (1 << 15)    // SQL: CREATE TRIGGER AFTER UPDATE
#define SQB_CURRENT_ALL          (SQB_CURRENT | SQB_CURRENT_ON_UPDATE)

/* SQ_N_PTRS() calculate number of pointer in array. for example:
	SqEntry *FooEntries[] = {...};
	int  n_entry = SQ_N_PTRS(FooEntries);
 */
#define SQ_N_PTRS(PointerArray) ( sizeof(PointerArray)/sizeof(PointerArray[0]) )

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

/* --- macro functions --- for maintaining C/C++ inline functions easily */
#define SQ_ENTRY_SET_NAME(entry, name_str)    \
		sq_entry_set_str_addr((SqEntry*)entry, (char**) &((SqEntry*)entry)->name, name_str)

/* SqEntry C functions */

// create/destroy entry for JSON or XML
SqEntry *sq_entry_new(const SqType *type_info);
void     sq_entry_free(SqEntry *entry);

// sq_entry_init() and sq_entry_final() are called by derived struct
void  sq_entry_init(SqEntry *entry, const SqType *type_info);
void  sq_entry_final(SqEntry *entry);

// If 'compareFunc' is NULL, it will use sq_entry_cmp_str__name() by default.
SqEntry *sq_entry_find(SqEntry *entry, const void *key, SqCompareFunc compareFunc);

/* SqCompareFunc for sorting and finding SqEntry */

// This function is used by find(). Its actual parameter type:
//int sq_entry_cmp_str__name(const char *str, SqEntry   **entryAddr);
int   sq_entry_cmp_str__name(const void *str, const void *entryAddr);

// This function is used by sort(). Its actual parameter type:
//int sq_entry_cmp_name(SqEntry   **entryAddr1, SqEntry   **entryAddr2);
int   sq_entry_cmp_name(const void *entryAddr1, const void *entryAddr2);

/* SqCompareFunc for sorting and finding SqEntry by SqType::name */

// This function is used by find(). Its actual parameter type:
//int sq_entry_cmp_str__type_name(const char *str, SqEntry   **entryAddr);
int   sq_entry_cmp_str__type_name(const void *str, const void *entryAddr);

// This function is used by sort(). Its actual parameter type:
//int sq_entry_cmp_type_name(SqEntry   **entryAddr1, SqEntry   **entryAddr2);
int   sq_entry_cmp_type_name(const void *entryAddr1, const void *entryAddr2);

// for internal use only
void  sq_entry_set_str_addr(SqEntry *entry, char **str_addr, const char *str_src);


/*	SqReentries: unsorted pointer array of SqReentry.
	             element can be NULL in array.
	             It used by migration, change list...etc
 */

// It remove all NULL pointer in array
// n_old_elements: number of old elements before removing NULL
// return:         number of old elements after  removing NULL
int   sq_reentries_remove_null(void *reentry_ptr_array, int n_old_elements);

#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------
// C++ declarations: declare C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

struct TypeMethod;    // defined in SqType.h
struct Entry;

};  // namespace Sq

#endif // __cplusplus

// ----------------------------------------------------------------------------
// C/C++ common definitions: define structure

/*	SqEntry:  define field of structure type.
	SqColumn: define SQL column (field) in SQL table (structure/class).

	SqEntry
	|
	`--- SqReentry
	     |
	     +--- SqTable
	     |
	     `--- SqColumn

	Because 'const' is used to define string 'char*' and type 'SqType*',
	C++ user can initialize static structure easily.

	SqEntry must have no base struct because I need use aggregate initialization with it.
 */

#define SQ_ENTRY_MEMBERS        \
	const SqType *type;         \
	const char   *name;         \
	size_t        offset;       \
	unsigned int  bit_field

struct SqEntry
{
	SQ_ENTRY_MEMBERS;
/*	// ------ SqEntry members ------
	const SqType *type;        // type information of entry
	const char   *name;
	size_t        offset;
	unsigned int  bit_field;
 */

#ifdef __cplusplus
	/* Note: If you add, remove, or change methods here, do the same things in Sq::EntryMethod. */

	void  init(const SqType *type_info) {
		sq_entry_init((SqEntry*)this, type_info);
	}
	void  init(const Sq::TypeMethod *type_info) {
		sq_entry_init((SqEntry*)this, (const SqType*)type_info);
	}
	void  final() {
		sq_entry_final((SqEntry*)this);
	}

	Sq::Entry *find(const void *key, SqCompareFunc compareFunc = NULL) {
		return (Sq::Entry*)sq_entry_find((SqEntry*)this, key, compareFunc);
	}

	void  setName(const char *name) {
		SQ_ENTRY_SET_NAME(this, name);
	}
#endif  // __cplusplus
};

/*	SqReentry is used to change previously-defined entries.
	It derives from SqEntry and adds a field 'old_name' for recording name changes (rename or remove).

	Because 'const' is used to define string 'char*' and type 'SqType*',
	C++ user can initialize static structure easily.
*/

#define SQ_REENTRY_MEMBERS      \
	const SqType *type;         \
	const char   *name;         \
	size_t        offset;       \
	unsigned int  bit_field;    \
	const char   *old_name

struct SqReentry
{
	SQ_REENTRY_MEMBERS;
/*	// ------ SqReentry members ------
	const SqType *type;        // type information of entry
	const char   *name;
	size_t        offset;
	unsigned int  bit_field;
	const char   *old_name;    // rename or drop
 */
	// if name is NULL, it will drop old_name
	// if name is NOT NULL, it will rename from old_name to name
};

// ----------------------------------------------------------------------------
// C/C++ common definitions: define global inline function

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || defined(__cplusplus)
// define inline functions here if compiler supports inline function.

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_entry_set_name(SqEntry *entry, const char *name) {
	SQ_ENTRY_SET_NAME(entry, name);
}

#else   // __STDC_VERSION__ || __cplusplus
// declare functions here if compiler does NOT support inline function.

void  sq_entry_set_name(SqEntry *entry, const char *name);

#endif  // __STDC_VERSION__ || __cplusplus

// ----------------------------------------------------------------------------
// C++ definitions: define C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

/*	EntryMethod is used by SqEntry's children.

	It's derived struct/class must be C++11 standard-layout and has SqEntry members.

	Note: If you add, remove, or change methods here, do the same things in SqEntry.
 */
struct EntryMethod
{
	void  init(const SqType *type_info) {
		sq_entry_init((SqEntry*)this, type_info);
	}
	void  init(const Sq::TypeMethod *type_info) {
		sq_entry_init((SqEntry*)this, (const SqType*)type_info);
	}
	void  final() {
		sq_entry_final((SqEntry*)this);
	}

	Sq::Entry *find(const void *key, SqCompareFunc compareFunc = NULL) {
		return (Sq::Entry*)sq_entry_find((SqEntry*)this, key, compareFunc);
	}

	void  setName(const char *name) {
		SQ_ENTRY_SET_NAME((SqEntry*)this, name);
	}
};

/* All derived struct/class must be C++11 standard-layout. */

struct Entry : SqEntry {
	Entry() {}
	Entry(const SqType *type_info) {
		sq_entry_init(this, type_info);
	}
	Entry(const Sq::TypeMethod *type_info) {
		sq_entry_init(this, (const SqType*)type_info);
	}
	~Entry() {
		sq_entry_final(this);
	}
};

typedef struct SqReentry        Reentry;

};  // namespace Sq

#endif  // __cplusplus

#endif  // SQ_ENTRY_H
