/*
 *   Copyright (C) 2022 by C.H. Huang
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

#ifndef SQ_ROW_H
#define SQ_ROW_H

#include <SqType.h>
#include <SqJoint.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structue, macro, enumeration.

typedef struct SqRow              SqRow;
typedef struct SqRowColumn        SqRowColumn;
typedef        SqTypeJoint        SqTypeRow;

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

// --- SqRow functions ---
SqRow *sq_row_new(int cols_allocated, int allocated);
void   sq_row_free(SqRow *row);

void   sq_row_init(SqRow *row, int cols_allocated, int allocated);
void   sq_row_final(SqRow *row);

// User can NOT use this function directly.
void  *sq_row_alloc_part(SqRow *row, int n_element, int part, size_t element_size);

// SqValue  *sq_row_alloc(SqRow *row, int n_element);
#define sq_row_alloc(row, n_element)  \
		( (SqValue*)    sq_row_alloc_part(row, n_element, 0, sizeof(SqValue)) )

// SqRowColumn *sq_row_alloc_column(SqRow *row, int n_element);
#define sq_row_alloc_column(row, n_element)  \
		( (SqRowColumn*)sq_row_alloc_part(row, n_element, 1, sizeof(SqRowColumn)) )

// --- SqTypeRow functions ---

SqTypeRow *sq_type_row_new(void);
void       sq_type_row_free(SqTypeRow *type_row);

void  sq_type_row_init(SqTypeRow *type_row);
void  sq_type_row_final(SqTypeRow *type_row);

// SqTypeRow is derived from SqTypeJoint, it can use these SqTypeJoint functions.

// void sq_type_row_add(SqTypeRow *type_row, SqTable *table, const char *table_as_name);
#define sq_type_row_add(type_row, table, table_as_name)    \
		sq_type_joint_add((SqTypeJoint*)(type_row), table, table_as_name)

// void sq_type_row_erase(SqTypeRow *type_row, SqTable *table, const char *table_as_name);
#define sq_type_row_erase(type_row, table, table_as_name)    \
		sq_type_joint_erase((SqTypeJoint*)(type_row), table, table_as_name)

// void sq_type_row_clear(SqTypeRow *type_row);
#define sq_type_row_clear(type_row)    \
		sq_type_joint_clear((SqTypeJoint*)(type_row))

#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// C++ declarations: declare C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

typedef struct SqRowColumn  RowColumn;

/*
	TypeJointMethod
	|
	`--- TypeRowMethod

	TypeRowMethod is used by SqTypeRow and it's children.

	It's derived struct/class must be C++11 standard-layout and has SqTypeRow members.
 */
struct TypeRowMethod : Sq::TypeJointMethod {
};

/*	RowMethod is used by SqRow and it's children.

	It's derived struct/class must be C++11 standard-layout and has SqRow members.
 */
struct RowMethod {
	Sq::Value     *alloc(int  n_element = 1);
	Sq::RowColumn *allocColumn(int  n_element = 1);
};

};  // namespace Sq

#endif  // __cplusplus

// ----------------------------------------------------------------------------
// C/C++ common definitions: define structue

/*
	SqRow.cols.type is type of SqRow.data.xxx
	If SqRow.cols.type is SQ_TYPE_BOOL,   value is SqRow.data.boolean
	If SqRow.cols.type is SQ_TYPE_STRING, value is SqRow.data.string
 */
struct SqRowColumn
{
	const char    *name;
	const SqType  *type;
	const SqEntry *entry;
};

/*
	SqRow - This can handle unknown(or known) table and column

	Note: data type and order of below SqRow's members can NOT change:
	      data, length, allocated, cols, cols_length, and cols_allocated.
 */
#define SQ_ROW_MEMBERS               \
	SqValue      *data;              \
	int           length;            \
	int           allocated;         \
	SqRowColumn  *cols;              \
	int           cols_length;       \
	int           cols_allocated

#ifdef __cplusplus
struct SqRow : Sq::RowMethod                 // <-- 1. inherit C++ member function(method)
#else
struct SqRow
#endif
{
	SQ_ROW_MEMBERS;                          // <-- 2. inherit member variable
/*	// ------ SqRow members ------
	SqValue      *data;
	int           length;
	int           allocated;

	SqRowColumn  *cols;
	int           cols_length;
	int           cols_allocated;
 */
};

/*	SqTypeRow - SqTypeRow is derived from SqTypeJoint. It support unknown (or known) table and column.
	            It can create instance of SqRow.

	SqType
	|
	`--- SqTypeJoint
	     |
	     `--- SqTypeRow
 */
#define SQ_TYPE_ROW_MEMBERS    SQ_TYPE_JOINT_MEMBERS

// ----------------------------------------------------------------------------
// C/C++ common definitions: define global inline function

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || defined(__cplusplus)
// define inline functions here if compiler supports inline function.

#else   // __STDC_VERSION__ || __cplusplus
// declare functions here if compiler does NOT support inline function.

#endif  // __STDC_VERSION__ || __cplusplus

// ----------------------------------------------------------------------------
// C++ definitions: define C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

/* define TypeRowMethod functions. */

/* define RowMethod functions. */
Sq::Value *RowMethod::alloc(int  n_element) {
	return sq_row_alloc((SqRow*)this, n_element);
}
Sq::RowColumn *RowMethod::allocColumn(int  n_element) {
	return sq_row_alloc_column((SqRow*)this, n_element);
}

/* All derived struct/class must be C++11 standard-layout. */

struct TypeRow : SqTypeRow, TypeRowMethod {
	TypeRow() {
		sq_type_row_init((SqTypeRow*)this);
	}
	~TypeRow() {
		sq_type_row_final((SqTypeRow*)this);
	}
};

struct Row : SqRow {
	Row(int colAllocated = 0, int allocated = 0) {
		sq_row_init((SqRow*)this, colAllocated, allocated);
	}
	~Row() {
		sq_row_final((SqRow*)this);
	}
};

};  // namespace Sq

#endif  // __cplusplus


#endif  // SQ_ROW_H