#ifndef __TARGETING_H__
#define __TARGETING_H__

#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <utility>
#include <sstream>

using namespace std;
#include "json.h"
#include "util.h"
#include "glog/logging.h"


class Targeting {
	private:
		// Number of line items
		// TODO(@ktk57): Remove the MAX_KEY_LEN limitation
		/*
#if __cplusplus >= 201103L
const int MAX_KEY_LEN = 64;
#else
const int MAX_KEY_LEN;
#endif
*/
		uint16_t count_;
		// approach in use, bitmap or line_item ids, true for bitmap else false
		bool using_bitmap;
		const char delim_;
		// key i.e "US" To line-item vector unordered_map
		unordered_map<string, vector<uint16_t>> table_;

		// key i.e "US" To line-item bitArray
		unordered_map<string, vector<uint8_t>> bit_table_;
		// attr_ibute-id To don't-care-line-item vector 
		unordered_map<uint8_t, vector<uint8_t>> bit_dnt_care_;
		// attr_ibute name to id mapping
		unordered_map<string, uint8_t> attr_;
		// attr_ibute-id To don't-care-line-item vector 
		unordered_map<uint8_t, vector<uint16_t>> dnt_care_;

		// max_load_factor
		double max_lf_;
		// memory factor to decide approach
		short mem_factor_;


		// TODO(@ktk57): Is this required?
		vector<string> makeKeys(const char* const *k, int sz) const
		{
			if (k == NULL) {
				return vector<string>();
			}

			vector<string> r(sz);

			const int TEMP_BUFF = 32;
			char temp[TEMP_BUFF + 1];

			for (int i = 0; i < sz; i++) {
				if (k[i] == NULL) {
					continue;
				}
				//sprintf(temp, "%u", attr_[k[i]]);
				sprintf(temp, "%u", i);
				string t;
				t += delim_;
				r[i] += k[i] + t + temp;
			}
			return r;
		}

		vector<uint16_t> getUnion(unordered_map<string, vector<uint16_t>>::const_iterator it1, unordered_map<uint8_t, vector<uint16_t>>::const_iterator it2) const
		{

			if (it1 != table_.cend() && it2 == dnt_care_.cend()) {
				return it1->second;
			}

			if (it1 == table_.cend() && it2 != dnt_care_.cend()) {
				return it2->second;
			}

			if (it1 == table_.cend() && it2 == dnt_care_.cend()) {
				return vector<uint16_t>();
			}

			auto sz = it1->second.size() + it2->second.size();

			vector<uint16_t> result(sz);
			const auto& iter = set_union(it1->second.cbegin(), it1->second.cend(), it2->second.cbegin(), it2->second.cend(), result.begin());
			result.resize(iter - result.cbegin());
			return result;
		}

		// Returns the "value" corresponding to a key and attr_id
		vector<uint16_t> getValue(const string& k, uint16_t id) const
		{
			// TODO(@ktk57): Does this make any sense, this constantness?
			const auto& it1 = table_.find(k);
			const auto& it2 = dnt_care_.find(id);

			return getUnion(it1, it2);
		}

		// Given a value in [0, (1<<16) - 1], sets the corresponding bit in des
		inline void setBit(vector<uint8_t>& des, uint16_t val) const
		{
			auto idx = val / 8;
			auto r = val % 8;
			auto num = 1 << (7 - r);
			des[idx] |=  num;
		}

		//void performUnion(vector<uint8_t>& dst, const vector<uint8_t>& src) const
		//{
		//assert(dst.size() == src.size());
		//for (int i = 0; i != static_cast<int>(dst.size()); i++) {
		//dst[i] = dst[i] | src[i];
		//}
		//}


		vector<uint8_t> createBitVec(const vector<uint16_t>& v) const
		{
			int sz = (count_ - 1) / 8 + 1;
			vector<uint8_t> bits(sz, 0);
			for (auto i = 0U; i != v.size(); i++) {
				setBit(bits, v[i]);
			}
			return bits;
		}

