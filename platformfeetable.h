#ifndef __PLATFORMFEETABLE_H__
#define __PLATFORMFEETABLE_H__

#include <vector>
#include <utility>
#include <cassert>
#include <sstream>
#include <memory>
#include <algorithm>
//#include <unordered_map>
//#include "cmph.h"
#include "utils/util.h"
#include "db_mbf.h"
//#include "utils/db.h"
//#include "utils/targeting.h"
//#include "mbftargeting.h"
//#include "json.h"
#include "glog/logging.h"

#if __cplusplus >= 201103L
#include <atomic>
#else
#include <cstdatomic>
#endif

using namespace std;

static bool comp(const pair<int, double>& first, int second)
{
	return first.first < second;
}

class PlatformFeeTable {

	private:

		atomic_char idx_inuse_;
		// TODO(@ktk57):- is this required?
		uint8_t padding_[64 - sizeof(idx_inuse_)];

		const vector<pair<int, double>> *table_[2];
		// last modified on
		uint32_t modification_timestamp_;
		// TODO(@ktk57): add padding for cache line
		// timestamp
		bool tableModified(struct db_info **conn)
		{
			/*
			(void) conn;
			return true;
			*/

			uint32_t last_updated = LastUpdateTimeStamp(conn, "pub_platform_fee", "modification_time");
			if (last_updated > modification_timestamp_) {
				modification_timestamp_ = last_updated;
				return true;
			}
			return false;
		}

	public:
		PlatformFeeTable(const PlatformFeeTable&) = delete;
		PlatformFeeTable& operator=(const PlatformFeeTable&) = delete;

		PlatformFeeTable():idx_inuse_{-1}, table_{nullptr, nullptr}, modification_timestamp_(0U) {}

		// to be used by a single thread running in infinite loop waking up after configurable amount of seconds
		void Update(struct db_info **conn)
		{
			int8_t inuse = idx_inuse_.load();
			if (inuse == -1) {
				const vector<pair<int, double>> *tmp = ReadTablePF(conn, queryPF);
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
				const vector<pair<int, double>> *tmp = ReadTablePF(conn, queryPF);
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
		double Find(int pub_id)
		{

			int8_t i = idx_inuse_.load();
			if (i < 0) {
				LOG(ERROR) << "idx_inuse_ = " << idx_inuse_ << ", probably the update thread hasn't even run once yet, need to add sleep in main()";
				return -1.0;
			}

			const vector<pair<int, double>> *v = table_[i];
			const auto& iter = lower_bound(v->cbegin(), v->cend(), pub_id, comp);
			if (iter == v->cend() || iter->first != pub_id) {
				LOG(ERROR) << "platform fee for pub_id " << pub_id << " not found in the table as could not locate the pub_id";
				return -1.0;
			}
			return iter->second;
		}
};
#endif
