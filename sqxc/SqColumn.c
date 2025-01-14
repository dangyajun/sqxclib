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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <SqColumn.h>

#ifdef _MSC_VER
#define strdup       _strdup
#endif

static void  sq_foreign_free(SqForeign *foreign);
static SqForeign *sq_foreign_copy(SqForeign *src);

// ----------------------------------------------------------------------------
// SqColumn C functions

#define SQ_COMPOSITE_LENGTH_DEFAULT    4

#define sq_composite_capacity(constraint)    *( (intptr_t*) ((constraint)-1) )
#define sq_composite_alloc(n)                 ( (char**)malloc(sizeof(char*) *(n+1)) +1)
#define sq_composite_realloc(constraint, n)   ( (char**)realloc((constraint)-1, sizeof(char*) *(n+1)) +1)
#define sq_composite_free(constraint)         free((constraint)-1)

SqColumn  *sq_column_new(const char *name, const SqType *typeinfo)
{
	SqColumn  *column;

	column = malloc(sizeof(SqColumn));
	sq_column_init(column, name, typeinfo);
	return column;
}

void  sq_column_free(SqColumn *column)
{
	if (column->bit_field & SQB_DYNAMIC) {
		sq_column_final(column);
		free(column);
	}
}

void  sq_column_init(SqColumn *column, const char *name, const SqType *type_info)
{
	memset(column, 0, sizeof(SqColumn));
	// init SqEntry members
	sq_entry_init((SqEntry*)column, type_info);
	column->name = strdup(name);
}

void  sq_column_final(SqColumn *column)
{
	if (column->bit_field & SQB_DYNAMIC) {
		// finalize parent struct - SqEntry
		sq_entry_final((SqEntry*)column);
		// free SqColumn
		free((char*)column->default_value);
//		free((char*)column->reserve);
		free((char*)column->raw);
		free((char*)column->old_name);
		if (column->foreign)
			sq_foreign_free(column->foreign);
		if (column->composite) {
			for (char **cur = column->composite;  *cur;  cur++)
				free(*cur);
			sq_composite_free(column->composite);
		}
	}
}

SqColumn *sq_column_copy_static(const SqColumn *column_src)
{
	SqColumn *column;
	int       index, length;

	column = malloc(sizeof(SqColumn));
	column->type      = column_src->type;
	column->offset    = column_src->offset;
	column->bit_field = column_src->bit_field | SQB_DYNAMIC;
	column->size      = column_src->size;
	column->digits    = column_src->digits;

	column->name          = column_src->name ? strdup(column_src->name) : NULL;
	column->default_value = column_src->default_value ? strdup(column_src->default_value) : NULL;
	column->reserve       = NULL;
//	column->reserve       = column_src->reserve ? strdup(column_src->reserve) : NULL;
	column->raw           = column_src->raw ? strdup(column_src->raw) : NULL;
	column->old_name      = column_src->old_name ? strdup(column_src->old_name) : NULL;

	column->foreign = NULL;
	if (column_src->foreign) {
		column->foreign = sq_foreign_copy(column_src->foreign);
		column->bit_field |= SQB_COLUMN_FOREIGN;
	}

	column->composite = NULL;
	if (column_src->composite) {
		for (index = 0;  column_src->composite[index];  index++)
			;
		if (index > 0) {
			length = index + 1;
			column->composite = sq_composite_alloc(length);
			column->composite[index] = NULL;
			for (index = 0;  index < length;  index++)
				column->composite[index] = strdup(column_src->composite[index]);
		}
	}
	return column;
}

// foreign key references
void  sq_column_reference(SqColumn *column,
                          const char *foreign_table_name,
                          const char *foreign_column_name)
{
	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;

	// remove foreign key if foreign_table_name == NULL
	if (foreign_table_name == NULL) {
		if (column)
			sq_foreign_free(column->foreign);
		column->foreign = NULL;
		return;
	}

	if (column->foreign == NULL) {
		column->foreign = malloc(sizeof(SqForeign));
		column->foreign->on_delete = NULL;
		column->foreign->on_update = NULL;
	}
	else {
		free((char*)column->foreign->table);
		free((char*)column->foreign->column);
	}

	column->foreign->table = strdup(foreign_table_name);
	column->foreign->column = strdup(foreign_column_name);
}

