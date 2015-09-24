#include <cstring>
#include <cassert>
#include <algorithm>
#include <memory>
#include "util.h"
//#include "cmph.h"
#include "json.h"

inline static void cleanup(SQLHANDLE handle)
{
	SQLFreeHandle(SQL_HANDLE_STMT, handle);
}

/*
void init_vec_str(struct vec_str *vs, const char **buf, int size)
{
	vs->buf = buf;
	vs->size = size;
	vs->pos = 0;
}
*/

uint32_t mfloor(uint32_t dividend, uint32_t divisor)
{
	assert(divisor != 0);
	if (dividend == 0) {
		return 0;
	}
	return dividend / divisor;
}

// returns ceil(dividend/divisor)
uint32_t mceil(uint32_t dividend, uint32_t divisor)
{
	assert(divisor != 0);
	if (dividend == 0) {
		return 0;
	}
	return (dividend - 1)/divisor + 1;
}

/*
int vec_str_read(void *data, char **key, cmph_uint32 *keylen)
{
	struct vec_str *vs = (struct vec_str*) data;

	int pos = vs->pos;

	int size = vs->size;

	if (pos >= size) {
		fprintf(stderr, "\nERROR pos > size i.e %d > %d %s:%d\n", pos, size, __FILE__, __LINE__);
		return 0;
	}

	*key = strdup(vs->buf[pos]);

	if (*key == NULL) {
		fprintf(stderr, "\nERROR strdup failed for %s %s:%d\n", vs->buf[pos], __FILE__,
				__LINE__);
		return 0;
	}

	*keylen = strlen(vs->buf[pos]);

	return (int)(*keylen);
}

void vec_str_dispose(void *data, char *key, cmph_uint32 keylen)
{
	(void) data;
	(void) key;
	(void) keylen;

  free(key);
}

void vec_str_rewind(void *data)
{
	struct vec_str *vs = (struct vec_str*) data;
	vs->pos = 0;
}

*/
json_object *getFromObject (
		json_object *obj,
		const char *key,
		json_type type
		)
{

	json_object *result = NULL;

	/*
	 * Check function parameters.
	 */
	if ((obj == NULL) || (key == NULL) || (json_type_object != json_object_get_type(obj))){
		//fprintf(stderr, "\nERROR Invalid parameters : %s:%s:%d\n", __func__, __FILE__, __LINE__);
		LOG(ERROR) << "Invalid parameters to getFromObject()";
		return NULL;
	}

	result = json_object_object_get(obj, key);
	if (result == NULL) {

		DLOG(WARNING) << "Key " << key << " not present in json object";
		return result;
	}

	/*
	 * Now check object type.
	 */
	if (json_object_is_type(result, type) == 0) {

		DLOG(ERROR) << "Invalid JSON type fetched for key " << key;
		return NULL;
	}

	return result;
}

