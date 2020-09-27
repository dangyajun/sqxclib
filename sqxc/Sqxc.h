/*
 *   Copyright (C) 2020 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 * sqxc is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

/* ----------------------------------------------------------------------------
	Sqxc - Entry and value convert C to/from X  (X = SQLite, JSON...etc)
	       xc is an abbreviation. (sqxc namespace "Sq" + "xc" = Sqxc)

	SqxcXml    - C to/from XML     - SqxcXml.c     (TODO or not)
	SqxcJson   - C to/from JSON    - SqxcJson.c
	SqxcSqlite - C to/from SQLite  - SqxcSqlite.c
	SqxcValue  - X to/from Memory  - SqxcValue.c

	-----------> input ----------->  send()   -----------> output ---------->

	             +-> SqxcJson --+               +-> SqxcJson --+
	             |              |               |              |
	SqxcSqlite --+--------------+-> SqxcValue --+--------------+-> SqxcSqlite
	             |              |               |              |
	             +--> SqxcXml --+               +--> SqxcXml --+

    (src) prev <---------> (dest) next     (src) prev <---------> (dest) next

 */

#ifndef SQXC_H
#define SQXC_H

#include <stddef.h>
#include <stdint.h>     // int64_t
#include <stdbool.h>    // bool

#include <SqDefine.h>
#include <SqBuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sqxc             Sqxc;
typedef struct SqxcInfo         SqxcInfo;
typedef struct SqxcNested       SqxcNested;

// declare in <SqEntry.h>
typedef struct SqEntry          SqEntry;

// define C type convert to/from X
typedef enum {
	SQXC_TYPE_NONE     =  0,
	SQXC_TYPE_BOOL     = (1 << 0),    // 0x001
	SQXC_TYPE_INT      = (1 << 1),    // 0x002
	SQXC_TYPE_UINT     = (1 << 2),    // 0x004
	SQXC_TYPE_INT64    = (1 << 3),    // 0x008
	SQXC_TYPE_UINT64   = (1 << 4),    // 0x010
	SQXC_TYPE_DOUBLE   = (1 << 5),    // 0x020

	SQXC_TYPE_ARITHMETIC = 0x3F,      // 0x03F

	SQXC_TYPE_STRING   = (1 << 6),    // 0x040
//	SQXC_TYPE_POINTER  = (1 << 7),    // 0x080    // reserve
	SQXC_TYPE_OBJECT   = (1 << 8),    // 0x100
	SQXC_TYPE_ARRAY    = (1 << 9),    // 0x200    // other storage structure
	SQXC_TYPE_ALL      =  0x3FF,
//	SQXC_TYPE_AUTO     = (1 << 14),
	SQXC_TYPE_END      = (1 << 15),   // End of SQXC_TYPE_OBJECT or SQXC_TYPE_ARRAY
	SQXC_TYPE_OBJECT_END = SQXC_TYPE_END | SQXC_TYPE_OBJECT,
	SQXC_TYPE_ARRAY_END  = SQXC_TYPE_END | SQXC_TYPE_ARRAY,
} SqxcType;

typedef enum {
	// common
	SQXC_CTRL_READY,         // reset status
	SQXC_CTRL_FINISH,        // flush data

	// SqxcSql
	SQXC_SQL_USE_SELECT,     // SqTable* data
	SQXC_SQL_USE_INSERT,     // SqTable* data
	SQXC_SQL_USE_UPDATE,     // SqTable* data
	SQXC_SQL_USE_WHERE,      // char*    condition
} SqxcCtrlId;

typedef int   (*SqxcCtrlFunc)(Sqxc* xc, int id, void* data);
typedef int   (*SqxcSendFunc)(Sqxc* dest, Sqxc* src);

/* ----------------------------------------------------------------------------
	SqxcInfo

	Any Sqxc must has 2 SqxcInfo for output/input, for example:

	SqxcInfo MyXcInfo[2];

	// MyXcInfo[0] for Output
	// MyXcInfo[1] for Input
 */

struct SqxcInfo
{
	size_t       size;
	SqInitFunc   init;
	SqFinalFunc  final;
};