		inline uint8_t getAttrID(const string& s) const
		{
			auto idx = s.find_last_of(delim_);
			DCHECK(idx != s.npos);
			uint32_t result = -1;
			// TODO(@ktk57): no error handling done
			sscanf(s.c_str() + idx + 1, "%u", &result);
			return result;
		}

		void oR(vector<uint8_t>& dst, const vector<uint8_t>& src) const
		{
			DCHECK(dst.size() == src.size());
			for (auto i = 0U; i != dst.size(); i++) {
				dst[i] |= src[i];
			}
		}

		// Build bit_table from vector
		void buildBitMap()
		{
			for (auto iter = table_.cbegin(); iter != table_.cend(); iter++) {
				//auto id = getID(iter->first);
				//auto it = dnt_care_.find(id);
				// TODO(@ktk57): Verify that this doesn't generate multiple copies
				// TODO(@ktk57): Verify what are redundant move operations
				bit_table_.insert(move(make_pair(iter->first, move(createBitVec(iter->second)))));
				//bit_table_[iter->first] = createBitVec(iter->second);
			}

			//DLOG(INFO) << "bit_table_.size() = " << bit_table_.size();

			for (auto iter = dnt_care_.cbegin(); iter != dnt_care_.cend(); iter++) {
				//auto id = getID(iter->first);
				//auto it = dnt_care_.find(id);
				bit_dnt_care_.insert(move(make_pair(iter->first, move(createBitVec(iter->second)))));
				//bit_dnt_care_[iter->first] = createBitVec(iter->second);
			}

			// Take OR
			for (auto iter = bit_table_.begin(); iter != bit_table_.end(); iter++) {
				//auto id = getID(iter->first);
				//auto it = dnt_care_.find(id);
				//DLOG(INFO) << "Calling OR for " << iter->first << " and size = " << iter->second.size() << " # of line_items = " << count_ << " getAttrID(iter->first) = " << getAttrID(iter->first);
				const auto& it = bit_dnt_care_.find(getAttrID(iter->first));
				if (it != bit_dnt_care_.end()) {
					oR(iter->second, it->second);
				}
			}
		}


		// This gets called for each attribute name, i.e attribute name and array of values are passed.
		// Store line_item # for each of the items in the array attr_values appended with <delim>suffix
		int storeValues(json_object *attr_values, uint8_t suffix, uint16_t li)
		{
			int sz = json_object_array_length(attr_values);
			for (int i = 0; i < sz; i++) {
				json_object *v = json_object_array_get_idx(attr_values, i);
				if (v == NULL) {
					// TODO(@ktk57): We should continue? What if the value null is stored in JSON?
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					LOG(WARNING) << "json_object_array_get_idx(attr_values, " << i << ") returned NULL";
					//--sz;
					//continue;
					return -1;
				}
				//char tbuff[MAX_KEY_LEN + 1];
				const char* str = json_object_get_string(v);
				if (str == NULL || json_object_get_type(v) != json_type_string) {
					LOG(WARNING) << "json_object_get_string(v) returned " << ((str == NULL)?"NULL":"non-string type");
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					return -1;
					//--sz;
					//continue;
				}

				string tbuff{str};

#if __cplusplus >= 201103L
				tbuff += delim_ + to_string(suffix);
#else
				tbuff += delim_ + to_string(static_cast<long long>(suffix));
#endif
				/*
					 if (snprintf(tbuff, MAX_KEY_LEN + 1, "%s_%d", str, suffix) >= MAX_KEY_LEN + 1) {
					 LOG(ERROR) << "Buffer Overflow: key(attr_value) i.e " << str << "_" << suffix << " greater than " << MAX_KEY_LEN;
				//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
				return -1;
				}
				*/
				// TODO(@ktk57): what if a given key is duplicated in a given array? In this case the key's values will have duplicate line items. Need to FIX this
				table_[tbuff].push_back(li);
			}
			return sz;
		}

