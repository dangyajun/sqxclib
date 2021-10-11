/*
 *   Copyright (C) 2021 by C.H. Huang
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

#ifndef COMMAND_MIGRATE_H
#define COMMAND_MIGRATE_H

#include <SqCommand.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structue, macro, enumeration.

typedef struct CommandMigrate        CommandMigrate;

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

extern const SqCommandType           SqCommandType_Migrate_;

#define SQ_COMMAND_TYPE_MIGRATE    (&SqCommandType_Migrate_)

#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// C/C++ common definitions: define structue

/*	CommandMigrate: migrate command
 */

#ifdef __cplusplus
struct CommandMigrate : Sq::CommandMethod     // <-- 1. inherit C++ member function(method)
#else
struct CommandMigrate
#endif
{
	SQ_COMMAND_MEMBERS;                       // <-- 2. inherit member variable
/*	// ------ SqCommand members ------
	const SqCommandType  *type;

	// shortcuts is an array that sorted by SqOption.shortcut
	SqPtrArray     shortcuts;
	SqPtrArray     arguments;
 */

	// ------ CommandMigrate members ------   // <-- 3. Add variable and non-virtual function in derived struct.

	// options
	bool           step;

#ifdef __cplusplus
	// C++11 standard-layout
	CommandMigrate() {
		init(SQ_COMMAND_TYPE_MIGRATE);
	}
	~CommandMigrate() {
		final();
	}
#endif  // __cplusplus
};


#endif  // End of COMMAND_MIGRATE_H