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

#ifndef SQ_COLUMN_H
#define SQ_COLUMN_H

#include <stdarg.h>

#include <SqType.h>
#include <SqEntry.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structure, macro, enumeration.

typedef struct SqColumn       SqColumn;
typedef struct SqForeign      SqForeign;    // used by SqColumn


// SQL special type
#define SQ_TYPE_CONSTRAINT    SQ_TYPE_FAKE0
#define SQ_TYPE_INDEX         SQ_TYPE_FAKE1

#define SQ_N_COLUMNS(ColumnArray)    ( sizeof(ColumnArray) / sizeof(ColumnArray[0]) )

// SQL common bit_field
#define SQB_COLUMN_CHANGED              SQB_CHANGED         // SQL: column has been changed.
// SQL common bit_field (for internal use only. use it when SQLite recreate)
#define SQB_COLUMN_RENAMED              SQB_RENAMED         // SQL: column has been renamed.

/* SqColumn::bit_field for migration
   SQB_COLUMN_XXX must large then SQB_RESERVE_END because it derived from SqEntry. (SqEntry.h)
   SQB_COLUMN_XXX can overlap SQB_TABLE_XXX (SqTable.h)
 */
/* --- SqColumn::bit_field (Column Modifiers) --- */
#define SQB_COLUMN_PRIMARY              SQB_PRIMARY         // SQL: PRIMARY KEY
#define SQB_COLUMN_FOREIGN              SQB_FOREIGN         // SQL: FOREIGN KEY
#define SQB_COLUMN_UNIQUE               SQB_UNIQUE          // SQL: UNIQUE
#define SQB_COLUMN_INCREMENT            SQB_INCREMENT       // SQL: AUTOINCREMENT == SQB_AUTOINCREMENT
#define SQB_COLUMN_AUTOINCREMENT        SQB_AUTOINCREMENT   // SQL: AUTOINCREMENT == SQB_INCREMENT
#define SQB_COLUMN_NULLABLE             SQB_NULLABLE        // SQL: remove "NOT NULL"

// SqColumn::type == SQ_TYPE_TIME (use CURRENT_TIMESTAMP)
#define SQB_COLUMN_CURRENT              SQB_CURRENT              // SQL: DEFAULT CURRENT_TIMESTAMP
#define SQB_COLUMN_CURRENT_ON_UPDATE    SQB_CURRENT_ON_UPDATE    // SQL: CREATE TRIGGER AFTER UPDATE
#define SQB_COLUMN_CURRENT_ALL          SQB_CURRENT_ALL

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

/* --- macro functions --- for maintaining C/C++ inline functions easily */
#define SQ_COLUMN_SET_DEFAULT(column, default_value_str)    \
		sq_entry_set_str_addr((SqEntry*)column, (char**) &((SqColumn*)column)->default_value, default_value_str)

#define SQ_COLUMN_SET_RAW(column, raw_property_str)    \
		sq_entry_set_str_addr((SqEntry*)column, (char**) &((SqColumn*)column)->raw, raw_property_str)

/* SqColumn C functions */

SqColumn  *sq_column_new(const char *name, const SqType *type_info);
void       sq_column_free(SqColumn *column);

void       sq_column_init(SqColumn *column, const char *name, const SqType *type_info);
void       sq_column_final(SqColumn *column);

// create new SqColumn and copy data from static one.
SqColumn  *sq_column_copy_static(const SqColumn *column_src);

// foreign key references
void       sq_column_reference(SqColumn *column,
                               const char *foreign_table_name,
                               const char *foreign_column_name);
// foreign key on delete
void       sq_column_on_delete(SqColumn *column, const char *act);
// foreign key on update
void       sq_column_on_update(SqColumn *column, const char *act);

// void sq_column_foreign(SqColumn *column, const char *foreign_table_name, const char *foreign_column_name);
#define sq_column_foreign    sq_column_reference