		void storeDontCareValues(const unordered_map<string, bool>& is_present, int li)
		{
			for (auto iter = is_present.cbegin(); iter != is_present.cend(); iter++) {
				if (iter->second == false) {
					dnt_care_[attr_[iter->first]].push_back(li);
				}
			}
		}

		// This is called for each line item.
		// Input are 2 arrays, attibute names, and double array of attribute values i.e for each value in attirbute array, there exists an array of attribute values
		int parse(json_object* attr_names, json_object *attr_vdarr, int sz, uint16_t li)
		{
			// To find all attr_ibute names which are don't care
			unordered_map<string, bool> is_present;

			for (auto it = attr_.cbegin(); it != attr_.cend(); it++) {
				// mark all attr_ibutes as not present
				is_present[it->first] = false;
			}

			for (int i = 0; i < sz; i++) {

				json_object *name = json_object_array_get_idx(attr_names, i);
				if (name == NULL) {
					// TODO(@ktk57): we should continue instead of flagging an error
					LOG(WARNING) << "json_object_array_get_idx(attr_names, " << i << ") returned NULL";
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					return -1;
					//continue;
				}

				const char *str = json_object_get_string(name);
				if (str == NULL || json_object_get_type(name) != json_type_string) {
					LOG(ERROR) << "json_object_get_string(v) returned " << ((str == NULL)?"NULL":"non-string type");
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					return -1;
				}


				// Verify that this is a valid attribute name
				const auto& t = attr_.find(str);
				if (t == attr_.end()) {
					LOG(ERROR) << "Attribute name " << str << " doesn't exist in initially passed list of attributes";
					//fprintf(stderr, "\nERROR attr_ibute %s name not DEFINED %s:%s:%d\n", str, __func__, __FILE__, __LINE__);
					return -1;
				}

				// Get the array of values corresponding to this attribute name
				json_object *value_arr = json_object_array_get_idx(attr_vdarr, i);
				if (value_arr == NULL || json_object_get_type(value_arr) != json_type_array) {
					// TODO(@ktk57): should we continue?
					LOG_IF(WARNING, value_arr == NULL) << "json_object_array_get_idx(attr_vdarr, " << i << ") " << "for " << str << " returned NULL";

					LOG_IF(WARNING, json_object_get_type(value_arr) != json_type_array) << "json_object_array_get_idx(attr_vdarr, " << i << ") " << "for " << str << " is not of type array"; 
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					return -1;
					//continue;
				}


				// id to append to the key
				uint8_t suffix = attr_[str];
				int count = storeValues(value_arr, suffix, li);
				if (count <= 0) {
#if __cplusplus >= 201103L
					LOG(ERROR) << "storeValues(value_arr, " << to_string(suffix) << ", " << li << ") failed for " << str;
#else
					LOG(ERROR) << "storeValues(value_arr, " << to_string(static_cast<long long> (suffix)) << ", " << li << ") failed for " << str;
#endif
					LOG(ERROR) << count;
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					return -1;
				}

				if (count > 0) {
					is_present[str] = true;
				}
			}
			storeDontCareValues(is_present, li);
			return 0;
		}


		void aND(vector<uint8_t>& dst, const vector<uint8_t>& src) const
		{
			DCHECK(dst.size() == src.size());
			for (auto i = 0U; i != dst.size(); i++) {
				dst[i] = dst[i] & src[i];
			}
		}

