#ifndef __DB_H__
#define __DB_H__

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUERY_SIZE (8192)

struct db_info {
	SQLHANDLE env_handle;
	SQLHANDLE db_handle;
	const char *dsn_name;
	const char *dsn_user_name;
	const char *dsn_password;
};

void error_print(
		SQLSMALLINT htype, /* A handle type identifier */
		SQLHANDLE   hndl,  /* A handle */
		SQLRETURN   frc,   /* Return code to be included with error msg  */
		int         line,  /* Used for output message, indcate where     */
		const char *file   /* the error was reported from  */
		);

struct db_info* init_db_info(
		const char *dsn_name,
		const char *dsn_user_name,
		const char *dsn_password
		);

void destroy_db_info(struct db_info* info);

struct db_info* reinit_db_info(
		struct db_info* info,
		const char *dsn_name,
		const char *dsn_user_name,
		const char *dsn_password
		);


#ifdef __cplusplus
}
#endif

#endif
