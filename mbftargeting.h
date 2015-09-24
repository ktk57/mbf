#ifndef __MBFTARGETING_H__
#define __MBFTARGETING_H__
#include <vector>
#include <utility>
#include <cassert>
#include <sstream>
//#include "cmph.h"
#include "utils/util.h"
#include "utils/targeting.h"
#include "json.h"
#include "glog/logging.h"

using namespace std;

class MBFTargeting {

	private:

		Targeting line_target_;
		char delim_;
		vector<pair<vector<pair<double, double>>, vector<double>>> line_info_;

		pair<double, double> range(json_object *val) const
		{
			const char *str = json_object_get_string(val);

			if (str == NULL) {
				stringstream err_msg;
				err_msg << "json_object_get_string(val) returned NULL";
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}
			// TODO(@ktk57): remove magic numbers
			pair<double, double> range_p2p{-1, 1000};
			char fmt_specifier[32];
			sprintf(fmt_specifier, "%%lf%c%%lf", delim_);
			int ret_val = sscanf(str, fmt_specifier, &range_p2p.first, &range_p2p.second);
			if (ret_val != 2 || range_p2p.first < 0 || range_p2p.first > 1000 || range_p2p.second < 0 || range_p2p.second > 1000 || range_p2p.first > range_p2p.second) {
				stringstream err_msg;
				err_msg << "sscanf for " << str << " returned " << ret_val << " left = " << range_p2p.first << " right = " << range_p2p.second;
				LOG(ERROR) << err_msg.str();
				throw BadJSON(err_msg.str());
			}
			return range_p2p;
		}

		// Single p2p array
		vector<pair<double, double>> getP2P(json_object *arr) const
		{

			vector<pair<double, double>> result;
			switch (json_object_get_type(arr)) {

				case json_type_string :
					{
						result.push_back(move(range(arr)));
						break;
					}

				case json_type_array :
					{
						auto sz = json_object_array_length(arr);
						for (auto i = 0; i < sz; i++) {
							json_object *val = json_object_array_get_idx(arr, i);
							if (val == NULL) {
								stringstream err_msg;
								err_msg << "json_object_array_get_idx(arr, " << i << " ) returned NULL";
								LOG(ERROR) << err_msg.str();
								throw BadJSON(err_msg.str());
							}
							result.push_back(move(range(val)));
						}
						break;
					}

				default :
					{
						stringstream err_msg;
						err_msg << "p2p is neither string nor array of string";
						LOG(ERROR) << err_msg.str();
						throw BadJSON(err_msg.str());
					}
			}
			return result;
		}

		double getDouble(json_object *val) const
		{
			switch (json_object_get_type(val)) {
				case json_type_double:
					{
						return json_object_get_double(val);
					}

				case json_type_int:
					{
						return json_object_get_int(val);
					}
				default :
					{
						stringstream err_msg;
						err_msg << "val is neither int nor double";
						LOG(ERROR) << err_msg.str();
						throw BadJSON(err_msg.str());
					}
			}
		}

		vector<double> getProbability(json_object *arr) const
		{

			vector<double> result;

			switch (json_object_get_type(arr)) {

				case json_type_double:
					{
						result.push_back(json_object_get_double(arr));
						break;
					}

				case json_type_int:
					{
						result.push_back(json_object_get_int(arr));
						break;
					}

				case json_type_array :
					{
						auto sz = json_object_array_length(arr);
						for (auto i = 0; i < sz; i++) {
							json_object *val = json_object_array_get_idx(arr, i);
							if (val == NULL) {
								stringstream err_msg;
								err_msg << "json_object_array_get_idx(arr, " << i << " ) returned NULL";
								LOG(ERROR) << err_msg.str();
								throw BadJSON(err_msg.str());
							}
							result.push_back(getDouble(val));
						}
						break;
					}

				default :
					{
						stringstream err_msg;
						err_msg << "prob is neither array of doubles nor double";
						LOG(ERROR) << err_msg.str();
						throw BadJSON(err_msg.str());
					}
			}
			return result;
		}