		// Get a resultant vector of line_items from a bit vector of line items
		vector<uint16_t> getVec(const vector<uint8_t>& s) const
		{
			vector<uint16_t> result;
			for (auto i = 0U; i != s.size(); i++) {
				uint8_t temp = (1<<7);
				for (int j = 0; j < 8; j++) {
					if (s[i] & (temp >> j)) {
						result.push_back(i * 8 + j);
					}
				}
			}

#ifdef MBF_FUNCTIONAL_TESTING
			string tmp;
			DLOG(INFO) << "Printing the resulting bit vector";
			for (auto i = 0U; i != s.size(); i++) {
				tmp += itoa(s[i], 2, 8);
			}
			DLOG(INFO) << tmp;
#endif
			return result;
		}

		// get bitVector for input string and attr_id
		vector<uint8_t> getBitV(const string& s, int id) const
		{
			const auto& it1 = bit_table_.find(s);

			if (it1 != bit_table_.cend()) {
				return it1->second;
			}

			const auto& it2 = bit_dnt_care_.find(id);

			if (it2 != bit_dnt_care_.cend()) {
				return it2->second;
			}
			return vector<uint8_t>();
		}

		vector<uint16_t> find2(const vector<string>& k) const
		{
			//vector<uint16_t> empty;
			//vector<uint16_t> result;
			vector<uint8_t> result;
			bool flag = true;

			for (auto i = 0U; i != k.size(); i++) {
				if (k[i].empty()) {
					continue;
				}
				const auto& v = getBitV(k[i], i);
				if (v.empty()) {
					LOG(WARNING) << "getBitV(" << k[i] << ", " << i << ") returned empty vector";
					//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
					return vector<uint16_t>();
				}

				if (flag == true) {
					result = move(v);
					flag = false;
					continue;
				}
				aND(result, v);
			}
			/*
				 fprintf(stderr, "\n");
				 for (auto i = 0U; i != result.size(); i++) {
				 fprintf(stderr, "%u, ", result[i]);
				 }
				 fprintf(stderr, "\n");
				 */
			return getVec(result);
		}

		vector<uint16_t> find1(const vector<string>& k) const
		{
			vector<uint16_t> result;
			bool flag = true;

			for (auto i = 0U; i != k.size(); i++) {
				if (k[i].empty()) {
					continue;
				}

				const auto& v = getValue(k[i], i);
				if (v.empty()) {
					LOG(WARNING) << "getValue(" << k[i] << ", " << i << ") returned empty vector";
					return vector<uint16_t>();
				}

				if (flag == true) {
					//result = v; // TODO should i use move here?
					result = move(v);
					flag = false;
					continue;
				}

				auto sz = min(result.size(), v.size());
				vector<uint16_t> temp(sz);
				const auto& iter = set_intersection(result.begin(), result.end(), v.begin(), v.end(), temp.begin());
				temp.resize(iter - temp.cbegin());
				//result = temp;// TODO should i use move here?
				result = move(temp);
			}
			return result;
		}

		bool use_bitmap()
		{

			size_t sz = 0;
			for (auto iter = table_.begin(); iter != table_.end(); iter++) {
				sz += iter->second.capacity();
			}
			for (auto iter = dnt_care_.begin(); iter != dnt_care_.end(); iter++) {
				sz += iter->second.capacity();
			}

			fprintf(stderr, "\n# of Line Items = %d\n", count_);

			fprintf(stderr, "\nSize taken by table_ approach  = %lu bytes(including excess capacity)\n", sz * sizeof(uint16_t));
			fprintf(stderr, "\nSize taken by bit_table_ approach  = %lu bytes(contant time)\n", mceil(static_cast<uint32_t>(count_), static_cast<uint32_t>(8)) * (table_.size() + dnt_care_.size()));


			size_t sz_bitmap_approach = mceil(static_cast<uint32_t>(count_), static_cast<uint32_t>(8)) * (table_.size() + dnt_care_.size());

			if (sz_bitmap_approach > mem_factor_ * sz * sizeof(uint16_t)) {
				fprintf(stderr, "\nWARNING, using alternate approach, bitmap size = %lu bytes, alternate = %lu bytes %s:%d\n", sz_bitmap_approach, sz * sizeof(uint16_t), __FILE__, __LINE__);
				return false;
			}
			return true;
		}