// the last argument must be NULL
// sq_column_set_composite(column, colume_name1, column_name2, NULL);
void       sq_column_set_composite(SqColumn *column, ...);
void       sq_column_set_composite_va(SqColumn *column, va_list arg_list);

#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// C++ declarations: declare C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

struct Column;

};  // namespace Sq

#endif  // __cplusplus

// ----------------------------------------------------------------------------
// C/C++ common definitions: define structure

/*	SqForeign: foreign key data in SqColumn
 */
struct SqForeign
{
	// Note: use 'const char*' to declare string here, C++ user can initialize static structure easily.
	const char  *table;
	const char  *column;
	const char  *on_delete;
	const char  *on_update;
};

/*	SqColumn defines column in SQL table.

	SqEntry
	|
	`--- SqReentry
	     |
	     `--- SqColumn

	Migration - Alter Type : column->bit_field & SQB_COLUMN_CHANGED
	Migration - Drop   : column->name = NULL, column->old_name = column_name
	Migration - Rename : column->name = new_name, column->old_name = old_name

	Because 'const' is used to define string 'char*' and type 'SqType*',
	C++ user can initialize static structure easily.

	SqColumn must have no base struct because I need use aggregate initialization with it.
*/

#define SQ_COLUMN_MEMBERS       \
	SQ_REENTRY_MEMBERS;         \
	int16_t      size;          \
	int16_t      digits;        \
	const char  *default_value; \
	SqForeign   *foreign;       \
	char       **composite;     \
	const char  *reserve;       \
	const char  *raw

struct SqColumn
{
	SQ_COLUMN_MEMBERS;
/*	// ------ SqEntry members ------
	const SqType *type;             // field type
	const char   *name;             // column name
	size_t        offset;
	unsigned int  bit_field;

	// ------ SqReentry members ------
	const char   *old_name;         // use this when renaming or dropping

	// ------ SqColumn members ------

	// size  : total number of digits is specified in size, or length of string
	int16_t       size;
	// digits: number of digits after the decimal point.
	int16_t       digits;

	const char   *default_value;    // DEFAULT

	SqForeign    *foreign;          // foreign key
	char        **composite;        // Null-terminated (column-name) string array

	const char   *reserve;          // reserve, set it to NULL.
	const char   *raw;              // raw SQL column property

	// if column->name is NULL, it will drop column->old_name
	// if column->name is NOT NULL, it will rename from column->old_name to column->name
 */

#ifdef __cplusplus
	/* Note: If you add, remove, or change methods here, do the same things in Sq::ColumnMethod. */

	Sq::Column *operator->() {
		return (Sq::Column*)this;
	}

	void        setName(const char *columnName) {
		SQ_ENTRY_SET_NAME(this, columnName);
	}
	Sq::Column &reference(const char *table_name, const char *column_name) {
		sq_column_reference((SqColumn*)this, table_name, column_name);
		return *(Sq::Column*)this;
	}
	Sq::Column &onDelete(const char *act) {
		sq_column_on_delete((SqColumn*)this, act);
		return *(Sq::Column*)this;
	}
	Sq::Column &onUpdate(const char *act) {
		sq_column_on_update((SqColumn*)this, act);
		return *(Sq::Column*)this;
	}
	template <typename... Args>
	Sq::Column &setComposite(const Args... args) {
		sq_column_set_composite((SqColumn*)this, args..., NULL);
		return *(Sq::Column*)this;
	}

