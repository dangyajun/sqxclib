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

#ifndef SQ_SCHEMA_H
#define SQ_SCHEMA_H

#ifdef __cplusplus
#include <typeinfo>
#endif

#include <stddef.h>
#include <stdlib.h>

#include <SqPtrArray.h>
#include <SqTable.h>

/*	DataBase object - [Server Name].[DataBase Name].[Schema].[Table Name]
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SqSchema       SqSchema;

// SqSchema::bit_field for internal use only
#define SQB_SCHEMA_INCLUDED                (1 << 15)

// ----------------------------------------------------------------------------
// C functions

SqSchema* sq_schema_new(const char* name);
void      sq_schema_free(SqSchema* schema);

/*	sq_schema_create_full()
  if "type_info" == NULL, program will use "type_name" and "type_size" to create "type_info".
  You can pass 0 to "type_size" because program calculate type_size automatically.

  C Language
  #define SQ_HAVE_NAMING_CONVENTION to enable "table_name" <-> "type_name" converting,
  program only need one of table_name, type_name, or type_info->name.

  C++ Language
  It doesn't need #define SQ_HAVE_NAMING_CONVENTION because program
  use typeid(Type).name() to get "type_name".
 */
SqTable* sq_schema_create_full(SqSchema* schema,
                               const char* table_name,
                               const SqType* type_info,
                               const char* type_name,
                               size_t instance_size);

#define sq_schema_create_by_type(schema, table_name, type_info)  \
		sq_schema_create_full(schema, table_name, type_info, NULL, 0)

#define sq_schema_create(schema, table_name, StructType)  \
		sq_schema_create_full(schema, table_name, NULL, SQ_GET_TYPE_NAME(StructType), sizeof(StructType))

#ifdef SQ_HAVE_NAMING_CONVENTION
#define sq_schema_create_as(schema, StructType)  \
		sq_schema_create_full(schema, NULL, NULL, SQ_GET_TYPE_NAME(StructType), sizeof(StructType))
#endif

SqTable* sq_schema_alter(SqSchema* schema, const char* table_name, const SqType* type_info);
void     sq_schema_drop(SqSchema* schema, const char* name);
void     sq_schema_rename(SqSchema* schema, const char* from, const char* to);

SqTable* sq_schema_find(SqSchema* schema, const char* table_name);
SqTable* sq_schema_find_type(SqSchema* schema, const char* type_name);

/* --------------------------------------------------------
	migration functions

	// --- if database_schema_version is 3, get current schema in database
	sq_schema_include(schema, schema_v2);
	sq_schema_include(schema, schema_v3);
	sq_schema_trace_foreign(schema);
	sq_schema_clear_records(schema, 0, SQB_TABLE_CREATED);

	// --- if the latest schema_version is 5, migrate to schema_v5
	// migrate schema_v4 and schema_v5 by SQL statement
	sq_schema_include(schema, schema_v4);
	sq_schema_include(schema, schema_v5);
	sq_schema_trace_foreign(schema);
	// SQLite must try to rename, drop, or create table
	//                    rename, or add column here
	sq_schema_clear_records(schema, 1, 0);

	sq_schema_arrange(schema, entries);
	// create table by SQL statement
	// SQLite must try to recreate or create table here

	sq_schema_complete(schema);
 */

// This used by migration: include and apply changes from 'schema_src'.
// It may move/steal tables and column from 'schema_src'.
int     sq_schema_include(SqSchema* schema, SqSchema* schema_src);

// It trace renamed (or dropped) table/column that was referenced by others and update others references.
int     sq_schema_trace_foreign(SqSchema* schema);

/* clear changed records after calling sq_schema_include() and sq_schema_trace_foreign()
   To pass 1 (or TRUE) to 'reset_traced_position':
   1. If you don't need calling sq_schema_trace_foreign() any more
   2. before you call sq_schema_arrange()

   If you pass 1 (TRUE) to 'reset_traced_position', it will affect performance of sq_schema_trace_foreign().
 */
void    sq_schema_clear_records(SqSchema* schema, int reset_traced_position, unsigned int set_table_bit_field);

/* call this function before creating SQL table after sq_schema_clear_records(schema, 1, 0)
   if table has no foreign key, this function move it to front.
   if table references most tables, this function move it to end.
   if table references each other, table->extra->foreigns.length > 0
   output arranged tables in 'entries'
 */
void    sq_schema_arrange(SqSchema* schema, SqPtrArray* entries);

// call this function after creating/altering SQL table.
// It will free temporary data (e.g. table->foreigns)
void    sq_schema_complete(SqSchema* schema);

#ifdef __cplusplus
}  // extern "C"
#endif

/* ----------------------------------------------------------------------------
	SqSchema

	SqEntry
	|
	`--- SqSchema
 */

struct SqSchema
{
	SQ_ENTRY_MEMBERS;
/*	// ------ SqEntry members ------
	SqType*      type;        // type information for this entry
	char*        name;
	size_t       offset;      // sq_schema_trace_foreign() and migration use this
	unsigned int bit_field;
 */
	int          version;

	// table_types is sorted by SqTable.type.name
	SqPtrArray   table_types;
	bool         table_types_sorted;

#ifdef __cplusplus
	// C++11 standard-layout
	// ----------------------------------------------------
/*
	void* operator new(size_t size)
		{ return malloc(size); }
	void operator delete(void* instance)
		{ free(instance); }
	SqSchema(const char* name)
		{ sq_schema_init(this, name); }
	~SqSchema()
		{ sq_schema_final(this); }
 */

	SqTable* create(const char* name,
	                const SqType* type_info,
	                const char* type_name = NULL,
	                size_t instance_size = 0)
		{ return sq_schema_create_full(this, name, type_info, type_name, instance_size); }

	template <class StructType>
	SqTable* create(const char* name)
		{ return sq_schema_create_full(this, name, NULL, typeid(StructType).name(), sizeof(StructType)); }

	SqTable* alter(const char* name, const SqType* type_info = NULL)
		{ return sq_schema_alter(this, name, type_info); }
#endif  // __cplusplus
};

// ----------------------------------------------------------------------------
// C++ namespace

#ifdef __cplusplus
namespace Sq
{
// These are for directly use only. You can NOT derived it.
typedef struct SqSchema     Schema;
};  // namespace Sq
#endif  // __cplusplus


#endif  // SQ_SCHEMA_H