	public:
		Targeting(const Targeting&) = delete;
		Targeting& operator=(const Targeting&) = delete;
		// Assumes that the attr_ibutes names are not duplicate and buf and buf[i]
		// are valid
		// TODO(@ktk57): maximum number of attr_ibute names supported = 128 i.e [0, 127]
		/*
#if __cplusplus >= 201103L
Targeting(const char** buf, uint8_t size, char d = '_', double lf = 0.5):count_(0), delim_(d), max_lf_(lf)
#else
Targeting(const char** buf, uint8_t size, char d = '_'):MAX_KEY_LEN(64), count_(0), delim_(d)
#endif
*/
		Targeting(const vector<string>& att , json_object* arr, int len, const string& k = "keys", const string& v = "values", char d = '_', double lf = 0.5):count_(0), using_bitmap(false), delim_(d), max_lf_(lf), mem_factor_(5)
	{
		assert(arr != nullptr);
		for (auto i = 0U; i != att.size(); i++) {
			// TODO(@ktk57): no error checking is done for att[i]
			attr_[att[i]] = i;
		}

		table_.max_load_factor(max_lf_);
		dnt_care_.max_load_factor(max_lf_);
		bit_table_.max_load_factor(max_lf_);
		bit_dnt_care_.max_load_factor(max_lf_);

		if (len <= 0 || len > (1<<16)) {
			stringstream err_msg;
			err_msg << "# of line_items " << len << ", we support only " << (1<<16) << " lineitems.";
			LOG(ERROR) << err_msg.str();
			throw BadJSON(err_msg.str());
		}

		// copy of len
		//int clen = len;

		for (int i = 0; i < len; i++) {

			json_object *kv = json_object_array_get_idx(arr, i);

			if (kv == NULL) {
				//--clen;
				stringstream err_msg;
				err_msg << "json_object_array_get_idx(arr, " << i << ") returned NULL";
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}

			json_object *key = getFromObject(kv, k.c_str(), json_type_array);

			if (key == NULL) {

				stringstream err_msg;
				err_msg << "getFromObject(kv, " << k.c_str() << " json_type_array) returned NULL for lineitem " << i << " (index starts from 0)";
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}

			json_object *value = getFromObject(kv, v.c_str(), json_type_array);
			if (value == NULL) {
				stringstream err_msg;
				err_msg << "getFromObject(kv, " << v.c_str() << " json_type_array) returned NULL for lineitem " << i << " (index starts from 0)";
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}

			int sz_keys = json_object_array_length(key);
			int sz_values = json_object_array_length(value);

			if (sz_keys != sz_values) {
				stringstream err_msg;
				err_msg << "For line_item " << i << "sz_keys != sz_values i.e ( " << sz_keys << " != " << sz_values << " )";
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}

			if (parse(key, value, sz_keys, i) != 0) {
				stringstream err_msg;
				err_msg << "parse(key, value, # of keys i.e " << sz_keys << ",  line_item_idx = " << i << ") failed";
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}
		}
		count_ = len;
		// TODO(@ktk57): Add logic for bitMap approach
		using_bitmap = use_bitmap();
		if (using_bitmap) {
			buildBitMap();
#ifdef NDEBUG
			table_.clear();
			dnt_care_.clear();
#endif
		}
	}

