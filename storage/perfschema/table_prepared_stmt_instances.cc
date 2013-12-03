/* Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/**
  @file storage/perfschema/table_prepared_stmt_instances.cc
  Table EVENTS_STATEMENTS_SUMMARY_BY_PROGRAM (implementation).
*/

#include "my_global.h"
#include "my_pthread.h"
#include "pfs_instr_class.h"
#include "pfs_column_types.h"
#include "pfs_column_values.h"
#include "pfs_global.h"
#include "pfs_instr.h"
#include "pfs_timer.h"
#include "pfs_visitor.h"
#include "pfs_prepared_stmt.h"
#include "table_prepared_stmt_instances.h"

THR_LOCK table_prepared_stmt_instances::m_table_lock;

static const TABLE_FIELD_TYPE field_types[]=
{
  {
    { C_STRING_WITH_LEN("OBJECT_INSTANCE_BEGIN") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SQL_TEXT") },
    { C_STRING_WITH_LEN("longtext") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("OWNER_THREAD_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("OWNER_EVENT_ID") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("OWNER_OBJECT_TYPE") },
    { C_STRING_WITH_LEN("enum(\'EVENT\',\'FUNCTION\',\'PROCEDURE\',\'TABLE\',\'TRIGGER\')") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("OWNER_OBJECT_SCHEMA") },
    { C_STRING_WITH_LEN("varchar(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("OWNER_OBJECT_NAME") },
    { C_STRING_WITH_LEN("varchar(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("TIMER_PREPARE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("COUNT_EXECUTE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_TIMER_EXECUTE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("MIN_TIMER_EXECUTE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("AVG_TIMER_EXECUTE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("MAX_TIMER_EXECUTE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_LOCK_TIME") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_ERRORS") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_WARNINGS") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_ROWS_AFFECTED") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_ROWS_SENT") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_ROWS_EXAMINED") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_CREATED_TMP_DISK_TABLES") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_CREATED_TMP_TABLES") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SELECT_FULL_JOIN") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SELECT_FULL_RANGE_JOIN") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SELECT_RANGE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SELECT_RANGE_CHECK") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SELECT_SCAN") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SORT_MERGE_PASSES") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SORT_RANGE") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SORT_ROWS") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_SORT_SCAN") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_NO_INDEX_USED") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SUM_NO_GOOD_INDEX_USED") },
    { C_STRING_WITH_LEN("bigint(20)") },
    { NULL, 0}
  },
};

TABLE_FIELD_DEF
table_prepared_stmt_instances::m_field_def=
{ 32, field_types };

PFS_engine_table_share
table_prepared_stmt_instances::m_share=
{
  { C_STRING_WITH_LEN("prepared_statement_instances") },
  &pfs_truncatable_acl,
  table_prepared_stmt_instances::create,
  NULL, /* write_row */
  table_prepared_stmt_instances::delete_all_rows,
  NULL, /* get_row_count */
  1000, /* records */
  sizeof(PFS_simple_index),
  &m_table_lock,
  &m_field_def,
  false /* checked */
};

PFS_engine_table*
table_prepared_stmt_instances::create(void)
{
  return new table_prepared_stmt_instances();
}

int
table_prepared_stmt_instances::delete_all_rows(void)
{
  reset_prepared_stmt_instances();
  return 0;
}

table_prepared_stmt_instances::table_prepared_stmt_instances()
  : PFS_engine_table(&m_share, &m_pos),
    m_row_exists(false), m_pos(0), m_next_pos(0)
{}

void table_prepared_stmt_instances::reset_position(void)
{
  m_pos= 0;
  m_next_pos= 0;
}

int table_prepared_stmt_instances::rnd_next(void)
{
  PFS_prepared_stmt* prepared_stmt;

  if (prepared_stmt_array == NULL)
    return HA_ERR_END_OF_FILE;

  for (m_pos.set_at(&m_next_pos);
       m_pos.m_index < prepared_stmt_max;
       m_pos.next())
  {
    prepared_stmt= &prepared_stmt_array[m_pos.m_index];
    if (prepared_stmt->m_lock.is_populated())
    {
      make_row(prepared_stmt);
      m_next_pos.set_after(&m_pos);
      return 0;
    }
  }

  return HA_ERR_END_OF_FILE;
}

int
table_prepared_stmt_instances::rnd_pos(const void *pos)
{
  PFS_prepared_stmt* prepared_stmt;

  if (prepared_stmt_array == NULL)
    return HA_ERR_END_OF_FILE;

  set_position(pos);
  prepared_stmt= &prepared_stmt_array[m_pos.m_index];

  if (prepared_stmt->m_lock.is_populated())
  {
    make_row(prepared_stmt);
    return 0;
  }

  return HA_ERR_RECORD_DELETED;
}


void table_prepared_stmt_instances::make_row(PFS_prepared_stmt* prepared_stmt)
{
  pfs_optimistic_state lock;
  m_row_exists= false;

  prepared_stmt->m_lock.begin_optimistic_lock(&lock);

  m_row.m_sql_text_length= prepared_stmt->m_sqltext_length;
  if(m_row.m_sql_text_length > 0)
    memcpy(m_row.m_sql_text, prepared_stmt->m_sqltext,
           m_row.m_sql_text_length);

  /*Mayank TODO : Add code to copy remaining statistics values. */

  time_normalizer *normalizer= time_normalizer::get(statement_timer);
  /* Get prepared statement stats. */
  m_row.m_prepared_stmt_stat.set(normalizer, & prepared_stmt->m_prepared_stmt_stat);
  /* Get prepared statement execute stats. */
  m_row.m_prepared_stmt_execute_stat.set(normalizer, & prepared_stmt->m_prepared_stmt_execute_stat);

  if (! prepared_stmt->m_lock.end_optimistic_lock(&lock))
    return;

  m_row_exists= true;
}

int table_prepared_stmt_instances
::read_row_values(TABLE *table, unsigned char *buf, Field **fields,
                  bool read_all)
{
  Field *f;

  if (unlikely(! m_row_exists))
    return HA_ERR_RECORD_DELETED;

  /*
    Set the null bits.
  */
  DBUG_ASSERT(table->s->null_bytes == 1);
  buf[0]= 0;

  for (; (f= *fields) ; fields++)
  {
    if (read_all || bitmap_is_set(table->read_set, f->field_index))
    {
      switch(f->field_index)
      {
      case 1: /* SQL_TEXT */
        if(m_row.m_sql_text_length > 0)
          set_field_longtext_utf8(f, m_row.m_sql_text,
                                 m_row.m_sql_text_length);
        else
          f->set_null();
        break;
      case 0: /* Mayank TODO, OBJECT_INSTANCE_BEGIN */
      case 2: /* Mayank TODO, OWNER_THREAD_ID */
      case 3: /* Mayank TODO, OWNER_EVENT_ID */
      case 4: /* Mayank TODO, OWNER_OBJECT_TYPE */
      case 5: /* Mayank TODO, OWNER_OBJECT_SCHEMA */
      case 6: /* Mayank TODO, OWNER_OBJECT_NAME */
      case 7: /* Mayank TODO, TIMER_PREPARE */
        f->set_null();
        break;
      default: /* 8, ... COUNT/SUM/MIN/AVG/MAX */
        m_row.m_prepared_stmt_execute_stat.set_field(f->field_index - 8, f);
        break;
      }
    }
  }

  return 0;
}

