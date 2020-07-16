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

#ifndef SQ_JOINT_H
#define SQ_JOINT_H

#include <SqTable.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SqField           SqJoint;

/* ----------------------------------------------------------------------------
	SqJoint - multiple field and instance (SQL joined table)

	SqField
	|
	`--- SqJoint

	if (joint->type->map_length > 1)

	SELECT 'T01'.'column' AS 'T01.column',
	       'T02'.'column' AS 'T02.column'
	FROM table1 AS T01
	JOIN table2 AS T02 ON T02.table1_id = T01.id
	WHERE T01.column > 100

	sq_joint_add(joint, table1, "T01");
	sq_joint_add(joint, table2, "T02");


	SELECT 'table1'.'column' AS 'table1.column',
	       'table2'.'column' AS 'table2.column'
	FROM table1
	JOIN table2 ON table2.table1_id = table1.id
	WHERE table1.column > 100

	sq_joint_add(joint, table1, NULL);
	sq_joint_add(joint, table2, NULL);
 */

SqJoint* sq_joint_new();
void     sq_joint_free(SqJoint* joint);

void sq_joint_add(SqJoint* joint, SqTable* table, const char* as_table_name);


#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// C++ namespace

#ifdef __cplusplus

namespace Sq
{
// These are for directly use only. You can NOT derived it.
typedef struct SqField          Joint;
};  // namespace Sq

#endif  // __cplusplus

#endif  // SQ_JOINT_H