		uint16_t Count() const
		{
			return count_;
		}

#ifdef MBF_FUNCTIONAL_TESTING
		void Stats() const
		{
			fprintf(stderr, "\n==========Stats===========\n");

			size_t sz = 0;
			for (auto iter = table_.begin(); iter != table_.end(); iter++) {
				sz += iter->second.capacity();
			}
			for (auto iter = dnt_care_.begin(); iter != dnt_care_.end(); iter++) {
				sz += iter->second.capacity();
			}

			fprintf(stderr, "\n# of Line Items = %d\n", count_);

			fprintf(stderr, "\nSize taken by table_ approach  = %lu bytes(including excess capacity)\n", sz * sizeof(uint16_t));


			sz = 0;
			for (auto iter = bit_table_.begin(); iter != bit_table_.end(); iter++) {
				sz += iter->second.capacity();
			}
			for (auto iter = bit_dnt_care_.begin(); iter != bit_dnt_care_.end(); iter++) {
				sz += iter->second.capacity();
			}

			fprintf(stderr, "\nSize taken by bit_table_ approach  = %lu bytes(contant time), %lu (using loop)\n", mceil(static_cast<uint32_t>(count_), static_cast<uint32_t>(8)) * (bit_table_.size() + bit_dnt_care_.size()), (bit_table_.size() + bit_dnt_care_.size()) * sz);

			/*
				 fprintf(stderr, "\nLoad factor for table_, dnt_care_ = %lf, %lf, max_load_factor = %lf, size = %lu, # of buckets = %lu\n", table_.load_factor(), dnt_care_.load_factor(), max_lf_, table_.size(), table_.bucket_count());

				 fprintf(stderr, "\n=====Collision info for table_====\n");
				 for (size_t i = 0; i != table_.bucket_count(); i++) {
				 if (table_.bucket_size(i) > 1) {
				 fprintf(stderr, "\nCollision exists for bucket = %lu, # of collisions = %lu\n", i, table_.bucket_size(i));
				 fprintf(stderr, "\nKeys that collide at bucket %lu are :-\n", i);
				 for (auto iter = table_.begin(i); iter != table_.end(i); iter++) {
				 fprintf(stderr, "%s\n", iter->first.c_str());
				 }
				 }
				 }
				 fprintf(stderr, "\n=====End table_ collision info======\n");


				 fprintf(stderr, "\n=====Collision info for dnt_care_====\n");
				 for (size_t i = 0; i != dnt_care_.bucket_count(); i++) {
				 if (dnt_care_.bucket_size(i) > 1) {
				 fprintf(stderr, "\nCollision exists for bucket = %lu, # of collisions = %lu\n", i, dnt_care_.bucket_size(i));
				 fprintf(stderr, "\nKeys that collide at bucket %lu are :-\n", i);
				 for (auto iter = dnt_care_.begin(i); iter != dnt_care_.end(i); iter++) {
				 fprintf(stderr, "%u\n", iter->first);
				 }
				 }
				 }

				 fprintf(stderr, "\n=====End dnt_care_ collision info======\n");

				 fprintf(stderr, "\n=====Collision info for bit_table_====\n");
				 for (size_t i = 0; i != bit_table_.bucket_count(); i++) {
				 if (bit_table_.bucket_size(i) > 1) {
				 fprintf(stderr, "\nCollision exists for bucket = %lu, # of collisions = %lu\n", i, bit_table_.bucket_size(i));
				 fprintf(stderr, "\nKeys that collide at bucket %lu are :-\n", i);
				 for (auto iter = bit_table_.begin(i); iter != bit_table_.end(i); iter++) {
				 fprintf(stderr, "%s\n", iter->first.c_str());
				 }
				 }
				 }

				 fprintf(stderr, "\n=====End bit_table_ collision info======\n");

				 fprintf(stderr, "\n=====Collision info for bit_dnt_care_====\n");
				 for (size_t i = 0; i != bit_dnt_care_.bucket_count(); i++) {
				 if (bit_dnt_care_.bucket_size(i) > 1) {
				 fprintf(stderr, "\nCollision exists for bucket = %lu, # of collisions = %lu\n", i, bit_dnt_care_.bucket_size(i));
				 fprintf(stderr, "\nKeys that collide at bucket %lu are :-\n", i);
				 for (auto iter = bit_dnt_care_.begin(i); iter != bit_dnt_care_.end(i); iter++) {
				 fprintf(stderr, "%u\n", iter->first);
				 }
				 }
				 }
				 fprintf(stderr, "\n=====End bit_dnt_care_ collision info======\n");
				 */

			fprintf(stderr, "\nLoad factor for bit_table_, bit_dnt_care_ = %lf, %lf, max_load_factor = %lf, size = %lu, # of buckets = %lu\n", bit_table_.load_factor(), bit_dnt_care_.load_factor(), max_lf_, bit_table_.size(), bit_table_.bucket_count());

			fprintf(stderr, "\n==========End===========\n");
		}
#endif

