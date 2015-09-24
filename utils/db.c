#include "db.h"
#include <stdio.h>
#include <stdlib.h>

void error_print(
		SQLSMALLINT htype, /* A handle type identifier */
		SQLHANDLE   hndl,  /* A handle */
		SQLRETURN   frc,   /* Return code to be included with error msg  */
		int         line,  /* Used for output message, indcate where     */
		const char *file   /* the error was reported from  */
		)
{

	SQLCHAR     buffer[SQL_MAX_MESSAGE_LENGTH + 1] ;
	SQLCHAR     sqlstate[SQL_SQLSTATE_SIZE + 1] ;
	SQLINTEGER  sqlcode ;
	SQLSMALLINT length, i ;
	SQLINTEGER NumRecords;

	fprintf(stderr, ">--- ERROR -- RC = %d Reported from %s, line %d ------------\n",
			frc,
			file,
			line
			) ;
	SQLGetDiagField(htype, hndl, 0,SQL_DIAG_NUMBER, &NumRecords, SQL_IS_INTEGER,NULL);
	//kartik_cleanup
	//fprintf(stderr,"Total Number of diagnostic records: %d\n",NumRecords);
	fprintf(stderr,"Total Number of diagnostic records: %ld\n", (long int) NumRecords);
	//~kartik_cleanup
	i = 1 ;
	while ( SQLGetDiagRec( htype,
				hndl,
				i,
				sqlstate,
				&sqlcode,
				buffer,
				SQL_MAX_MESSAGE_LENGTH + 1,
				&length
				) == SQL_SUCCESS ) {
		fprintf(stderr, "         SQLSTATE: %s\n", sqlstate ) ;
		fprintf(stderr, "Native Error Code: %ld\n", (long int)sqlcode ) ;
		fprintf(stderr, "%s \n", buffer ) ;
		i++ ;
	}

	fprintf( stderr,">--------------------------------------------------\n" ) ;
}

struct db_info* init_db_info(
		const char *dsn_name,
		const char *dsn_user_name,
		const char *dsn_password
		)
{

	/* check if the input parameters are valid */
	if (dsn_name == NULL || dsn_user_name == NULL || dsn_password  == NULL) {
		fprintf(stderr, "Invalid dsn_name OR dsn_user_name OR dsn_password");
		return NULL;
	}

	/* Local variables */
	SQLRETURN retval = SQL_SUCCESS;

	/* Allocate an envoironment info */
	//unique_ptr<struct db_info> result(new struct db_info);

	struct db_info *result = (struct db_info*) malloc(sizeof(struct db_info));

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(result->env_handle));

	/*
	 * Check if we environment handle is valid and then proceed further
	 */
	if (result->env_handle == 0) {

		fprintf(stderr, "\nERROR SQLAllocHandle() failed");
		free(result);
		/* could not allocate the environment handle so return error */
		return NULL;
	}

	SQLSetEnvAttr(result->env_handle, SQL_ATTR_ODBC_VERSION,(SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);


	/* Allocate connection handle */
	SQLAllocHandle(SQL_HANDLE_DBC, result->env_handle, &(result->db_handle));

	/*
	 * Check if we could allocate the connection handle and then proceed
	 * further
	 */
	if (result->db_handle == 0) {

		/*
		 * could not allocate the connection handle so return error
		 */
		fprintf(stderr, "\nERROR SQLAllocHandle() failed");

		SQLFreeHandle(SQL_HANDLE_ENV, result->env_handle);
		free(result);
		return NULL;
	}

	/* connect to the specified datasource with given parameters */
	retval = SQLConnect(result->db_handle, (SQLCHAR *) dsn_name, SQL_NTS, (SQLCHAR *) dsn_user_name, SQL_NTS, (SQLCHAR *) dsn_password, SQL_NTS); 

	/* Check the connection was successfull opened */
	if (retval != SQL_SUCCESS) {
		error_print(SQL_HANDLE_DBC,result->db_handle, retval, __LINE__, __FILE__);
		if (result->db_handle != 0) {
			SQLFreeHandle(SQL_HANDLE_DBC, result->db_handle);
		}

		if (result->env_handle != 0) {
			SQLFreeHandle(SQL_HANDLE_ENV, result->env_handle);
		}

		free(result);
		return NULL;
	}

	result->dsn_name = dsn_name;
	result->dsn_user_name = dsn_user_name;
	result->dsn_password = dsn_password;

	return result;
}

void destroy_db_info(struct db_info* info)
{
	if (info == NULL) {
		return;
	}

	if (info->db_handle != 0) {
		SQLDisconnect(info->db_handle);
		SQLFreeHandle(SQL_HANDLE_DBC, info->db_handle);
	}

	if (info->env_handle != 0) {
		SQLFreeHandle(SQL_HANDLE_ENV, info->env_handle);
	}
	free(info);
}

struct db_info* reinit_db_info(
		struct db_info* info,
		const char *dsn_name,
		const char *dsn_user_name,
		const char *dsn_password
		)
{
	struct db_info *tmp = init_db_info(dsn_name, dsn_user_name, dsn_password);
	if (tmp != NULL) {
		destroy_db_info(info);
		info = tmp;
	}
	return info;
}
