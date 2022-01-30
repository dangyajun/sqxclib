/*
 *   Copyright (C) 2021-2022 by C.H. Huang
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

#include <sqxclib.h>
#include <SqAppTool.h>

//#define TEST_ARGV

#ifdef TEST_ARGV
const char *test_argv[] = {
	"sqtool", "migrate",
//	"sqtool", "migrate", "--step", "testarg",
//	"sqtool", "migrate:install", "testarg",
//	"sqtool", "migrate:rollback", "--step", "testarg",
//	"sqtool", "make:migration", "create_companies_table",
//	"sqtool", "make:migration", "--table=companies", "alter_companies_table",
//	"sqtool",
};
const int   test_argc = sizeof(test_argv) / sizeof(char*);
#endif

int  main(int argc, char **argv)
{
	SqAppTool *apptool;

	apptool = malloc(sizeof(SqAppTool));
	sq_app_tool_init(apptool, "sqtool");

#ifdef TEST_ARGV
	argc = test_argc;
	argv = (char**)test_argv;
#endif

	sq_app_tool_run(apptool, argc, argv);

	sq_app_tool_final(apptool);
	free(apptool);

	return EXIT_SUCCESS;
}