		void DebugPrint() const
		{
			fprintf(stderr, "\n=====================================\n");
			fprintf(stderr, "\n# of Line Items = %u\n", count_);

			fprintf(stderr, "\nPrinting attr_ibute names:\n");

			//for (uint8_t i = 0; i != attr_.size(); i++)
			for (auto it = attr_.cbegin(); it != attr_.cend(); it++) {
				fprintf(stderr, "%s:%u\n", it->first.c_str(), it->second);
			}

			fprintf(stderr, "\nPrinting line items :\n");

			for (auto it = table_.cbegin(); it != table_.cend(); it++) {
				fprintf(stderr, "%s : ", it->first.c_str());
				auto& vec = it->second;
				for (auto iter = vec.begin(); iter != vec.end(); iter++) {
					fprintf(stderr, "%u, ", *iter);
				}
				fprintf(stderr, "\n");
			}


			fprintf(stderr, "\nPrinting dont care line items :\n");

			for (auto it = dnt_care_.cbegin(); it != dnt_care_.cend(); it++) {
				fprintf(stderr, "%u : ", it->first);
				auto& vec = it->second;
				for (auto iter = vec.begin(); iter != vec.end(); iter++) {
					fprintf(stderr, "%u, ", *iter);
				}
				fprintf(stderr, "\n");
			}
			fprintf(stderr, "\n=====================================\n");


			fprintf(stderr, "\nPrinting bit_table_ :\n");

			for (auto it = bit_table_.cbegin(); it != bit_table_.cend(); it++) {
				fprintf(stderr, "%s : ", it->first.c_str());
				auto& vec = it->second;
				for (auto iter = vec.begin(); iter != vec.end(); iter++) {
					fprintf(stderr, "%s", itoa(*iter, 2, 8).c_str());
				}
				fprintf(stderr, "\n");
			}

			fprintf(stderr, "\nPrinting dont care bit_table_ :\n");

			for (auto it = bit_dnt_care_.cbegin(); it != bit_dnt_care_.cend(); it++) {
				fprintf(stderr, "%u : ", it->first);
				auto& vec = it->second;
				for (auto iter = vec.begin(); iter != vec.end(); iter++) {
					fprintf(stderr, "%s", itoa(*iter, 2, 8).c_str());
				}
				fprintf(stderr, "\n");
			}
			fprintf(stderr, "\n=====================================\n");
		}

