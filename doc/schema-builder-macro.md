[中文](schema-builder-macro.cn.md)

# Schema Builder (C macro)

This document introduce how to use C macro to define table dynamically.  

Define a C structured data type that mappings to your database table "users".

```c++
// If you use C language, please use 'typedef' to give a struct type a new name.
typedef struct  User    User;

struct User {
	int     id;          // primary key
	char   *name;
	char   *email;
	int     city_id;     // foreign key

	time_t  created_at;
};
```

use C macro to define table and column in schema_v1 (dynamic)  
  
macro SQ_SCHEMA_CREATE() can create table. The last parameter in macro is like lambda function.  
macro SQT_XXXX() is used to add column to table.  
macro SQC_XXXX() is used to set column properties.

```c
#include <sqxclib.h>
#include <SqSchema-macro.h>    // sqxclib.h doesn't contain special macros

	schema_v1 = sq_schmea_new("Ver 1");
	schema_v1->version = 1;    // specify version number or auto generate it

	// create table "users"
	SQ_SCHEMA_CREATE(schema_v1, "users", User, {
		// PRIMARY KEY
		SQT_INTEGER("id", User, id);  SQC_PRIMARY();

		// VARCHAR
		SQT_STRING("name", User, name, -1);

		// VARCHAR(60)
		SQT_STRING("email", User, email, 60);

		// DEFAULT CURRENT_TIMESTAMP
		SQT_TIMESTAMP("created_at", User, created_at);  SQC_USE_CURRENT();

		// FOREIGN KEY
		SQT_INTEGER("city_id", User, city_id);  SQC_REFERENCE("cities", "id");

		// CONSTRAINT FOREIGN KEY
		SQT_ADD_FOREIGN("users_city_id_foreign", "city_id");
			SQC_REFERENCE("cities", "id");  SQC_ON_DELETE("NO ACTION");  SQC_ON_UPDATE("NO ACTION"):

		// CREATE INDEX
		SQT_ADD_INDEX("users_id_index", "id");
	});
```

use C macro to change table and column in schema_v2 (dynamic)  
  
macro SQ_SCHEMA_ALTER() can alter table. The last parameter in macro is like lambda function.

```c
	schema_v2 = sq_schema_new("Ver 2");
	schema_v2->version = 2;    // specify version number or auto generate it

	// alter table "users"
	SQ_SCHEMA_ALTER(schema_v2, "users", User, {
		// add column to table
		SQT_INTEGER("test_add", User, test_add);

		// alter column in table
		SQT_INTEGER("city_id", User, city_id);  SQC_CHANGE();

		// DROP CONSTRAINT FOREIGN KEY
		SQT_DROP_FOREIGN("users_city_id_foreign");

		// drop column
		SQT_DROP("name");

		// rename column
		SQT_RENAME("email", "email2");
	});
```