std::string itoa(uint32_t n, uint8_t base, int min_len)
{
	if (base < 2 || base > 36) {
		return "";
	}
	std::string s("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	std::string result;
	while (n >= base) {
		result += s[n%base];
		n /= base;
	}
	result += s[n];
	auto sz = result.size();
	if (min_len > 0 && static_cast<int>(sz) < min_len) {
		std::string tmp;
		int count = min_len - static_cast<int>(sz);
		for (int i = 0; i < count; i++) {
			tmp += "0";
		}
		result += tmp;
	}
	std::reverse(result.begin(), result.end());
	return result;
}

vector<string>* ReadStringFromTable(struct db_info **conn, const char *query)
{
	SQLHANDLE statement_handle = 0;
	SQLRETURN sql_retval = SQL_SUCCESS;
	SQLCHAR sql_statement[QUERY_SIZE + 1];

	struct db_info *info = *conn;


	/* Allocate the statement info */
	SQLAllocHandle(SQL_HANDLE_STMT, info->db_handle, &statement_handle);

	/* Create SQL char string which contains the query */
	// TODO(@ktk57): use nstrcpy
	strcpy((char *) sql_statement, query);
	sql_statement[QUERY_SIZE] = '\0';


	unique_ptr<vector<string>> result{new vector<string>};
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
		fprintf(stderr, "\nError executing select statement, %s:%d\n",__FILE__,__LINE__);
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle,
				sql_retval,__LINE__,__FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return nullptr;
	}

	unique_ptr<SQLCHAR[]> ps_output_str(new SQLCHAR[MAX_DB_STRING + 1]);
	SQLLEN cb_output_len = 0;

	/* Bind Column :  targeting_info*/
	sql_retval = SQLBindCol(statement_handle, 1, SQL_C_CHAR, ps_output_str.get(), MAX_DB_STRING + 1, &cb_output_len);
	if (sql_retval != SQL_SUCCESS) {
		error_print((SQLSMALLINT)SQL_HANDLE_STMT, statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return nullptr;
	}

	while (sql_retval != SQL_NO_DATA) {
		sql_retval = SQLFetch(statement_handle);
		if (sql_retval != SQL_NO_DATA) {
			//result.push_back(make_pair(s_pub_id, string{ps_output_str.get()}));
			// TODO(@ktk57): is move redundant?
			if (cb_output_len != SQL_NULL_DATA) {
				result->push_back(move(string(reinterpret_cast<const char*>(ps_output_str.get()), static_cast<string::size_type>(cb_output_len))));
			}
		}
	}
#ifdef MBF_FUNCTIONAL_TESTING
	DLOG(INFO) << "Read the following from table";
	for (auto i = 0U; i != result->size(); i++) {
		DLOG(INFO) << (*result)[i];
	}
#endif
	cleanup(statement_handle);
	return result.release();
}

uint32_t LastUpdateTimeStamp(struct db_info **conn, const char *table, const char *column)
{
	SQLHANDLE statement_handle = 0;
	SQLRETURN sql_retval = SQL_SUCCESS;
	SQLCHAR sql_statement[QUERY_SIZE + 1];

	SQLUINTEGER s_update_time;
	SQLLEN cb_update_time;

	struct db_info *info = *conn;


	/* Allocate the statement info */
	SQLAllocHandle(SQL_HANDLE_STMT, info->db_handle, &statement_handle);

	/* Create SQL char string which contains the query */
	// TODO(@ktk57): use nstrcpy
	string query("select UNIX_TIMESTAMP(max(");
	query += column;
	query += ")) from ";
	query += table;
	DLOG(INFO) << "Query = " << query.c_str();
	strcpy((char *) sql_statement, query.c_str());
	sql_statement[QUERY_SIZE] = '\0';


	uint32_t result = 0U;
	/* Create a prepared statement */
	sql_retval = SQLPrepare(statement_handle, sql_statement, SQL_NTS);
	if (sql_retval != SQL_SUCCESS) {
		//LOG_CRITICAL(SQL_PREPARE_FAILED, MOD_DEFAULT);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle, sql_retval,__LINE__,__FILE__ );
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
		fprintf(stderr, "\nError executing select statement, %s:%d\n",__FILE__,__LINE__);
		error_print((SQLSMALLINT)SQL_HANDLE_STMT,statement_handle,
				sql_retval,__LINE__,__FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return result;
	}


	/* Bind Column :  targeting_info*/
	sql_retval = SQLBindCol(statement_handle, 1, SQL_C_ULONG, &s_update_time, 0, &cb_update_time);
	if (sql_retval != SQL_SUCCESS) {
		error_print((SQLSMALLINT)SQL_HANDLE_STMT, statement_handle, sql_retval, __LINE__, __FILE__ );
		cleanup(statement_handle);
		*conn = reinit_db_info(info, info->dsn_name, info->dsn_user_name, info->dsn_password);
		return result;
	}

	while (sql_retval != SQL_NO_DATA) {
		sql_retval = SQLFetch(statement_handle);
		if (sql_retval != SQL_NO_DATA) {
			//result.push_back(make_pair(s_pub_id, string{ps_output_str.get()}));
			// TODO(@ktk57): is move redundant?
			if (cb_update_time != SQL_NULL_DATA) {
				result = s_update_time;
				break;
			}
		}
	}
#ifdef MBF_FUCNTIONAL_TESTING
	DLOG(INFO) << "Update timestamp = " << result << " and time = " << time(0);
#endif
	cleanup(statement_handle);
	return result;
}

int WriteBufToFile(
		const char *path,
		const uint8_t *buf,
		int len
		)
{
	DCHECK(path != NULL && buf != NULL);

	int rc = 0;

	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		LOG(ERROR) << "fopen() for " << path << " failed";
		return -1;
	}

	size_t bytes_written = fwrite(buf, sizeof(uint8_t), len, fp);
	if (bytes_written != (size_t) len) {
		fclose(fp);
		LOG(ERROR) << "fwrite() failed";
		return -1;
	}

	rc = fclose(fp);
	if (rc < 0) {
		LOG(ERROR) << "fclose() failed with errno = " << errno;
		return -1;
	}

	return rc;
}

int WriteBufToFile(
		const char *path,
		const string& buf
		)
{
	DCHECK(path != NULL);
	if (buf.empty()) {
		LOG(WARNING) << "buf is empty";
		return 0;
	}

	int rc = 0;

	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		LOG(ERROR) << "fopen() for " << path << " failed";
		return -1;
	}

	size_t bytes_written = fwrite(buf.c_str(), sizeof(uint8_t), buf.size(), fp);
	if (bytes_written != buf.size()) {
		fclose(fp);
		LOG(ERROR) << "fwrite() failed, bytes written = " << bytes_written << " and bytes_to_write = " << buf.size();
		return -1;
	}

	rc = fclose(fp);
	if (rc < 0) {
		LOG(ERROR) << "fclose() failed with errno = " << errno;
		return -1;
	}

	return rc;
}


