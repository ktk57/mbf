#ifndef __DB_MBF_H__
#define __DB_MBF_H__
#include <vector>
#include <string>
//using namespace std;

const char* const queryMBF = "select pub_id, targeting_info from pub_mbf_info where on_off = 1 order by pub_id ASC";
const char* const queryOSList = "select os_name from os_list order by id ASC";
const char* const queryBrowserList = "select browser_name from browser_list order by id ASC";
const char* const queryPF = "select pub_id, platform_fee from pub_platform_fee order by pub_id ASC";

struct db_info;
const int MBF_TARGETING_JSON_SIZE = 10 * (1<<20);

std::vector<std::pair<int, std::string>> ReadTableMBF(struct db_info**, const char*);

std::vector<std::pair<int, double>>* ReadTablePF(struct db_info **conn, const char *query);
#endif
