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

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#define snprintf	_snprintf
#endif

#include <stdio.h>      // snprintf

#include <SqConfig.h>
#include <SqError.h>
#include <Sqdb.h>

#define SQL_STRING_LENGTH_DEFAULT    SQ_CONFIG_SQL_STRING_LENGTH_DEFAULT

Sqdb*   sqdb_new(const SqdbInfo *info, SqdbConfig* config)
{
	void (*init)(Sqdb* db, SqdbConfig* conf);
	Sqdb* db;

	init = info->init;
	if (init) {
		db = malloc(info->size);
		info->init(db, config);
	}
	else
		db = calloc(1, info->size);
	db->info = info;
	return db;
}

void    sqdb_free(Sqdb* db)
{
	void (*final)(Sqdb* db);

	final = db->info->final;
	if (final)
		final(db);
	free(db);
}

// ----------------------------------------------------------------------------

void sqdb_sql_write_schema(Sqdb* db, SqBuffer* buffer, SqSchema* schema, SqPtrArray* arranged_tables)
{
	SqTable* table;

	if (arranged_tables == NULL)
		arranged_tables = sq_type_get_ptr_array(schema->type);
	for (int index = 0;  index < arranged_tables->length;  index++) {
		table = (SqTable*)arranged_tables->data[index];

		if (index > 0)
			sq_buffer_write_c(buffer, ' ');

		if (table->bit_field & SQB_CHANGED) {
			// ALTER TABLE
			sqdb_sql_alter_table(db, buffer, table, NULL);
		}
		else if (table->name == NULL) {
			// DROP TABLE
			sqdb_sql_drop_table(db, buffer, table);
		}
		else if (table->old_name && (table->bit_field & SQB_RENAMED) == 0) {
			// RENAME TABLE
			sqdb_sql_rename_table(db, buffer, table->old_name, table->name);
		}
		else {
			// CREATE TABLE
			if ((table->bit_field & SQB_TABLE_SQL_CREATED) == 0)
				sqdb_sql_create_tables_reo(db, buffer, schema, table);
			if (table->foreigns.length > 0)
				sqdb_sql_alter_table(db, buffer, table, &table->foreigns);
		}
	}
}

// ------------------------------------
// SqTable

// sort schema->type->entry by table->name before calling this function.
int  sqdb_sql_create_tables_reo(Sqdb* db, SqBuffer* buffer, SqSchema* schema, SqTable* table)
{
	SqColumn*   column;
	SqPtrArray* arranged;
	SqPtrArray  tmp_array = (SqPtrArray){NULL, 0};
	int         code = SQCODE_OK;

	// SQLite
	if (db->info->product == SQDB_PRODUCT_SQLITE && table->bit_field & SQB_TABLE_REO_CONSTRAINT) {
		// avoid infinite recursive
		table->bit_field |= SQB_TABLE_REO_CHECKING;
		// check constraint reference each other
		for (int index = 0;  index < table->foreigns.length;  index++) {
			column = table->foreigns.data[index];
			if (column->type == SQ_TYPE_CONSTRAINT) {
				SqTable* fore_table;

//				fore_table = sq_type_find_entry(schema->type, column->foreign->table, NULL);
				fore_table = sq_ptr_array_search(&schema->type->entry, column->foreign->table,
		                                         (SqCompareFunc)sq_entry_cmp_str__name);
				if (fore_table) {
					fore_table = *(SqTable**)fore_table;
					if (fore_table->bit_field & SQB_TABLE_SQL_CREATED)
						continue;
					if (fore_table->bit_field & SQB_TABLE_REO_CHECKING) {
						// error...
						// constraint reference each other
						code = SQCODE_REFERENCE_EACH_OTHER;
						break;
					}
				}
				else {
					// error...
					code = SQCODE_REFERENCE_NOT_FOUND;
					continue;
				}
				// recursive
				sqdb_sql_create_tables_reo(db, buffer, schema, fore_table);
				// remove current column in excluded list
				table->foreigns.data[index] = NULL;
			}
		}
		sq_reentries_remove_null(&table->foreigns, 0);
		// avoid infinite recursive
		table->bit_field &= ~SQB_TABLE_REO_CHECKING;
	}

	if (table->foreigns.length == 0)
		arranged = sq_type_get_ptr_array(table->type);
	else {
		sq_ptr_array_init(&tmp_array, 16, NULL);
		sq_table_exclude(table, &table->foreigns, &tmp_array);
		arranged = &tmp_array;
	}

	// move primary key to front of table and move constraint to end of table
	sq_ptr_array_sort(arranged, (SqCompareFunc)sq_column_cmp_attrib);
	// SQL create table
	sqdb_sql_create_table(db, buffer, table, arranged);
	table->bit_field |= SQB_TABLE_SQL_CREATED;

	if (tmp_array.data)
		sq_ptr_array_final(&tmp_array);

	return code;
}