		double value(double minp2p, double cp, double p2p, double left, double right) const
		{
			DCHECK(minp2p <= p2p && p2p <= cp);
			DCHECK(left <= right);
			DCHECK(left <= cp && right >= minp2p);

			if (p2p >= left && p2p <= right) {
				return p2p;
			}

			// This check is intenionally to verify that p2p lies to the left of [left, right]
			if (p2p < right) {
				DCHECK(left > p2p);
				return left;
			}

			DCHECK(right < p2p);
			return right;
		}

#ifdef MBF_INTEGRATION_TESTING
		vector<double> selectProb(const vector<uint16_t>& in, double p2p) const
		{
			vector<double> result;

			for (auto i = 0U; i != in.size(); i++) {
				auto idx = in[i];
				const auto& p = line_info_[idx];
				const auto& p2p_vec = p.first;
				const auto& prob_vec = p.second;

				if (p2p_vec.size() != prob_vec.size()) {
					LOG(ERROR) << "p2p_vec.size() i.e " << p2p_vec.size() << " != " << prob_vec.size() << " (prob_vec.size()) for " << idx << " line item";
					return result;
				}

				for (auto j = 0U; j != p2p_vec.size(); j++) {
					if (p2p_vec[j].second >= p2p && p2p_vec[j].first <= p2p) {
						result.push_back(1.0 - prob_vec[j]);
					}
				}
			}
			return result;
		}
#endif
		//pair<vector<double>, vector<double>> select(const vector<uint16_t>& in, double minp2p, double cp, double p2p)
		vector<pair<double, double>> select(const vector<uint16_t>& in, double minp2p, double cp, double p2p) const
		{
			vector<pair<double, double>> result;

			for (auto i = 0U; i != in.size(); i++) {
				auto idx = in[i];
				const auto& p = line_info_[idx];
				const auto& p2p_vec = p.first;
				const auto& prob_vec = p.second;

				if (p2p_vec.size() != prob_vec.size()) {
					LOG(ERROR) << "p2p_vec.size() i.e " << p2p_vec.size() << " != " << prob_vec.size() << " (prob_vec.size()) for " << idx << " line item";
					return result;
				}

				for (auto j = 0U; j != p2p_vec.size(); j++) {
#ifdef MBF_FUNCTIONAL_TESTING
					LOG(INFO) << "line item id : " << idx << " p2p range [" << p2p_vec[j].first << ", " << p2p_vec[j].second << "]";
#endif
					if (p2p_vec[j].second < minp2p || p2p_vec[j].first > cp) {
						// range is outside [minp2p, cp]
						continue;
					}
					//result.first.push_back(value(minp2p, cp, p2p, p2p_vec[j].first, p2p_vec[j].second));
					//result.second.push_back(prob_vec[j]);
					result.push_back(move(make_pair(value(minp2p, cp, p2p, p2p_vec[j].first, p2p_vec[j].second), prob_vec[j])));
				}
			}
			return result;
		}

		double calculateP2P(const vector<pair<double, double>>& selected_li, double cp, double platform_fee, double unused_p2p) const
		{
			double max_rev = 0.0;
			double result = -1;
			for (auto i = 0U; i != selected_li.size(); i++) {
				double p2p = selected_li[i].first;
				double prob = selected_li[i].second;
				double temp_rev = prob * ((cp - p2p) + p2p * platform_fee);
				if (temp_rev > max_rev) {
					result = p2p;
					max_rev = temp_rev;
				}
			}

			DCHECK(result >= 0.0);

			if (result < 0.0) {
				// TODO TODO 
				// should never reach here
				// TODO TODO 
				LOG(ERROR) << "p2p is coming out negative i.e " << result << " for cp = " << cp << ", platform_fee = " << platform_fee << ", returning old_p2p = " << unused_p2p;
				return unused_p2p;
			}

			return result;
		}


	public:
		MBFTargeting(const MBFTargeting&) = delete;
		MBFTargeting& operator=(const MBFTargeting&) = delete;