// Gets the size of an "opened" file
long FileSize(FILE* is)
{
	// File must be opened for read
	int rc = 0;
	rc = fseek(is, 0L, SEEK_END);
	if (rc < 0) {
		LOG(ERROR) << "fseek() failed with errno = " << errno;
		return -1;
	}

	long size = ftell(is);
	if  (size < 0) {
		LOG(ERROR) << "ftell() failed with errno = " << errno;
		return -1;
	}

	/*
	 * Reset
	 */
	rc = fseek(is, 0L, SEEK_SET);
	if (rc < 0) {
		LOG(ERROR) << "fseek() failed with errno = " << errno;
		return -1;
	}
	return size;
}
/*
 * Reads a file given by "path" to buf
 */
#if 0
string ReadBufFromFile(const char *path)
{
	DCHECK(path != NULL);
	int rc = 0;
	FILE *is = fopen(path, "r");
	if (is == NULL) {
		LOG(ERROR) << "fopen() for " << path;
		return string();
	}

	/*
	 * Find size of the file
	 */
	long size = FileSize(is);
	if (size <= 0) {
		LOG(ERROR) << "FileSize() = " << size;
		fclose(is);
		return string();
	}

	string result('\0', size);

	size_t bytes_read = fread(temp, sizeof(uint8_t), size, is);
	if (bytes_read != (size_t) size) {
		fprintf(stderr, "\nERROR fread() failed bytes read = %lu size of file = %ld %s:%d\n", bytes_read, size, __FILE__, __LINE__);
		free(temp);
		return -1;
	}
	rc = fclose(is);
	if (rc < 0) {
		fprintf(stderr, "\nERROR fclose() failed with errno = %d %s:%d\n", errno, __FILE__, __LINE__);
		free(temp);
		return -1;
	}

	if (add_nul_char) {
		temp[size] = '\0';
	}

	*buf = temp;
	*len = add_nul_char?size + 1:size;
	return rc;
}
#endif


/*
 * returns the # of token in str delimited by "delimiter"
 * <str><delimiter><str> returns 2
 * <str><delimiter>returns 2
 * <delimiter><str> returns 2
 * <delimiter><str><delimiter> returns 3
 * <str> returns 1
 */
int CountToken(const char *str, char delimiter)
{
	DCHECK(str != NULL);

	if (str == NULL || str[0] == '\0') {
		return 0;
	}

	int counter = 0;
	//const char *temp = str;
	const char *s = str;
	while (true) {
		counter++;
		s = strchr(s, delimiter);
		if (s == NULL) {
			break;
		}
		s = s + 1;
	}
	return counter;
}

int ReadFileToBuf(const char *path, uint8_t **buf, int *len, bool add_nul_char)
{
	DCHECK(path != NULL);
	*len = 0;
	int rc = 0;
	FILE *is = fopen(path, "r");
	if (is == NULL) {
		LOG(ERROR) << "fopen() failed for " << path;
		return -1;
	}

	long size = FileSize(is);
	if (size <= 0) {
		LOG(ERROR) << "FileSize() = " << size;
		fclose(is);
		return -1;
	}

	uint8_t *temp = (uint8_t*) malloc(add_nul_char?size + 1:size);

	if (temp == NULL) {
		LOG(ERROR) << "malloc() failed for " << ((add_nul_char)?size + 1:size);
		return -1;
	}

	size_t bytes_read = fread((void*) temp, sizeof(uint8_t), size, is);
	if (bytes_read != (size_t) size) {
		LOG(ERROR) << "fread() failed, bytes read = " << bytes_read << " bytes to read " << size;
		free(temp);
		return -1;
	}

	rc = fclose(is);
	if (rc < 0) {
		LOG(ERROR) << "fclose() failed with errno = " << errno;
		free(temp);
		return -1;
	}

	if (add_nul_char) {
		temp[size] = '\0';
	}

	*buf = temp;
	*len = add_nul_char?size + 1:size;
	return rc;
}