// ----------------------------------------------------------------------------

int  sqdb_sql_create_table(Sqdb* db, SqBuffer* sql_buf, SqTable* table, SqPtrArray* arranged_columns)
{
	SqPtrArray  indexes;

	if (arranged_columns == NULL)
		arranged_columns = sq_type_get_ptr_array(table->type);

	sq_ptr_array_init(&indexes, 4, NULL);
	sq_table_get_columns(table, &indexes, SQ_TYPE_INDEX, 0);    // get record of "CREATE INDEX"
	// CREATE TABLE
	if (indexes.length < table->type->n_entry) {
		sq_buffer_write(sql_buf, "CREATE TABLE \"");
		sq_buffer_write(sql_buf, table->name);
		sq_buffer_write(sql_buf,"\" ");
		sqdb_sql_create_table_params(db, sql_buf, arranged_columns, -1);
	}

	// CREATE INDEX
	for (int i = 0;  i < indexes.length;  i++)
		sqdb_sql_create_index(db, sql_buf, table, (SqColumn*)indexes.data[i]);
	sq_ptr_array_final(&indexes);
	return SQCODE_OK;
}

int  sqdb_sql_create_table_params(Sqdb* db, SqBuffer* buffer, SqPtrArray* arranged_columns, int n_old_columns)
{
	SqColumn* column;
	int       index, count = 0;
	bool      has_constraint = false;

	sq_buffer_write(buffer, "( ");
	if (n_old_columns == -1)
		n_old_columns = arranged_columns->length;
	for (index = 0;  index < arranged_columns->length;  index++) {
		column = (SqColumn*)arranged_columns->data[index];
		if (column == NULL)
			continue;
		// skip ignore
//		if (column->bit_field & SQB_IGNORE)
//			continue;
		// skip "dropped" or "renamed" records
		if (column->old_name && (column->bit_field & SQB_RENAMED) == 0)
			continue;
		// skip INDEX
		if (column->type == SQ_TYPE_INDEX)
			continue;
		// skip CONSTRAINT
		if (column->type == SQ_TYPE_CONSTRAINT) {
			has_constraint = true;
			continue;
		}

		// write comma between two columns
		if (count > 0) {
			sq_buffer_alloc(buffer, 2);
			buffer->buf[buffer->writed -2] = ',';
			buffer->buf[buffer->writed -1] = ' ';
		}
		count++;
		// write column
		sqdb_sql_write_column(db, buffer, column);
#ifdef SQ_CONFIG_SQL_COLUMN_NOT_NULL_WITHOUT_DEFAULT
		// if column is newly "added" one
		if (index >= n_old_columns && (column->bit_field & SQB_CHANGED) == 0) {
			// Program can add default value if newly added column has "NOT NULL" without "DEFAULT".
			if ((column->bit_field & SQB_NULLABLE)==0 && column->default_value == NULL) {
				sq_buffer_write(buffer, " DEFAULT ");
				if (SQ_TYPE_IS_ARITHMETIC(column->type))
					sq_buffer_write(buffer, "0");
				else
					sq_buffer_write(buffer, "''");
			}
		}
#endif  // SQ_CONFIG_SQL_COLUMN_NOT_NULL_WITHOUT_DEFAULT
	}

//	if (db->info->product == SQDB_PRODUCT_MYSQL) {
		for (index = 0;  index < arranged_columns->length;  index++) {
			column = (SqColumn*)arranged_columns->data[index];
			// Don't output CONSTRAINT and INDEX here
			if (column->type == SQ_TYPE_CONSTRAINT || column->type == SQ_TYPE_INDEX)
				continue;
			// FOREIGN KEY
			if (column->foreign)    // || column->bit_field & SQB_FOREIGN
				sq_buffer_write(buffer, ", FOREIGN KEY");
			// PRIMARY KEY
			else if (column->bit_field & SQB_PRIMARY)
				sq_buffer_write(buffer, ", PRIMARY KEY");
			// UNIQUE
			else if (column->bit_field & SQB_UNIQUE)
				sq_buffer_write(buffer, ", UNIQUE");
			else
				continue;
			// ("column_name")
			sq_buffer_write(buffer, " (\"");
			sq_buffer_write(buffer, column->name);
			sq_buffer_alloc(buffer, 2);
			buffer->buf[buffer->writed -2] = '"';
			buffer->buf[buffer->writed -1] = ')';

			// FOREIGN KEY REFERENCES
			if (column->foreign)
				sqdb_sql_write_foreign_ref(db, buffer, column);
		}
//	}

	if (has_constraint) {
		for (index = 0;  index < arranged_columns->length;  index++) {
			column = (SqColumn*)arranged_columns->data[index];
			// DROP COLUMN
			if (column->name == NULL)
				continue;
			// CREATE INDEX
			if (column->type == SQ_TYPE_INDEX)
				continue;
			// CONSTRAINT
			if (column->type == SQ_TYPE_CONSTRAINT) {
				sq_buffer_write_c(buffer, ',');
				sqdb_sql_write_constraint(db, buffer, column);
			}
		}
	}

	sq_buffer_write(buffer, " );");
	return SQCODE_OK;
}