	/* --- Column Modifiers --- */
	// C/C++ pointer
	Sq::Column &pointer() {
		((SqColumn*)this)->bit_field |= SQB_POINTER;
		return *(Sq::Column*)this;
	}
	// JSON
	Sq::Column &hidden() {
		((SqColumn*)this)->bit_field |= SQB_HIDDEN;
		return *(Sq::Column*)this;
	}
	Sq::Column &hiddenNull() {
		((SqColumn*)this)->bit_field |= SQB_HIDDEN_NULL;
		return *(Sq::Column*)this;
	}
	// SQL column property
	Sq::Column &primary() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_PRIMARY;
		return *(Sq::Column*)this;
	}
	Sq::Column &unique() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_UNIQUE;
		return *(Sq::Column*)this;
	}
	Sq::Column &increment() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_INCREMENT;    // equal SQB_COLUMN_AUTOINCREMENT
		return *(Sq::Column*)this;
	}
	Sq::Column &autoIncrement() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_AUTOINCREMENT;
		return *(Sq::Column*)this;
	}
	Sq::Column &nullable() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_NULLABLE;
		return *(Sq::Column*)this;
	}
	Sq::Column &change() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_CHANGED;
		return *(Sq::Column*)this;
	}
	Sq::Column &useCurrent() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_CURRENT;
		return *(Sq::Column*)this;
	}
	Sq::Column &useCurrentOnUpdate() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_CURRENT_ON_UPDATE;
		return *(Sq::Column*)this;
	}

	Sq::Column &default_(const char *default_val) {
		SQ_COLUMN_SET_DEFAULT(this, default_val);
		return *(Sq::Column*)this;
	}
	Sq::Column &defaultValue(const char *default_val) {
		SQ_COLUMN_SET_DEFAULT(this, default_val);
		return *(Sq::Column*)this;
	}
	Sq::Column &raw_(const char *raw_property) {
		SQ_COLUMN_SET_RAW(this, raw_property);
		return *(Sq::Column*)this;
	}
	Sq::Column &rawProperty(const char *raw_property) {
		SQ_COLUMN_SET_RAW(this, raw_property);
		return *(Sq::Column*)this;
	}
