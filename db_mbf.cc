#include "db_mbf.h"
#include <memory>
#include <cstring>
#include <utility>
#include "glog/logging.h"
#include "utils/db.h"
#include "utils/util.h"


inline static void cleanup(SQLHANDLE handle)
{
	SQLFreeHandle(SQL_HANDLE_STMT, handle);
}

StringTable g_os_table(queryOSList);
StringTable g_browser_table(queryBrowserList);


// TODO(@ktk57): empty vector either means error or records in table
vector<pair<int, string>> ReadTableMBF(struct db_info **conn, const char *query)
{
	SQLHANDLE statement_handle = 0;
	SQLRETURN sql_retval = SQL_SUCCESS;
	SQLCHAR sql_statement[QUERY_SIZE + 1];

	SQLINTEGER s_pub_id = 0;
	SQLLEN cb_pub_id = 0;

	struct db_info *info = *conn;


	/* Allocate the statement info */
	SQLAllocHandle(SQL_HANDLE_STMT, info->db_handle, &statement_handle);

	/* Create SQL char string which contains the query */
	// TODO(@ktk57): use nstrcpy
	strcpy((char *) sql_statement, query);
	sql_statement[QUERY_SIZE] = '\0';


	vector<pair<int, string>> result;
	/* Create a prepared statement */
	sql_retval = SQLPrepare(statement_handle, sql_statement, SQL_NTS);
	if (sql_retval != SQL_SUCCESS) {
		//LOG_CRITICAL(SQL_PREPARE_FAILED, MOD_DEFAULT);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		return result;
	}

	/* Bind parameters : campaign_id */
	/*
		 sql_retval = SQLBindParameter(statement_handle, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
		 SQL_INTEGER, 0, 0, &s_campaign_id, 0, &cb_s_campaign_id);
		 if (sql_retval != SQL_SUCCESS) {
		 printf("\nError binding:\n");
		 error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval,__LINE__,__FILE__ );
		 retval = DB_ERROR_INTERNAL;
		 cleanup(statement_handle);
		 }
		 */
	//s_campaign_id = campaign_id;

	// Execute The SQL Statement
	sql_retval = SQLExecute(statement_handle);


	if (sql_retval != SQL_SUCCESS) {
		LOG(ERROR) << "error executing the query " << query;
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return result;
	}

	unique_ptr<SQLCHAR[]> ps_targeting_info(new SQLCHAR[MBF_TARGETING_JSON_SIZE + 1]);
	SQLLEN cb_targeting_info = 0;

	/* Bind Column : pub_id */
	sql_retval = SQLBindCol(statement_handle, 1, SQL_C_LONG, &s_pub_id, 0, &cb_pub_id);
	if (sql_retval != SQL_SUCCESS) {
		error_print((SQLSMALLINT)SQL_HANDLE_STMT, statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return result;
	}

	/* Bind Column :  targeting_info*/
	sql_retval = SQLBindCol(statement_handle, 2, SQL_C_CHAR, ps_targeting_info.get(), MBF_TARGETING_JSON_SIZE + 1, &cb_targeting_info);
	if (sql_retval != SQL_SUCCESS) {
		error_print((SQLSMALLINT)SQL_HANDLE_STMT, statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return result;
	}

	while (sql_retval != SQL_NO_DATA) {
		sql_retval = SQLFetch(statement_handle);
		if (sql_retval != SQL_NO_DATA) {
			//result.push_back(make_pair(s_pub_id, string{ps_targeting_info.get()}));
			// TODO(@ktk57): is move redundant?
			if (cb_targeting_info != SQL_NULL_DATA) {
				result.push_back(move(make_pair(s_pub_id, string(reinterpret_cast<const char*>(ps_targeting_info.get()), static_cast<string::size_type>(cb_targeting_info)))));
			}
		}
	}
	cleanup(statement_handle);
	return result;
}

vector<pair<int, double>>* ReadTablePF(struct db_info **conn, const char *query)
{
	SQLHANDLE statement_handle = 0;
	SQLRETURN sql_retval = SQL_SUCCESS;
	SQLCHAR sql_statement[QUERY_SIZE + 1];

	struct db_info *info = *conn;

	SQLINTEGER s_pub_id = 0;
	SQLLEN cb_pub_id = 0;

	SQLDOUBLE s_pf;
	SQLLEN cb_pf;


	/* Allocate the statement info */
	SQLAllocHandle(SQL_HANDLE_STMT, info->db_handle, &statement_handle);

	/* Create SQL char string which contains the query */
	// TODO(@ktk57): use nstrcpy
	strcpy((char *) sql_statement, query);
	sql_statement[QUERY_SIZE] = '\0';


	unique_ptr<vector<pair<int, double>>> result{new vector<pair<int, double>>};
	/* Create a prepared statement */

	sql_retval = SQLPrepare(statement_handle, sql_statement, SQL_NTS);
	if (sql_retval != SQL_SUCCESS) {
		//LOG_CRITICAL(SQL_PREPARE_FAILED, MOD_DEFAULT);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval,__LINE__,__FILE__ );
		cleanup(statement_handle);
		return nullptr;
	}

	/* Bind parameters : campaign_id */
	/*
		 sql_retval = SQLBindParameter(statement_handle, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
		 SQL_INTEGER, 0, 0, &s_campaign_id, 0, &cb_s_campaign_id);
		 if (sql_retval != SQL_SUCCESS) {
		 printf("\nError binding:\n");
		 error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval,__LINE__,__FILE__ );
		 retval = DB_ERROR_INTERNAL;
		 cleanup(statement_handle);
		 }
		 */
	//s_campaign_id = campaign_id;

	// Execute The SQL Statement
	sql_retval = SQLExecute(statement_handle);


	if (sql_retval != SQL_SUCCESS) {
		LOG(ERROR) << "error executing the query " << query;
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return nullptr;
	}

	sql_retval = SQLBindCol(statement_handle, 1, SQL_C_LONG, &s_pub_id, 0, &cb_pub_id);
	if (sql_retval != SQL_SUCCESS) {
		error_print((SQLSMALLINT)SQL_HANDLE_STMT, statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return nullptr;
	}

	/* Bind Column :  targeting_info*/
	sql_retval = SQLBindCol(statement_handle, 2, SQL_C_DOUBLE, &s_pf, 0, &cb_pf);

	if (sql_retval != SQL_SUCCESS) {
		error_print((SQLSMALLINT)SQL_HANDLE_STMT, statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return nullptr;
	}

	while (sql_retval != SQL_NO_DATA) {
		sql_retval = SQLFetch(statement_handle);
		if (sql_retval != SQL_NO_DATA) {
			//result.push_back(make_pair(s_pub_id, string{ps_output_dbl.get()}));
			// TODO(@ktk57): is move redundant?
			if (cb_pf != SQL_NULL_DATA) {
				result->push_back(move(make_pair(s_pub_id, s_pf)));
			}
		}
	}

#ifdef MBF_FUNCTIONAL_TESTING
	DLOG(INFO) << "Read the following from table";
	for (auto i = 0U; i != result->size(); i++) {
		DLOG(INFO) << (*result)[i].first << ", " << (*result)[i].second;
	}
#endif
	cleanup(statement_handle);
	return result.release();
}