		/*
			 int parseJSONArray(json_object* arr, int len, const string& k, const string& v)
			 {
			 if (len <= 0 || len >= (1<<16)) {
			 LOG(ERROR) << "# of attributes = " << len;

			 }

		// copy of len
		int clen = len;

		for (int i = 0; i < len; i++) {
		json_object *kv = json_object_array_get_idx(arr, i);

		if (kv == NULL) {
		LOG(WARNING) << "json_object_array_get_idx(arr, " << i << ") returned NULL";
		--clen;
		continue;
		}

		json_object *key = getFromObject(kv, k.c_str(), json_type_array);
		if (key == NULL) {
		LOG(ERROR) << "getFromObject(kv" <<  ", " << k.c_str() << ", json_type_array) returned NULL for lineitem " << i;
//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
return -1;
}
json_object *value = getFromObject(kv, v.c_str(), json_type_array);
if (value == NULL) {
LOG(ERROR) << "getFromObject(" << kv << ", " << v.c_str() << ", json_type_array) returned NULL";
//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
return -1;
}

int sz_keys = json_object_array_length(key);
int sz_values = json_object_array_length(value);

if (sz_keys != sz_values) {
LOG(ERROR) << "For line_item " << i << "sz_keys != sz_values i.e " << sz_keys << " != " << sz_values;
//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
return -1;
}

if (parse(key, value, sz_keys, i) != 0) {
LOG(ERROR) << "parse(key, value, " << sz_keys << ", " << i << ") failed";
//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
return -1;
}
}
count_ = static_cast<uint16_t>(clen);
// TODO(@ktk57): Add logic for bitMap approach
if (true) {
buildBitMap();
}
return 0;
}*/

vector<uint16_t> Find(const char *const *k, int sz) const
{
	if (sz != static_cast<int>(attr_.size())) {
		LOG(ERROR) << "sz != attr_.size() i.e " << sz << " != " << attr_.size();
		//fprintf(stderr, "\nERROR returning %s:%s:%d\n", __FILE__, __func__, __LINE__);
		return vector<uint16_t>();
	}

	const auto& keys = makeKeys(k, sz);
#if defined(MBF_FUNCTIONAL_TESTING) || defined(MBF_INTEGRATION_TESTING)
	//fprintf(stderr, "\n=====================================\n");
	//fprintf(stderr, "\nPrinting keys\n");
	for (auto i = 0U; i != keys.size();i++) {
		fprintf(stderr, "%s\t", keys[i].c_str());
	}
	fprintf(stderr, "\n");
	//fprintf(stderr, "\n=====================================\n");
#endif
#ifndef NDEBUG 
	const auto& f2 = find2(keys); // TODO should I use move here?
	const auto& f1 = find1(keys); // TODO should I use move here?
	assert(f1 == f2);
	return f2;
#else
	if (using_bitmap) {
		return find2(keys);
	}
	return find1(keys);
#endif
}
};

/*
// Line-item To Range of probabilities
map<uint16_t, vector<pair<uint16_t, uint16_t>>> prob;
// Line-item To Range of p2p(pay to publisher)
map<uint16_t, vector<pair<uint16_t, uint16_t>>> p2p;
// keys to get a perfect hash
struct str_vec_ keys;
*/

//int fill_info_(


#endif
/*
	 json_obj = json_tokener_parse(buffer);
	 if ((json_obj == NULL) || (is_error(json_obj))) {
 * 
 * json_object_array_get_idx(bid_arr, 0);
 * json_object_array_length(bid_arr);
 * bid_arr = get_json_child_from_parent(json_obj, BID_ARRAY, json_type_array);
 */
#if 0

json_object *get_json_child_from_parent(
		json_object *parent_json_object,
		const char *key,
		json_type child_json_type) {
	/*
	 * Local variables.
	 */
	json_object *child_json_object = NULL;

	/*
	 * Check function parameters.
	 */
	if ((parent_json_object == NULL) || (key == NULL) || (json_type_object != parent_json_object->o_type)){
		fprintf(stderr, "Invalid parameters : '%s:%s:%d'\n", __FILE__, __FUNCTION__, __LINE__);
		goto done;
	}

	child_json_object = json_object_object_get(parent_json_object, key);
	if (child_json_object == NULL) {
#ifdef DEBUG
		fprintf(stderr, "Key : '%s' not present in json object\n", key);
#endif
		goto done;
	}

	/*
	 * Now check object type.
	 */
	if (json_object_is_type(child_json_object, child_json_type) == 0) {
#ifdef DEBUG
		fprintf(stderr, "Invalid JSON object type : '%s:%s:%d'\n",
				__FILE__, __FUNCTION__, __LINE__);
#endif
		child_json_object = NULL;
		goto done;
	}

done:
	return child_json_object;
}

#endif
