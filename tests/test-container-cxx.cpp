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

#include <stdio.h>
#include <type_traits>  // is_standard_layout<>
#include <iostream>     // cout

#include <SqPtrArray.h>
#include <SqBuffer.h>

using namespace std;

// ----------------------------------------------------------------------------

void test_intptr_array()
{
	Sq::IntptrArray* array;
	intptr_t   intarray[] = {0, 3, 2, 5};

	array = new Sq::IntptrArray();
	array->insert(0, intarray, 4);

	sq_ptr_array_alloc_at(array, 2, 2);
	array->data[2] = 301;
	array->data[3] = 351;

	sq_ptr_array_foreach(array, element) {
		printf(" - %d", (intptr_t)element);
	}
	puts("");

	sq_ptr_array_free(array);
}

void test_ptr_array_vp()
{
	SqPtrArray  array;

	// array pointer stay the same
	sq_ptr_array_init(&array, 4, NULL);

	array.data[array.length++] = (void*)(intptr_t)5;
	array.data[array.length++] = (void*)(intptr_t)6;
	array.data[array.length++] = (void*)(intptr_t)7;
	array.data[array.length++] = (void*)(intptr_t)8;

	sq_ptr_array_erase(&array, 2, 1);
	sq_ptr_array_append(&array, (void*)(intptr_t)12);
	sq_ptr_array_insert(&array, 2, (void*)(intptr_t)105);
	sq_ptr_array_insert(&array, 3, (void*)(intptr_t)0);

	// this will remove index 3
	sq_ptr_array_remove_null(&array);

	sq_ptr_array_foreach(&array, element) {
		printf(" - 0x%p", element);
	}
	puts("");

	// array pointer stay the same
	sq_ptr_array_alloc(&array, 16);
	sq_ptr_array_final(&array);
}

void test_ptr_array()
{
	test_intptr_array();
	test_ptr_array_vp();
}

// ----------------------------------------------------------------------------

void test_buffer(void)
{
	SqBuffer* buf;
	int       len;

	buf = sq_buffer_new();

	len = snprintf(NULL, 0, "%d", 13254);
	sprintf(sq_buffer_alloc(buf, len), "%d", 13254);

//	sq_buffer_insert(buf, 2, "xyz");
	sq_buffer_insert_n(buf, 2, "xyz", 3);
//	sq_buffer_write(buf, "abc");
	sq_buffer_write_n(buf, "abc", 3);
	sq_buffer_write_c(buf, 'g');
	sq_buffer_insert_c(buf, 3, 'q');

	sq_buffer_write_c(buf, 0);
	puts(buf->buf);
	sq_buffer_free(buf);
}

// ----------------------------------------------------------------------------

int main(void)
{
	test_ptr_array();
	test_buffer();

//	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	return 0;
}
