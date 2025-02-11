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

#include <SqConfig.h>
#include <SqError.h>
#include <SqEntry.h>
#include <SqType.h>

#define SQ_TYPE_N_ENTRY_DEFAULT    SQ_CONFIG_TYPE_N_ENTRY_DEFAULT

#ifdef _MSC_VER
#define strdup       _strdup
#define strcasecmp   _stricmp
#define strncasecmp  _strnicmp
#endif

// ----------------------------------------------------------------------------
// SqEntry

SqEntry *sq_entry_new(const SqType *type_info)
{
	SqEntry *entry;

	entry = malloc(sizeof(SqEntry));
	sq_entry_init(entry, type_info);
	return entry;
}

void  sq_entry_free(SqEntry *entry)
{
	if (entry->bit_field & SQB_DYNAMIC) {
		sq_entry_final(entry);
		free(entry);
	}
}

void  sq_entry_init(SqEntry *entry, const SqType *type_info)
{
	entry->name = NULL;
	entry->offset = 0;
	entry->bit_field = SQB_DYNAMIC;
	entry->type = (SqType*)type_info;
}

void  sq_entry_final(SqEntry *entry)
{
	if (entry->bit_field & SQB_DYNAMIC)
		free((char*)entry->name);
}

SqEntry *sq_entry_find(SqEntry *entry, const void *key, SqCompareFunc compareFunc)
{
	SqEntry **addr;

	addr = (SqEntry**)sq_type_find_entry(entry->type, key, compareFunc);
	if (addr)
		return *addr;
	else
		return NULL;
}

// ------------------------------------
// SqEntry SqCompareFunc

// This function is used by find(). Its actual parameter type:
//int sq_entry_cmp_str__name(const char *str, SqEntry   **entryAddr);
int   sq_entry_cmp_str__name(const void *str, const void *entryAddr)
{
	const char *name;

	name = (*(SqEntry**)entryAddr) ? (*(SqEntry**)entryAddr)->name : "";
#if SQ_CONFIG_ENTRY_NAME_CASE_SENSITIVE
	return strcmp(str, name);
#else
	return strcasecmp(str, name);
#endif
}

// This function is used by sort(). Its actual parameter type:
//int sq_entry_cmp_name(SqEntry   **entryAddr1, SqEntry   **entryAddr2);
int   sq_entry_cmp_name(const void *entryAddr1, const void *entryAddr2)
{
	const char *name1;
	const char *name2;

	name1 = (*(SqEntry**)entryAddr1) ? (*(SqEntry**)entryAddr1)->name : "";
	name2 = (*(SqEntry**)entryAddr2) ? (*(SqEntry**)entryAddr2)->name : "";
#if SQ_CONFIG_ENTRY_NAME_CASE_SENSITIVE
	return strcmp(name1, name2);
#else
	return strcasecmp(name1, name2);
#endif
}

// This function is used by find(). Its actual parameter type:
//int sq_entry_cmp_str__type_name(const char *str, SqEntry   **entryAddr);
int   sq_entry_cmp_str__type_name(const void *str, const void *entryAddr)
{
	const char *name;

	name = (*(SqEntry**)entryAddr) ? (*(SqEntry**)entryAddr)->type->name : "";
	return strcmp(str, name);
}

// This function is used by sort(). Its actual parameter type:
//int sq_entry_cmp_type_name(SqEntry   **entryAddr1, SqEntry   **entryAddr2);
int   sq_entry_cmp_type_name(const void *entryAddr1, const void *entryAddr2)
{
	const char *name1;
	const char *name2;

	name1 = (*(SqEntry**)entryAddr1) ? (*(SqEntry**)entryAddr1)->type->name : "";
	name2 = (*(SqEntry**)entryAddr2) ? (*(SqEntry**)entryAddr2)->type->name : "";
	return strcmp(name1, name2);
}

// ------------------------------------
// for internal use only
void  sq_entry_set_str_addr(SqEntry *entry, char **str_addr, const char *str_src)
{
	if (entry->bit_field & SQB_DYNAMIC) {
		free(*str_addr);
		if (str_src)
			*str_addr = strdup(str_src);
		else
			*str_addr = NULL;
	}
}

// ----------------------------------------------------------------------------
// SqReentry functions for SqPtrArray

int   sq_reentries_remove_null(void *reentry_ptr_array, int n_old_elements)
{
	SqPtrArray *array = (SqPtrArray*)reentry_ptr_array;
	int  index_src, index_dest;

	// find first NULL pointer
	for (index_dest = 0;  index_dest < array->length;  index_dest++) {
		if (array->data[index_dest] == NULL)
			break;
	}
	// move non-NULL pointer to overwrite NULL pointer
	n_old_elements -= 1;
	for (index_src = index_dest +1;  index_src < array->length;  index_src++) {
		if (n_old_elements == index_src)
			n_old_elements = index_dest;
		if (array->data[index_src])
			array->data[index_dest++] = array->data[index_src];
	}

	array->length = index_dest;
	return n_old_elements +1;
}

// ----------------------------------------------------------------------------
// If C compiler doesn't support C99 inline function.

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
// C99 or C++ inline functions has defined in SqEntry.h

#else   // __STDC_VERSION__
// define functions here if compiler does NOT support inline function.

void  sq_entry_set_name(SqEntry *entry, const char *name) {
	SQ_ENTRY_SET_NAME(entry, name);
}

#endif  // __STDC_VERSION__