#endif  // __cplusplus
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
void  sq_column_set_name(SqColumn *column, const char *name) {
	SQ_ENTRY_SET_NAME(column, name);
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_pointer(SqColumn *column) {
	column->bit_field |= SQB_POINTER;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_hidden(SqColumn *column) {
	column->bit_field |= SQB_HIDDEN;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_hidden_null(SqColumn *column) {
	column->bit_field |= SQB_HIDDEN_NULL;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_primary(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_PRIMARY;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_unique(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_UNIQUE;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_increment(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_INCREMENT;    // equal SQB_COLUMN_AUTOINCREMENT
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_auto_increment(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_AUTOINCREMENT;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_nullable(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_NULLABLE;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_change(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_CHANGED;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_use_current(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_CURRENT;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_use_current_on_update(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_CURRENT_ON_UPDATE;
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_default(SqColumn *column, const char *default_value) {
	SQ_COLUMN_SET_DEFAULT(column, default_value);
}

#ifdef __cplusplus  // C++
inline
#else               // C99
static inline
#endif
void  sq_column_raw(SqColumn *column, const char *raw_property) {
	SQ_COLUMN_SET_RAW(column, raw_property);
}

#else   // __STDC_VERSION__ || __cplusplus
// declare functions here if compiler does NOT support inline function.

void  sq_column_set_name(SqColumn *column, const char *name);
void  sq_column_pointer(SqColumn *column);
void  sq_column_hidden(SqColumn *column);
void  sq_column_hidden_null(SqColumn *column);
void  sq_column_primary(SqColumn *column);
void  sq_column_unique(SqColumn *column);
void  sq_column_increment(SqColumn *column);
void  sq_column_auto_increment(SqColumn *column);
void  sq_column_nullable(SqColumn *column);
void  sq_column_change(SqColumn *column);
void  sq_column_use_current(SqColumn *column);
void  sq_column_use_current_on_update(SqColumn *column);
void  sq_column_default(SqColumn *column, const char *default_value);
void  sq_column_raw(SqColumn *column, const char *raw_property);

#endif  // __STDC_VERSION__ || __cplusplus

// ----------------------------------------------------------------------------
// C++ definitions: define C++ data, function, method, and others.

#ifdef __cplusplus
namespace Sq {

/*	ColumnMethod is used by SqColumn's children.

	It's derived struct/class must be C++11 standard-layout and has SqColumn members.

	Note: If you add, remove, or change methods here, do the same things in SqColumn.
 */
struct ColumnMethod
{
	Sq::Column *operator->() {
		return (Sq::Column*)this;
	}

	void        setName(const char *columnName) {
		SQ_ENTRY_SET_NAME(this, columnName);
	}
	Sq::Column &reference(const char *table_name, const char *column_name) {
		sq_column_reference((SqColumn*)this, table_name, column_name);
		return *(Sq::Column*)this;
	}
	Sq::Column &onDelete(const char *act) {
		sq_column_on_delete((SqColumn*)this, act);
		return *(Sq::Column*)this;
	}
	Sq::Column &onUpdate(const char *act) {
		sq_column_on_update((SqColumn*)this, act);
		return *(Sq::Column*)this;
	}
	template <typename... Args>
	Sq::Column &setComposite(const Args... args) {
		sq_column_set_composite((SqColumn*)this, args..., NULL);
		return *(Sq::Column*)this;
	}

	/* --- Column Modifiers --- */
	// C/C++ pointer
	Sq::Column &pointer() {
		((SqColumn*)this)->bit_field |= SQB_POINTER;
		return *(Sq::Column*)this;
	}
	// JSON
	Sq::Column &hidden() {
		((SqColumn*)this)->bit_field |= SQB_HIDDEN;
		return *(Sq::Column*)this;
	}
	Sq::Column &hiddenNull() {
		((SqColumn*)this)->bit_field |= SQB_HIDDEN_NULL;
		return *(Sq::Column*)this;
	}
	// SQL column property
	Sq::Column &primary() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_PRIMARY;
		return *(Sq::Column*)this;
	}
	Sq::Column &unique() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_UNIQUE;
		return *(Sq::Column*)this;
	}
	Sq::Column &increment() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_INCREMENT;    // equal SQB_COLUMN_AUTOINCREMENT
		return *(Sq::Column*)this;
	}
	Sq::Column &autoIncrement() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_AUTOINCREMENT;
		return *(Sq::Column*)this;
	}
	Sq::Column &nullable() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_NULLABLE;
		return *(Sq::Column*)this;
	}
	Sq::Column &change() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_CHANGED;
		return *(Sq::Column*)this;
	}
	Sq::Column &useCurrent() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_CURRENT;
		return *(Sq::Column*)this;
	}
	Sq::Column &useCurrentOnUpdate() {
		((SqColumn*)this)->bit_field |= SQB_COLUMN_CURRENT_ON_UPDATE;
		return *(Sq::Column*)this;
	}

	Sq::Column &default_(const char *default_val) {
		SQ_COLUMN_SET_DEFAULT(this, default_val);
		return *(Sq::Column*)this;
	}
	Sq::Column &defaultValue(const char *default_val) {
		SQ_COLUMN_SET_DEFAULT(this, default_val);
		return *(Sq::Column*)this;
	}
	Sq::Column &raw_(const char *raw_property) {
		SQ_COLUMN_SET_RAW(this, raw_property);
		return *(Sq::Column*)this;
	}
	Sq::Column &rawProperty(const char *raw_property) {
		SQ_COLUMN_SET_RAW(this, raw_property);
		return *(Sq::Column*)this;
	}
};

/* All derived struct/class must be C++11 standard-layout. */

struct Column : SqColumn {
	Column(const char *name = NULL, const SqType *dataType = NULL) {
		sq_column_init(this, name, dataType);
	}
	~Column() {
		sq_column_final(this);
	}
};

typedef struct SqForeign   Foreign;

};  // namespace Sq

#endif  // __cplusplus


#endif  // SQ_COLUMN_H