/* ----------------------------------------------------------------------------
	SqxcNested - pointer to current nested object/array
 */

struct SqxcNested
{
	SqxcNested*  outer;
	SqxcNested*  inner;

	void*  data;    // void*    instance;
	void*  data2;   // SqType*  type;
};

/* ----------------------------------------------------------------------------
	Sqxc - Interface for parse or write

	Entry and value convert C to/from X  (X = SQLite, JSON, or XML...etc)
 */

Sqxc*   sqxc_new(const SqxcInfo* xcinfo, int io);
void    sqxc_free(Sqxc* xc);

// create Sqxc chain for input/output
// parameter order from src to dest, return src
// sqxc_new_input(SQXC_INFO_SQLITE, SQXC_INFO_JSON, SQXC_INFO_VALUE, NULL);
// sqxc_new_output(SQXC_INFO_VALUE, SQXC_INFO_JSON, SQXC_INFO_SQLITE, NULL);

Sqxc*   sqxc_new_chain(int io_, ...);

// create Sqxc chain for input
// Sqxc* sqxc_new_input(SqxcInfo* src, ...);
#define sqxc_new_input(...)     sqxc_new_chain(1, __VA_ARGS__, NULL)

// create Sqxc chain for output
// Sqxc* sqxc_new_output(SqxcInfo* src, ...);
#define sqxc_new_output(...)    sqxc_new_chain(0, __VA_ARGS__, NULL)

// free Sqxc chain
void    sqxc_free_chain(Sqxc* xc);

Sqxc*   sqxc_get(Sqxc* xc, const SqxcInfo* info, int nth);
Sqxc*   sqxc_insert(Sqxc* xc, int position, Sqxc* xcdata);

#define sqxc_get_buffer(xc)    (SqBuffer*)(&((Sqxc*)(xc))->buf)

// sqxc_broadcast() broadcast Sqxc chain.
// It will call Sqxc.ctrl() from src to dest in chain of Sqxc.
int     sqxc_broadcast(Sqxc* xc, int id, void* data);

// sqxc_ready() broadcast Sqxc chain to ready
// void sqxc_ready(Sqxc* xc, void* data);
#define sqxc_ready(xc, data)    \
		sqxc_broadcast((Sqxc*)xc, SQXC_CTRL_READY, data);

// sqxc_finish() broadcast Sqxc chain to flush data
// void sqxc_finish(Sqxc* xc, void* error);
#define sqxc_finish(xc, data)   \
		sqxc_broadcast((Sqxc*)xc, SQXC_CTRL_FINISH, data);

/* sqxc_send(src) send data from src to src->dest
   It try to match type in Sqxc chain.
 */
int     sqxc_send(Sqxc* src);

/* --------------------------------------------------------
   These macro send data from src to src->dest

	[ {"id": 100, "name": "Bob"} ]

	sqxc_send_array_beg(sqxc_src, NULL);        //  [

	sqxc_send_object_beg(sqxc_src, NULL);       //  {
	sqxc_send_int(sqxc_src, "id", 100);         //  "id": 100
	sqxc_send_string(sqxc_src, "name", "Bob");  //  "name": "Bob"
	sqxc_send_object_end(sqxc_src, NULL);       //  }

	sqxc_send_array_end(sqxc_src, NULL);        //  ]
 */