int  sqdb_sql_drop_table(Sqdb* db, SqBuffer* buffer, SqTable* table)
{
	// DROP TABLE "name";
	sq_buffer_write(buffer, "DROP TABLE \"");
	sq_buffer_write(buffer, table->old_name);
	sq_buffer_write(buffer, "\";");
	return SQCODE_OK;
}

int  sqdb_sql_rename_table(Sqdb* db, SqBuffer* buffer, const char* old_name, const char* new_name)
{
	// RENAME TABLE "old_name" TO "new_name";
	// ALTER TABLE "old_name" RENAME TO "new_name";
	if (db->info->product == SQDB_PRODUCT_MYSQL)
		sq_buffer_write(buffer, "RENAME TABLE \"");
	else
		sq_buffer_write(buffer, "ALTER TABLE \"");
	sq_buffer_write(buffer, old_name);
	sq_buffer_write_c(buffer, '"');

	if (db->info->product == SQDB_PRODUCT_MYSQL)
		sq_buffer_write(buffer, " TO \"");
	else
		sq_buffer_write(buffer, " RENAME TO \"");
	sq_buffer_write(buffer, new_name);
	sq_buffer_write(buffer, "\";");
	return SQCODE_OK;
}

int  sqdb_sql_alter_table(Sqdb* db, SqBuffer* buffer, SqTable* table, SqPtrArray* arranged_columns)
{
	SqColumn* column;
	int       index;
	bool      is_ok;

	if (arranged_columns == NULL)
		arranged_columns = sq_type_get_ptr_array(table->type);
	// ALTER TABLE
	for (index = 0;  index < arranged_columns->length;  index++) {
		column = (SqColumn*)arranged_columns->data[index];
//		if (column->bit_field & SQB_IGNORE)
//			continue;
		if (column->bit_field & SQB_CHANGED) {
			// ALTER COLUMN
			is_ok = sqdb_sql_alter_column(db, buffer, table, column);
		}
		else if (column->name == NULL) {
			// DROP COLUMN / CONSTRAINT / INDEX / KEY
			is_ok = sqdb_sql_drop_column(db, buffer, table, column);
		}
		else if (column->old_name && (column->bit_field & SQB_RENAMED) == 0) {
			// RENAME COLUMN
			is_ok = sqdb_sql_rename_column(db, buffer, table, column);
		}
		else {
			// ADD COLUMN / CONSTRAINT / INDEX / KEY
			is_ok = sqdb_sql_add_column(db, buffer, table, column);
		}

		if (is_ok)
			sq_buffer_write_c(buffer, ';');
	}
	return (is_ok) ? SQCODE_OK : SQCODE_NOT_SUPPORT;
}

// ------------------------------------
// column

