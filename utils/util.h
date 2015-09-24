#ifndef __UTIL_H__
#define __UTIL_H__
#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>
#include <memory>
#if __cplusplus >= 201103L
#include <atomic>
#else
#include <cstdatomic>
#endif
#include "db.h"
#include "glog/logging.h"

/*
#ifdef __cplusplus 
extern "C"
{
#endif
}
*/
#include "json.h"

#ifdef S_PRINTF
#undef S_PRINTF
#define S_PRINTF(bytes_written, char_ptr, space_left, format, err_fun, return_or_not, return_code ...)   \ do { \
	bytes_written = snprintf(char_ptr, space_left, format, ##__VA_ARGS__);    \
	if (bytes_written >= space_left) {                      \
		err_fun(); \
		if (return_or_not) { \
			return return_code; \
		} \
	} else { \
		space_left -= bytes_written;                        \
		char_ptr += bytes_written; \
	} \
} while(0);
#endif


#if __cplusplus >= 201103L
#else
const                        // this is a const object...
class {
	public:
		template<class T>          // convertible to any type
			operator T*() const      // of null non-member
			{ return 0; }            // pointer...
		template<class C, class T> // or any type of null
			operator T C::*() const  // member pointer...
			{ return 0; }
	private:
		void operator&() const;    // whose address can't be taken
} nullptr = {};              // and whose name is nullptr
#endif

using namespace std;

const int MAX_DB_STRING = 1024;

vector<string>* ReadStringFromTable(struct db_info **conn, const char *query);

class BadJSON : public runtime_error {

	public:
		BadJSON(const string& msg):runtime_error(msg) {
		}
		BadJSON(const char* msg):runtime_error(msg) {
		}
};

class StringTable {

	private:

		atomic_char idx_inuse_;
		uint8_t padding_[64 - sizeof(idx_inuse_)];
		string query_;
		vector<string> *table_[2];

		// TODO(@ktk57): add padding for cache line
		// timestamp
		bool tableModified(struct db_info **conn)
		{
			(void) conn;
			return true;
		}

	public:
		StringTable(const StringTable&) = delete;
		StringTable& operator=(const StringTable&) = delete;

		StringTable(const string& q): idx_inuse_{-1}, query_(q), table_{nullptr, nullptr}  {}

		// to be used by a single thread running in infinite loop waking up after configurable amount of seconds
		void Update(struct db_info **conn)
		{
			int8_t inuse = idx_inuse_.load();
			if (inuse == -1) {
				vector<string> *tmp = ReadStringFromTable(conn, query_.c_str());
				if (tmp == nullptr) {
					return;
				}
				table_[0] = tmp;
				idx_inuse_.store(0);
				return;
			}
			//inuse == 0 OR 1

			auto flag = tableModified(conn);

			if (flag) {
				vector<string> *tmp = ReadStringFromTable(conn, query_.c_str());
				if (tmp == nullptr) {
					return;
				}
				int8_t not_inuse = (inuse == 0)?1:0;
				delete table_[not_inuse];
				table_[not_inuse] = tmp;
				idx_inuse_.store(not_inuse);
			}
		}

		// to be used by multiple threads
		const char* Find(const char *input)
		{

			DCHECK(input != NULL);
			if (input == NULL) {
				return NULL;
			}

			int8_t i = idx_inuse_.load();
			if (i < 0) {
				LOG(ERROR) << "idx_inuse_ = " << idx_inuse_ << ", probably the update thread hasn't even run once yet, need to add sleep in main()";
				return NULL;
			}

			const vector<string> *p = table_[i];

			for (auto j = 0U; j!= p->size(); j++) {
				const char *result = (*p)[j].c_str();

				if (strstr(input, result) != NULL) {
					return result;
				}
			}
			return NULL;
		}
};

struct vec_str {
	const char **buf;
	int pos;
	int size;
};

	/*
		 buf = (char**) malloc(sizeof(char*) * capacity);
	// No error handling
	for (int i = 0; i < capacity; i++) {
	buf[i] = NULL;
	}
	v->buf = NULL;
	v->size = 0;
	v->capacity = 0;
	}
	*/

json_object *getFromObject(json_object *obj, const char *key, json_type type);

uint32_t mfloor(uint32_t dividend, uint32_t divisor);
uint32_t mceil(uint32_t dividend, uint32_t divisor);
string itoa(uint32_t n, uint8_t base, int min_len = 0);
uint32_t LastUpdateTimeStamp(struct db_info **conn, const char *table, const char *column);
int ReadFileToBuf(const char *path, uint8_t **buf, int *len, bool add_nul_char);

/*
#ifdef __cplusplus 
}
#endif
*/
#endif
