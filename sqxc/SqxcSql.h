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

#ifndef SQXC_SQL_H
#define SQXC_SQL_H

#include <SqPtrArray.h>
#include <Sqxc.h>
#include <Sqdb.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------
	SqxcSql - Sqxc data convert to SQL statement. (destination of output chain)

	Sqxc
	|
	`--- SqxcSql

                     +-> SqxcJsonWriter --+
    ( output )       |                    |       (SQL statement)
    SqType.write() --+--------------------+-> SqxcSql   ---> Sqdb.exec()
                     |                    |
                     +--> SqxcXmlWriter --+
 */

typedef struct SqxcSql        SqxcSql;

extern const SqxcInfo *SQXC_INFO_SQL;

// ----------------------------------------------------------------------------
// macro for accessing variable of SqxcSqlite

#define sqxc_sql_db(xcsql)    ((SqxcSql*)xcsql)->db
#define sqxc_sql_id(xcsql)    ((SqxcSql*)xcsql)->id

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
struct SqxcSql : Sq::XcMethod
#else
struct SqxcSql
#endif
{
	SQXC_MEMBERS;
/*	// ------ Sqxc members ------
	const SqxcInfo  *info;

	// Sqxc chain
	Sqxc*        peer;     // pointer to other Sqxc elements
	Sqxc*        dest;     // pointer to current destination in Sqxc chain

	// stack of SqxcNested
	SqxcNested*  nested;          // current nested object/array
	int          nested_count;

	// ----------------------------------------------------
	// Buffer - common buffer for type conversion. To resize this buf:
	// buf = realloc(buf, buf_size);

//	SQ_BUFFER_MEMBERS(buf, buf_size, buf_writed);
	char*        buf;
	int          buf_size;
	int          buf_writed;

	// ----------------------------------------------------
	// arguments that used by SqxcInfo->send()

	// special arguments
	SqEntry*     entry;           // SqxcJsonc and SqxcSql use it to decide output. this can be NULL (optional).
	uint16_t     supported_type;  // supported SqxcType (bit field) for inputting, it can change at runtime.
//	uint16_t     outputable_type; // supported SqxcType (bit field) for outputting, it can change at runtime.
	// output arguments
//	uint16_t     required_type;   // required SqxcType (bit field) if 'code' == SQCODE_TYPE_NOT_MATCH
	uint16_t     code;            // error code (SQCODE_xxxx)

	// input arguments
	uint16_t     type;            // input SqxcType
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
		char*         stream;     // Text stream must be null-terminated string
		void*         pointer;
	} value;

	// input / output arguments
	void**       error;
 */

	// output
	Sqdb*        db;

	// controlled variable
	unsigned int mode;        // 1 == INSERT, 0 == UPDATE
	int          id;          // inserted id; update id if 'condition' == NULL
	char*        condition;   // WHERE condition if mode == 0 (UPDATE)

	// runtime variable
	uint16_t     outer_type;  // SQXC_TYPE_OBJECT, SQXC_TYPE_ARRAY or SQXC_TYPE_NONE
	int          row_count;   // used by INSERT
	int          col_count;   // used by INSERT and UPDATE
	int          buf_reuse;   // used by INSERT and UPDATE
};

#endif  // SQXC_SQL_H