		MBFTargeting(const vector<string>& attributes, json_object* li_arr, json_object* p2p_darr, json_object* prob_darr, int len, char d = '_'):line_target_(attributes, li_arr, len), delim_(d)
		{
			DCHECK(len == json_object_array_length(li_arr) && len == json_object_array_length(p2p_darr) && len == json_object_array_length(prob_darr)); 

			for (int i = 0; i < len; i++) {

				json_object *p2p_arr = json_object_array_get_idx(p2p_darr, i);

				if (p2p_arr == NULL) {
					// TODO(@ktk57): what to do in case of null and in case lengths are different
					stringstream err_msg;
					err_msg << "p2p_arr is NULL for json_object_array_get_idx(p2p_darr, " << i << " ) for len = " << len;
					LOG(ERROR) << err_msg.str();
					throw BadJSON(err_msg.str());
				}

				json_object *prob_arr = json_object_array_get_idx(prob_darr, i);

				if (prob_arr == NULL) {
					// TODO(@ktk57): what to do in case of null and in case lengths are different
					stringstream err_msg;
					err_msg << "prob_arr is NULL for json_object_array_get_idx(prob_darr, " << i << " ) for len = " << len;
					LOG(ERROR) << err_msg.str();
					throw BadJSON(err_msg.str());
				}

				const vector<pair<double,double>>& p2p = getP2P(p2p_arr);

				const vector<double>& prob = getProbability(prob_arr);

				if (p2p.size() == 0 || prob.size() == 0 || p2p.size() != prob.size()) {
					stringstream err_msg;
					err_msg << "p2p.size() = " << p2p.size() << " prob.size = " << prob.size() << " for index = " << i;
					LOG(ERROR) << err_msg.str();
					throw BadJSON(err_msg.str());
				}

				line_info_.push_back(move(make_pair(move(p2p), move(prob))));
			}
#ifdef MBF_FUNCTIONAL_TESTING
			line_target_.Stats();
#endif
		}

		double Find(const char* const *input, int sz, double minp2p, double cp, double p2p, double platform_fee)
		{
			DCHECK(input != NULL && sz >= 0 && minp2p <= p2p && p2p <= cp && platform_fee <= 1.0);

			// Find all line_items matching the input characteristics
			const vector<uint16_t>& matching_li = line_target_.Find(input, sz);
			if (matching_li.empty()) {
				LOG(WARNING) << "No matching line_items for incoming impression found";
				return p2p;
			}
#ifdef MBF_FUNCTIONAL_TESTING
			fprintf(stderr, "\nMatching line items are : ");
			for (auto i = 0U; i != matching_li.size(); i++) {
				fprintf(stderr, "%u, ", matching_li[i]);
			}
#endif

			// Filter and assign p2p from [p2p_left, p2p_right] to line_items
			//pair<vector<double>, vector<double>> selected_li = select(matching_li, minp2p, cp, p2p);
			const vector<pair<double, double>>& selected_li = select(matching_li, minp2p, cp, p2p);
			if (selected_li.empty()) {
				LOG(WARNING) << "Matching line items found but all got filtered because they are lying outside the interval [" << minp2p << ", " << cp << "]";
				return p2p;
			}
			/*
			 *
				 auto size = selected_li.first.size();
				 if (size != selected_li.second.size()) {
				 LOG(ERROR) << "p2p size(" << size << ") != prob.size(" << selected_li.second.size() << ")";
				 return p2p;
				 }
				 */
			return calculateP2P(selected_li, cp, platform_fee, p2p);
		}
#ifdef MBF_INTEGRATION_TESTING
		vector<double> FindProb(const char* const *input, int sz, double minp2p, double cp, double p2p, double platform_fee)
		{
			(void) minp2p;
			(void) cp;
			(void) platform_fee;

			DCHECK(input != NULL && sz >= 0 && p2p >= 0.0);

			// Find all line_items matching the input characteristics
			const vector<uint16_t>& matching_li = line_target_.Find(input, sz);
			if (matching_li.empty()) {
				LOG(WARNING) << "No matching line_items for incoming impression found";
				return vector<double>();
			}

			// Filter and assign p2p from [p2p_left, p2p_right] to line_items
			//pair<vector<double>, vector<double>> selected_li = select(matching_li, minp2p, cp, p2p);
			const vector<double>& prob = selectProb(matching_li, p2p);
			if (prob.empty()) {
				LOG(WARNING) << matching_li.size() << " matching line items found but there is none which overlaps p2p " << p2p;
				for (auto i = 0U; i != matching_li.size(); i++) {
#if __cplusplus >= 201103L
					LOG(INFO) << to_string(matching_li[i]);
#else
					LOG(INFO) << to_string(static_cast<long long>(matching_li[i]));
#endif
				}
			}
			return prob;
		}
#endif

};
#endif