// void sqxc_send_bool(Sqxc* src, const char* entry_name, bool value);
#define sqxc_send_bool(sqxc_src, entry_name, value_)     \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_BOOL;  \
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.boolean = value_; \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_int(Sqxc* src, const char* entry_name, int value);
#define sqxc_send_int(sqxc_src, entry_name, value_)      \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_INT;   \
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.integer = value_; \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_int64(Sqxc* src, const char* entry_name, int64_t value);
#define sqxc_send_int64(sqxc_src, entry_name, value_)    \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_INT64; \
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.int64 = value_;   \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_double(Sqxc* src, const char* entry_name, double value);
#define sqxc_send_double(sqxc_src, entry_name, val)      \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_DOUBLE;\
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.double_ = val;    \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_string(Sqxc* src, const char* entry_name, const char* value);
#define sqxc_send_string(sqxc_src, entry_name, val)      \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_STRING;\
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.string = (char*)val;   \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_object_beg(Sqxc* src, const char* entry_name);
#define sqxc_send_object_beg(sqxc_src, entry_name)       \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_OBJECT;\
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.pointer = NULL;   \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_object_end(Sqxc* src, const char* entry_name);
#define sqxc_send_object_end(sqxc_src, entry_name)       \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_OBJECT_END; \
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.pointer = NULL;   \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_array_beg(Sqxc* src, const char* entry_name);
#define sqxc_send_array_beg(sqxc_src, entry_name)        \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_ARRAY; \
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.pointer = NULL;   \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// void sqxc_send_array_end(Sqxc* src, const char* entry_name);
#define sqxc_send_array_end(sqxc_src, entry_name)        \
		{                                                \
			((Sqxc*)(sqxc_src))->type = SQXC_TYPE_ARRAY_END;  \
			((Sqxc*)(sqxc_src))->name = entry_name;      \
			((Sqxc*)(sqxc_src))->value.pointer = NULL;   \
			sqxc_send((Sqxc*)(sqxc_src));                \
		}

// --------------------------------------------------------
// functions for nested object/array

SqxcNested* sqxc_push_nested(Sqxc* xc);
void        sqxc_pop_nested(Sqxc* xc);

#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// SqxcMethod : a template C++ struct is used by Sqxc and it's children.

#ifdef __cplusplus

namespace Sq
{

struct XcMethod
{
	SqBuffer* buffer(void);

	Sqxc*  get(const SqxcInfo* info, int nth);

	int  ready(void* data = NULL);
	int  finish(void* data = NULL);

	int  sendFrom(Sqxc* src);
	int  sendBool(const char* entry_name, bool value);
	int  sendInt(const char* entry_name, int value);
	int  sendInt64(const char* entry_name, int64_t value);
	int  sendDouble(const char* entry_name, double value);
	int  sendString(const char* entry_name, const char* value);

	int  sendObjectBeg(const char* entry_name);
	int  sendObjectEnd(const char* entry_name);

	int  sendArrayBeg(const char* entry_name);
	int  sendArrayEnd(const char* entry_name);

	SqxcNested* push(void);
	void        pop(void);
};

};  // namespace Sq
#endif  // __cplusplus

// ----------------------------------------------------------------------------
// Sqxc

#define SQXC_MEMBERS       \
	const SqxcInfo*  info; \
	Sqxc*        next;     \
	Sqxc*        prev;     \
	Sqxc*        dest;     \
	unsigned int io_:1;    \
	unsigned int supported_type;   \
	SqxcNested*  nested;           \
	int          nested_count;     \
	char*        buf;              \
	int          buf_size;         \
	int          buf_writed;       \
	SqxcCtrlFunc ctrl;     \
	SqxcSendFunc send;     \
	SqxcType     type;          \
	const char*  name;          \
	union {                     \
		bool          boolean;  \
		int           integer;  \
		int           int_;     \
		unsigned int  uinteger; \
		unsigned int  uint;     \
		int64_t       int64;    \
		uint64_t      uint64;   \
		double        fraction; \
		double        double_;  \
		char*         string;   \
		void*         pointer;  \
	} value;                    \
	SqEntry*     entry;    \
	void**       error;    \
	int          code