int  sqdb_sql_add_column(Sqdb* db, SqBuffer* buffer, SqTable* table, SqColumn* column)
{
	// CREATE INDEX
	if (column->type == SQ_TYPE_INDEX) {
		sqdb_sql_create_index(db, buffer, table, column);
		return SQCODE_OK;
	}

	sq_buffer_write(buffer, "ALTER TABLE \"");
	sq_buffer_write(buffer, table->name);
	sq_buffer_write(buffer, "\" ");
	sq_buffer_write(buffer, "ADD ");

	// ADD CONSTRAINT
	if (column->type == SQ_TYPE_CONSTRAINT) {
		sqdb_sql_write_constraint(db, buffer, column);
		return SQCODE_OK;
	}
	// ADD FOREIGN KEY
	else if (column->foreign) {
		sq_buffer_write(buffer, "FOREIGN KEY");
	}
	// ADD PRIMARY KEY
	else if (column->bit_field & SQB_PRIMARY) {
		sq_buffer_write(buffer, "PRIMARY KEY");
	}
	// ADD UNIQUE
	else if (column->bit_field & SQB_UNIQUE) {
		sq_buffer_write(buffer, "UNIQUE");
	}
	// ADD COLUMN
	else {
		sqdb_sql_write_column(db, buffer, column);
		return SQCODE_OK;
	}

	// ("column_name")
	sq_buffer_write(buffer, " (\"");
	sq_buffer_write(buffer, column->name);
	sq_buffer_alloc(buffer, 2);
	buffer->buf[buffer->writed -2] = '"';
	buffer->buf[buffer->writed -1] = ')';

	// ADD FOREIGN KEY REFERENCES
	if (column->foreign)
		sqdb_sql_write_foreign_ref(db, buffer, column);

	return SQCODE_OK;
}

int  sqdb_sql_alter_column(Sqdb* db, SqBuffer* buffer, SqTable* table, SqColumn* column)
{
	sq_buffer_write(buffer, "ALTER TABLE \"");
	sq_buffer_write(buffer, table->name);
	sq_buffer_write(buffer, "\" ");

	if (db->info->column.use_alter) {
		// SQL Server / MS Access:
		sq_buffer_write(buffer, "ALTER COLUMN ");
		sqdb_sql_write_column(db, buffer, column);
	}
	else if (db->info->column.use_modify) {
		// My SQL / Oracle (prior version 10G):
		sq_buffer_write(buffer, "MODIFY COLUMN ");
		// Oracle 10G and later:
//		sq_buffer_write(buffer, "MODIFY ");
		sqdb_sql_write_column(db, buffer, column);
	}
	return SQCODE_OK;
}

int  sqdb_sql_rename_column(Sqdb* db, SqBuffer* buffer, SqTable* table, SqColumn* column)
{
	int  len;

	sq_buffer_write(buffer, "ALTER TABLE \"");
	sq_buffer_write(buffer, table->name);
	sq_buffer_write(buffer, "\" ");

	sq_buffer_write(buffer, "RENAME COLUMN ");
	len = snprintf(NULL, 0, "\"%s\" TO \"%s\"",
	               column->old_name, column->name);
	sprintf(sq_buffer_alloc(buffer, len), "\"%s\" TO \"%s\"",
	        column->old_name, column->name);
	return SQCODE_OK;
}

int  sqdb_sql_drop_column(Sqdb* db, SqBuffer* buffer, SqTable* table, SqColumn* column)
{
	if (column->type == SQ_TYPE_INDEX) {
		sqdb_sql_drop_index(db, buffer, table, column);
		return SQCODE_OK;
	}

	sq_buffer_write(buffer, "ALTER TABLE \"");
	sq_buffer_write(buffer, table->name);
	sq_buffer_write(buffer, "\" ");

	sq_buffer_write(buffer, "DROP ");
	// DROP CONSTRAINT
	if (column->type == SQ_TYPE_CONSTRAINT) {
		if (db->info->product == SQDB_PRODUCT_MYSQL) {
			if (column->bit_field & SQB_FOREIGN || column->foreign)
				sq_buffer_write(buffer, "FOREIGN KEY");
			else if (column->bit_field & SQB_PRIMARY)
				sq_buffer_write(buffer, "PRIMARY KEY");
			else if (column->bit_field & SQB_UNIQUE)
				sq_buffer_write(buffer, "INDEX");
		}
		else
			sq_buffer_write(buffer, "CONSTRAINT");
	}
	// DROP COLUMN
	else {
		sq_buffer_write(buffer, "COLUMN");
	}

	sq_buffer_write(buffer, " \"");
	sq_buffer_write(buffer, column->old_name);
	sq_buffer_write_c(buffer, '"');
	return SQCODE_OK;
}

void sqdb_sql_create_index(Sqdb* db, SqBuffer* sql_buf, SqTable* table, SqColumn* column)
{
	sq_buffer_write(sql_buf, "CREATE INDEX \"");
	sq_buffer_write(sql_buf, column->name);
	sq_buffer_write(sql_buf,"\" ON \"");
	sq_buffer_write(sql_buf, table->name);
	sq_buffer_write_c(sql_buf,'"');
	sqdb_sql_write_composite_columns(db, sql_buf, column);
	sq_buffer_write_c(sql_buf,';');
}