// foreign key on delete
void  sq_column_on_delete(SqColumn *column, const char *act)
{
	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;

	if (column->foreign) {
		free((char*)column->foreign->on_delete);
		column->foreign->on_delete = (act) ? strdup(act) : NULL;
	}
}

// foreign key on update
void  sq_column_on_update(SqColumn *column, const char *act)
{
	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;

	if (column->foreign) {
		free((char*)column->foreign->on_update);
		column->foreign->on_update = (act) ? strdup(act) : NULL;
	}
}

void  sq_column_set_composite(SqColumn *column, ...)
{
	va_list  arg_list;

	va_start(arg_list, column);
	sq_column_set_composite_va(column, arg_list);
	va_end(arg_list);
}

void  sq_column_set_composite_va(SqColumn *column, va_list arg_list)
{
	int   index, capacity;

	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;

	if (column->composite == NULL) {
		column->composite = sq_composite_alloc(SQ_COMPOSITE_LENGTH_DEFAULT);
		capacity = SQ_COMPOSITE_LENGTH_DEFAULT;
	}
	else {
		capacity = (int)sq_composite_capacity(column->composite);
		for (index = 0;  column->composite[index];  index++)
			free(column->composite[index]);
	}

	for (index = 0;  ;  index++) {
		// add string to null terminated string array 
		if (index == capacity) {
			capacity *= 2;
			column->composite = sq_composite_realloc(column->composite, capacity);
			sq_composite_capacity(column->composite) = capacity;
		}
		const char *name = va_arg(arg_list, const char*);
		if (name)
			column->composite[index] = strdup(name);
		else {
			column->composite[index] = NULL;
			break;
		}
	}
}

// ----------------------------------------------------------------------------
// SqForeign C functions

static void  sq_foreign_free(SqForeign *foreign)
{
	free((char*)foreign->table);
	free((char*)foreign->column);
	free((char*)foreign->on_delete);
	free((char*)foreign->on_update);
	free((char*)foreign);
}

static SqForeign *sq_foreign_copy(SqForeign *src)
{
	SqForeign  *foreign;

	foreign = malloc(sizeof(SqForeign));
	foreign->table  = strdup(src->table);
	foreign->column = strdup(src->column);
	foreign->on_delete = strdup(src->on_delete);
	foreign->on_update = strdup(src->on_update);
	return foreign;
}

// ----------------------------------------------------------------------------
// If C compiler doesn't support C99 inline function.

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
// C99 or C++ inline functions has defined in SqColumn.h

#else   // __STDC_VERSION__
// define functions here if compiler does NOT support inline function.

void  sq_column_set_name(SqColumn *column, const char *name) {
	SQ_ENTRY_SET_NAME(column, name);
}

void  sq_column_pointer(SqColumn *column) {
	column->bit_field |= SQB_POINTER;
}

void  sq_column_hidden(SqColumn *column) {
	column->bit_field |= SQB_HIDDEN;
}

void  sq_column_hidden_null(SqColumn *column) {
	column->bit_field |= SQB_HIDDEN_NULL;
}

void  sq_column_primary(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_PRIMARY;
}

void  sq_column_unique(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_UNIQUE;
}

void  sq_column_increment(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_INCREMENT;    // equal SQB_COLUMN_AUTOINCREMENT
}

void  sq_column_auto_increment(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_AUTOINCREMENT;
}

void  sq_column_nullable(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_NULLABLE;
}

void  sq_column_change(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_CHANGED;
}

void  sq_column_use_current(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_CURRENT;
}

void  sq_column_use_current_on_update(SqColumn *column) {
	column->bit_field |= SQB_COLUMN_CURRENT_ON_UPDATE;
}

void  sq_column_default(SqColumn *column, const char *default_value) {
	SQ_COLUMN_SET_DEFAULT(column, default_value);
}

void  sq_column_raw(SqColumn *column, const char *raw_property) {
	SQ_COLUMN_SET_RAW(column, raw_property);
}

#endif  // __STDC_VERSION__