#ifdef __cplusplus
struct Sqxc : Sq::XcMethod
#else
struct Sqxc
#endif
{
//	SQXC_MEMBERS;
/*	// ------ Sqxc members ------  */
	const SqxcInfo*  info;

	// Sqxc chain
	Sqxc*        next;     // next destination
	Sqxc*        prev;     // previous source

	// source and destination
//	Sqxc*        src;      // pointer to current source in Sqxc chain
	Sqxc*        dest;     // pointer to current destination in Sqxc chain

	// ----------------------------------------------------
	// properties

	unsigned int io_:1;           // Input = 1, Output = 0
	unsigned int supported_type;  // supported SqxcType (bit entry)

	// ----------------------------------------------------
	// stack of SqxcNested (placed in dest)

	SqxcNested*  nested;          // current nested object/array
	int          nested_count;

	// ----------------------------------------------------
	// Buffer - common buffer for type conversion. To resize this buf:
	// buf = realloc(buf, buf_size);

//	SQ_BUFFER_MEMBERS(buf, buf_size, buf_writed);
	char*        buf;
	int          buf_size;
	int          buf_writed;

	// ====================================================
	// functions

	SqxcCtrlFunc ctrl;
	SqxcSendFunc send;

	// ----------------------------------------------------
	// function arguments

	// input arguments
	SqxcType     type;     // if src->code = SQCODE_TYPE_NOT_MATCH, set required type in dest->type
	const char*  name;
	union {
		bool          boolean;
		int           integer;
		int           int_;
		unsigned int  uinteger;
		unsigned int  uint;
		int64_t       int64;
		int64_t       uint64;
		double        fraction;
		double        double_;
		char*         string;
		void*         pointer;
	} value;

	// input arguments - optional.  this one can be NULL.
	SqEntry*     entry;

	// input arguments - user data
//	void*        user_data;
//	void*        user_data2;

	// input / output arguments
	void**       error;

	// output arguments
	int          code;     // error code (SQCODE_xxxx)
};

// ----------------------------------------------------------------------------
// C++ namespace

#ifdef __cplusplus

namespace Sq
{

// XcMethod

SqBuffer* XcMethod::buffer(void)
	{ return sqxc_get_buffer(this); }

Sqxc*  XcMethod::get(const SqxcInfo* info, int nth)
	{ return sqxc_get((Sqxc*)this, info, nth); }

int  XcMethod::ready(void* data)
	{ return sqxc_ready((Sqxc*)this, data); }
int  XcMethod::finish(void* data)
	{ return sqxc_finish((Sqxc*)this, data); }

int  XcMethod::sendFrom(Sqxc* src)
	{ return ((Sqxc*)this)->send((Sqxc*)this, src); }
int  XcMethod::sendBool(const char* entry_name, bool value)
	{ sqxc_send_bool((Sqxc*)this, entry_name, value);  return ((Sqxc*)this)->code; }
int  XcMethod::sendInt(const char* entry_name, int value)
	{ sqxc_send_int((Sqxc*)this, entry_name, value);  return ((Sqxc*)this)->code; }
int  XcMethod::sendInt64(const char* entry_name, int64_t value)
	{ sqxc_send_int64((Sqxc*)this, entry_name, value);  return ((Sqxc*)this)->code; }
int  XcMethod::sendDouble(const char* entry_name, double value)
	{ sqxc_send_double((Sqxc*)this, entry_name, value);  return ((Sqxc*)this)->code; }
int  XcMethod::sendString(const char* entry_name, const char* value)
	{ sqxc_send_string((Sqxc*)this, entry_name, (char*)value);  return ((Sqxc*)this)->code; }

int  XcMethod::sendObjectBeg(const char* entry_name)
	{ sqxc_send_object_beg((Sqxc*)this, entry_name);  return ((Sqxc*)this)->code; }
int  XcMethod::sendObjectEnd(const char* entry_name)
	{ sqxc_send_object_end((Sqxc*)this, entry_name);  return ((Sqxc*)this)->code; }

int  XcMethod::sendArrayBeg(const char* entry_name)
	{ sqxc_send_array_beg((Sqxc*)this, entry_name);  return ((Sqxc*)this)->code; }
int  XcMethod::sendArrayEnd(const char* entry_name)
	{ sqxc_send_array_end((Sqxc*)this, entry_name);  return ((Sqxc*)this)->code; }

SqxcNested* XcMethod::push(void)
	{ return sqxc_push_nested((Sqxc*)this); }
void        XcMethod::pop(void)
	{ sqxc_pop_nested((Sqxc*)this); }

// These are for directly use only. You can NOT derived it.
typedef struct Sqxc             Xc;
typedef struct SqxcInfo         XcInfo;
typedef struct SqxcNested       XcNested;

typedef        SqxcType         XcType;

};  // namespace Sq

#endif  // __cplusplus

#endif  // SQXC_H