void sqdb_sql_drop_index(Sqdb* db, SqBuffer* sql_buf, SqTable* table, SqColumn* column)
{
	if (db->info->product == SQDB_PRODUCT_MYSQL) {
		sq_buffer_write(sql_buf, "ALTER TABLE \"");
		sq_buffer_write(sql_buf, table->name);
		sq_buffer_write(sql_buf, "\" ");
	}
	sq_buffer_write(sql_buf, "DROP INDEX \"");
	sq_buffer_write(sql_buf, column->name);

//	sq_buffer_write(sql_buf, "\" ON \"");
//	sq_buffer_write(sql_buf, table->name);
	sq_buffer_write(sql_buf, "\";");
}

void sqdb_sql_write_column(Sqdb* db, SqBuffer* buffer, SqColumn* column)
{
	SqType* type;
	int     len;
	int16_t size, digits;

	// "column_name"
	sq_buffer_write_c(buffer, '"');
	sq_buffer_write(buffer, column->name);
	sq_buffer_write(buffer, "\" ");
	type = column->type;
	if (SQ_TYPE_NOT_BUILTIN(type))
		type = SQ_TYPE_STRING;
	size = column->size;
	digits = column->digits;

	switch (SQ_TYPE_BUILTIN_INDEX(type)) {
	case SQ_TYPE_INDEX_BOOL:
		if (db->info->column.has_boolean)
			sq_buffer_write(buffer, "BOOLEAN");
		else
			sq_buffer_write(buffer, "TINYINT");
		break;

	case SQ_TYPE_INDEX_INT:
	case SQ_TYPE_INDEX_UINT:
	case SQ_TYPE_INDEX_INTPTR:
		sq_buffer_write(buffer, "INT");
		if (size > 0) {
			len = snprintf(NULL, 0, "(%d)", size);
			sprintf(sq_buffer_alloc(buffer, len), "(%d)", size);
		}
		if (column->type == SQ_TYPE_UINT)  // || column->type == SQ_TYPE_UINTPTR
			sq_buffer_write(buffer, " UNSIGNED");
		break;

	case SQ_TYPE_INDEX_INT64:
	case SQ_TYPE_INDEX_UINT64:
		sq_buffer_write(buffer, "BIGINT");
		if (size > 0) {
			len = snprintf(NULL, 0, "(%d)", size);
			sprintf(sq_buffer_alloc(buffer, len), "(%d)", size);
		}
		if (column->type == SQ_TYPE_UINT64)
			sq_buffer_write(buffer, " UNSIGNED");
		break;

	case SQ_TYPE_INDEX_TIME:
		sq_buffer_write(buffer, "TIMESTAMP");
		break;

	case SQ_TYPE_INDEX_DOUBLE:
		sq_buffer_write(buffer, "DOUBLE");    // FLOAT
		if (size > 0 || digits > 0) {
			sq_buffer_write_c(buffer, '(');
			if (size > 0 && digits == 0) {
				len = snprintf(NULL, 0, "%d", size);
				sprintf(sq_buffer_alloc(buffer, len), "%d", size);
			}
			else {
				len = snprintf(NULL, 0, "%d,%d", size, digits);
				sprintf(sq_buffer_alloc(buffer, len), "%d,%d", size, digits);
			}
			sq_buffer_write_c(buffer, ')');
		}
		break;

	case SQ_TYPE_INDEX_STRING:
		size = (size <= 0) ? SQL_STRING_LENGTH_DEFAULT : size;
		len = snprintf(NULL, 0, "VARCHAR(%d)", size);
		sprintf(sq_buffer_alloc(buffer, len), "VARCHAR(%d)", size);
		break;
	}

	// "AUTOINCREMENT"
	if (column->bit_field & SQB_AUTOINCREMENT)
		sq_buffer_write(buffer, " AUTOINCREMENT");
	// "NOT NULL"
	if ((column->bit_field & SQB_NULLABLE) == 0)
		sq_buffer_write(buffer, " NOT NULL");
/*
	if (db->info->product != SQDB_PRODUCT_MYSQL) {
		// "FOREIGN KEY"
		if (column->foreign) {
			sq_buffer_write(buffer, " FOREIGN KEY");
			sqdb_sql_write_foreign_ref(db, buffer, column);
		}
		// "PRIMARY KEY"
		else if (column->bit_field & SQB_PRIMARY)
			sq_buffer_write(buffer, " PRIMARY KEY");
		// "UNIQUE"
		else if (column->bit_field & SQB_UNIQUE)
			sq_buffer_write(buffer, " UNIQUE");
	}
 */
	if (column->default_value) {
		sq_buffer_write(buffer, " DEFAULT ");
		sq_buffer_write(buffer, column->default_value);
	}

	// raw SQL
	if (column->extra) {
		sq_buffer_write_c(buffer, ' ');
		sq_buffer_write(buffer, column->extra);
	}
}

