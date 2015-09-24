#ifndef __MBFTABLE_H__
#define __MBFTABLE_H__

#include <vector>
#include <utility>
#include <cassert>
#include <sstream>
#include <memory>
#include <unordered_map>
//#include "cmph.h"
#include "utils/util.h"
#include "db_mbf.h"
#include "utils/db.h"
//#include "utils/targeting.h"
#include "mbftargeting.h"
#include "json.h"
#include "glog/logging.h"

#if __cplusplus >= 201103L
#include <atomic>
#else
#include <cstdatomic>
#endif

using namespace std;

extern const vector<string> g_targeting_attr; 

class MBFTable {

	private:

		atomic_char idx_inuse_;
		// TODO(@ktk57):- is this required?
		uint8_t padding_[64 - sizeof(idx_inuse_)];

		const unordered_map<int, shared_ptr<MBFTargeting>> *table_[2];
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
			uint32_t last_updated = LastUpdateTimeStamp(conn, "pub_mbf_info", "modification_time");
			if (last_updated > modification_timestamp_) {
				modification_timestamp_ = last_updated;
				return true;
			}
			return false;
		}

		MBFTargeting* get(const char* str, size_t len) const
		{
			MBFTargeting *result = nullptr;

			if (str == nullptr || str[0] == '\0' || str[0] != '{' || str[len - 1] != '}') {
				return result;
			}

			json_object *obj = json_tokener_parse(str);

			if ((obj == NULL) || (is_error(obj))) {
				LOG(ERROR) << "json_tokener_parse() return NULL";
				return result;
			}

			json_object *li = getFromObject(obj, "li", json_type_array);

			if ((li == NULL) || (is_error(li))) {
				LOG(ERROR) << "parsing json for li failed";
				goto END;
			}

			// NOTE :- The following variables are decalared C-style(instead of direct initialization) as the code won't compile because of "goto"
			// goto can't jump over scalar types which are initialized
			json_object *p2p;
			json_object *prob;

			int sz_li;
			int sz_p2p;
			int sz_prob;

			p2p = getFromObject(obj, "p2p", json_type_array);
			if ((p2p == NULL) || (is_error(p2p))) {
				LOG(ERROR) << "parsing json for p2p failed";
				goto END;
			}

			prob = getFromObject(obj, "prob", json_type_array);
			if ((prob == NULL) || (is_error(prob))) {
				LOG(ERROR) << "parsing json for prob failed";
				goto END;
			}


			sz_li = json_object_array_length(li);
			sz_p2p = json_object_array_length(p2p);
			sz_prob = json_object_array_length(prob);

			if (sz_li != sz_p2p || sz_li != sz_prob) {
				LOG(ERROR) << "sz_li, sz_p2p, sz_prob not equal " << sz_li << ", " << sz_p2p << " , " << sz_prob;
				goto END;
			}
			try {
				result = new MBFTargeting{g_targeting_attr, li, p2p, prob, sz_li};
			} catch(BadJSON& b) {
				LOG(ERROR) << "MBFTargeting Ctor() failed with msg = " << b.what();
				json_object_put(obj);
				return nullptr;
			}
END:
			json_object_put(obj);
			return result;
		}

		unordered_map<int, shared_ptr<MBFTargeting>>* createMap(struct db_info **conn) const
		{

			const vector<pair<int, string>>& data = ReadTableMBF(conn, queryMBF);
			if (data.empty()) {
				return nullptr;
			}
			/*
#ifndef NDEBUG
for (auto i = 0U; i != data.size(); i++) {
fprintf(stderr, "pubId:%d, json = \n%s\n", data[i].first, data[i].second.c_str());
}
#endif
*/

			// TODO(@ktk57): is this idiomatic or stupid, looks STUPID to me
			//shared_ptr<unordered_map<int, shared_ptr<MBFTargeting>>> result{new unordered_map<int, MBFTargeting>};
			unique_ptr<unordered_map<int, shared_ptr<MBFTargeting>>> result{new unordered_map<int, shared_ptr<MBFTargeting>>};
			//result->insert()
			//unordered_map<int, MBFTargeting>> *result = new unordered_map<int, MBFTargeting>>; 
			bool flag = false;
			for (auto i = 0U; i != data.size(); i++) {
				//auto tmp = get(data[i].second.c_str());
				MBFTargeting *tmp = get(data[i].second.c_str(), data[i].second.size());
				if (tmp == nullptr) {
					LOG(ERROR) << "get() function(parsing model json to MBFTargeting) failed for pub_id " << data[i].first;
					continue;
				}
				result->insert(move(make_pair(data[i].first, shared_ptr<MBFTargeting>{tmp})));
				flag = true;
			}
			return (flag == true)? result.release():nullptr;
		}

	public:
		MBFTable(const MBFTable&) = delete;
		MBFTable& operator=(const MBFTable&) = delete;

		MBFTable():idx_inuse_{-1}, table_{nullptr, nullptr}, modification_timestamp_(0U) {}

		// to be used by a single thread running in infinite loop waking up after configurable amount of seconds
		void Update(struct db_info **conn)
		{
			int8_t inuse = idx_inuse_.load();
			if (inuse == -1) {
				const auto tmp = createMap(conn);
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
				fprintf(stderr, "\nTable pub_mbf_info has been modified, reading new models\n");
				auto tmp = createMap(conn);
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
		double Find(const char* const *input, int sz, int pub_id, double minp2p, double cp, double p2p, double platform_fee)
		{

			DCHECK(input != NULL && sz >= 0 && minp2p <= p2p && p2p <= cp && platform_fee <= 1.0);

			int8_t i = idx_inuse_.load();
			if (i < 0) {
				LOG(ERROR) << "idx_inuse_ = " << idx_inuse_ << ", probably the update thread hasn't even run once yet, need to add sleep in main(), returning old p2p";
				return p2p;
			}

			const auto&  iter = table_[i]->find(pub_id);
			if (iter == table_[i]->cend()) {
				LOG(INFO) << "No MBF targeting found for pub_id " << pub_id;
				return p2p;
			}

			return iter->second->Find(input, sz, minp2p, cp, p2p, platform_fee);
		}
#ifdef MBF_INTEGRATION_TESTING
		// to be used by multiple threads
		vector<double> FindProb(const char* const *input, int sz, int pub_id, double minp2p, double cp, double p2p, double platform_fee)
		{

			//DCHECK(input != NULL && sz >= 0 && minp2p <= p2p && p2p <= cp && platform_fee <= 1.0);

			int8_t i = idx_inuse_.load();
			if (i < 0) {
#if __cplusplus >= 201103L
				LOG(ERROR) << "idx_inuse_ = " << to_string(idx_inuse_) << ", probably the update thread hasn't even run once yet, need to add sleep in main(), returning empty vector";
#else
				LOG(ERROR) << "idx_inuse_ = " << to_string(static_cast<long long>(idx_inuse_)) << ", probably the update thread hasn't even run once yet, need to add sleep in main(), returning empty vector";
#endif
				return vector<double>();
			}

			const auto&  iter = table_[i]->find(pub_id);
			if (iter == table_[i]->cend()) {
				LOG(INFO) << "No MBF targeting found for pub_id" << pub_id;
				return vector<double>();
			}

			return iter->second->FindProb(input, sz, minp2p, cp, p2p, platform_fee);
		}
#endif
};
#endif