void sqdb_sql_write_constraint(Sqdb* db, SqBuffer* buffer, SqColumn* column)
{
	// CONSTRAINT "column name" PRIMARY KEY ("col1", "col2")
	sq_buffer_write(buffer, " CONSTRAINT \"");
	sq_buffer_write(buffer, column->name);
	sq_buffer_write_c(buffer, '"');

	if (column->foreign)
		sq_buffer_write(buffer, " FOREIGN KEY");
	else if (column->bit_field & SQB_PRIMARY)
		sq_buffer_write(buffer, " PRIMARY KEY");
	else if (column->bit_field & SQB_UNIQUE)
		sq_buffer_write(buffer, " UNIQUE");

	sqdb_sql_write_composite_columns(db, buffer, column);

	// FOREIGN KEY REFERENCES
	if (column->foreign)
		sqdb_sql_write_foreign_ref(db, buffer, column);
}

void sqdb_sql_write_composite_columns(Sqdb* db, SqBuffer* sql_buf, SqColumn* column)
{
	char** element;

	sq_buffer_write(sql_buf, " (\"");
	for (element = column->composite;  *element;  element++) {
		if (element != column->composite)
			sq_buffer_write(sql_buf, ",\"");
	 	sq_buffer_write(sql_buf, *element);
		sq_buffer_write_c(sql_buf, '"');
	}
	sq_buffer_write_c(sql_buf, ')');
}

void sqdb_sql_write_foreign_ref(Sqdb* db, SqBuffer* buffer, SqColumn* column)
{
	// REFERENCES "foreign table name"("foreign column name")
	sq_buffer_write(buffer, " REFERENCES \"");
	sq_buffer_write(buffer, column->foreign->table);    // foreign table name
	sq_buffer_write(buffer, "\"(\"");
	sq_buffer_write(buffer, column->foreign->column);   // foreign column name
	sq_buffer_alloc(buffer, 2);
	buffer->buf[buffer->writed -2] = '"';
	buffer->buf[buffer->writed -1] = ')';
	// ON DELETE
	if (column->foreign->on_delete) {
		sq_buffer_write(buffer, " ON DELETE ");
		sq_buffer_write(buffer, column->foreign->on_delete);
	}
	// ON UPDATE
	if (column->foreign->on_update) {
		sq_buffer_write(buffer, " ON UPDATE ");
		sq_buffer_write(buffer, column->foreign->on_update);
	}
}

void  sqdb_sql_write_column_list(Sqdb* db, SqBuffer* sql_buf, SqPtrArray* arranged_columns,
                                 int n_old_columns, bool old_name)
{
	SqColumn* column;
	char* allocated;
	int   count = 0;

	if (n_old_columns == -1)
		n_old_columns = arranged_columns->length;
	for (int index = 0;  index < arranged_columns->length;  index++) {
		column = (SqColumn*)arranged_columns->data[index];
		// skip ignore
//		if (column->bit_field & SQB_IGNORE)
//			continue;
		// skip CONSTRAINT and INDEX
		if (column->type == SQ_TYPE_CONSTRAINT || column->type == SQ_TYPE_INDEX)
			continue;
		// skip "dropped" or "renamed" records
		if (column->old_name && (column->bit_field & SQB_RENAMED) == 0)
			continue;
		// skip if column is newly "added" one
		if (index >= n_old_columns && (column->bit_field & SQB_CHANGED) == 0)
			continue;

		// write comma between two columns
		if (count > 0) {
			allocated = sq_buffer_alloc(sql_buf, 2);
			allocated[0] = ',';
			allocated[1] = ' ';
		}
		count++;
		// write column->old_name or column->name
		sq_buffer_write_c(sql_buf, '\"');
		if (old_name && column->old_name) {
			sq_buffer_write(sql_buf, column->old_name);
			if (column->bit_field & SQB_DYNAMIC) {
				free(column->old_name);
				column->old_name = NULL;
			}
		}
		else
			sq_buffer_write(sql_buf, column->name);
		sq_buffer_write_c(sql_buf, '\"');
	}
}
