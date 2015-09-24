#include <cassert>
#include "mbf.h"
//#include "cmph.h"
//#include "utils/util.h"
//#include "utils/targeting.h"
//#include "json.h"
//#include <vector>
//#include "glog/logging.h"
#include "mbftable.h"
#include "platformfeetable.h"
/*
#include "utils/db.h"
*/
//#include "db_mbf.h"
//using namespace std;

extern class MBFTable g_mbf_table;
extern class StringTable g_os_table;
extern class StringTable g_browser_table;
extern class PlatformFeeTable g_pf_table;
//extern char *g_fatal_log_file;

#define UNKNOWN "UNKNOWN"
#if 0

/*
const char *g_req_attr_name[] = {"sid", "adid", "adsize", "gctry", "greg", "gdma", "shr", "os", "browser", "infrm", "lang", "tz", "ufp", "sfp", "platform", "dc", "it", "gcity"};
int g_req_attr_vec_size = sizeof(g_req_attr_name) / sizeof(char*);
*/


vector<string> g_req_attr_name {"sid", "adid", "adsize", "gctry", "greg", "gdma", "shr", "os", "browser", "infrm", "lang", "tz", "ufp", "sfp", "platform", "dc", "it", "gcity"};
//int g_req_attr_vec_size = sizeof(g_req_attr_name) / sizeof(char*);

const char *test[] = {"5", "5", "728x9", "CH", "NY", NULL, "23", "iOS", "safari", "1", "en-US", "+5.5", "1", "1", "1"/*it*/, "2", NULL, NULL};
//const char *test[] = {"5", "56600", "728x9", "CH", "NY", NULL, "23", "iOS", "safari", "1", "en-US", "+5.5", "1", "1", "1"/*it*/, "2", NULL, "Zumikon"};

/*
static cmph_t *get_hash_func(char** buf, int size) {

	struct vec_str vs;
	init_vec_str(&vs, g_req_attr_name, g_req_attr_vec_size);
	struct cmph_io_adapter_t source = {.data = (void*) &vs, .nkeys = vs.size, .read = vec_str_read, .dispose = vec_str_dispose, .rewind = vec_str_rewind};
	cmph_config_t *config = cmph_config_new(&source);

	if (config == NULL) {
		return NULL;
	}

	cmph_config_set_algo(config, CMPH_CHD);
	cmph_config_set_verbosity(config, 1);
	//cmph_config_set_mphf_fd(config, mphf_fd);
	cmph_t *hash = cmph_new(config);
	fprintf(stderr, "\nSize = %u\n", hash == NULL? "null:0":cmph_size(hash));
	cmph_config_destroy(config);
	return hash
}
*/
void build_and_test_info(const vector<string>& v, const char *target, int size)
{
#if __cplusplus == 201103L
	fprintf(stderr, "\n%ld\n", __cplusplus);
#else
	fprintf(stderr, "\n%d\n", __cplusplus);
#endif
	(void) size;
	json_object *obj = json_tokener_parse(target);
	if ((obj == NULL) || (is_error(obj))) {
		fprintf(stderr, "\nERROR parsing the json : %s %s:%s:%d\n", target, __FILE__, __func__, __LINE__);
		return;
	}

	json_object *li = getFromObject(obj, "li", json_type_array);
	if ((li == NULL) || (is_error(li))) {
		fprintf(stderr, "\nERROR parsing the json : %s %s:%s:%d\n", target, __FILE__, __func__, __LINE__);
		return;
	}

	json_object *prob = getFromObject(obj, "prob", json_type_array);
	if ((prob == NULL) || (is_error(prob))) {
		fprintf(stderr, "\nERROR parsing the json : %s %s:%s:%d\n", target, __FILE__, __func__, __LINE__);
		return;
	}

	json_object *p2p = getFromObject(obj, "p2p", json_type_array);
	if ((p2p == NULL) || (is_error(p2p))) {
		fprintf(stderr, "\nERROR parsing the json : %s %s:%s:%d\n", target, __FILE__, __func__, __LINE__);
		return;
	}

	int sz = json_object_array_length(li);
	assert(sz == json_object_array_length(p2p));
	assert(sz == json_object_array_length(prob));

	try {
		Targeting f(v, li, sz, "keys", "values");
		f.debugPrint();
		f.stats();
		const auto& val = f.Find(test, sizeof(test)/sizeof(char*));
		fprintf(stderr, "\nPrinting the selected line items %s:%s:%d\n", __FILE__, __func__, __LINE__);
		for (int i = 0; i != static_cast<int>(val.size()); i++) {
			fprintf(stderr, "%u, ", val[i]);
		}
		fprintf(stderr, "\n");
	} catch(const BadJSON& b) {
		fprintf(stderr, "%s\n", b.what());
		return;
	}

	/*
	if (f.parseJSONArray(li, sz, "keys", "values") != 0) {
		fprintf(stderr, "\nERROR parseJSON failed %s:%s:%d\n", __FILE__, __func__, __LINE__);
		return;
	}
	*/
}
#endif
//100001100001011011000000000011000001000000000110000010000111000000000000000000000010000111110000000000000011100000000000010000010000000000000001
//100001100001011011000000000011000001000000000110000010000111000000000000000000000010000111110000000000000011100000000000010000010000000000000001
//0, 5, 6, 11, 13, 14, 16, 17, 28, 29, 35, 45, 46, 52, 57, 58, 59, 82, 87, 88, 89, 90, 91, 106, 107, 108, 121, 127, 143,
//0, 5, 6, 11, 13, 14, 16, 17, 28, 29, 35, 45, 46, 52, 57, 58, 59, 82, 87, 88, 89, 90, 91, 106, 107, 108, 121, 127, 143,
void UpdateMBF(struct db_info **conn)
{
	if (conn == NULL) {
		fprintf(stderr, "\nERROR UpdateMBF() passed NULL %s:%d\n", __FILE__, __LINE__);
		return;
	}
	g_mbf_table.Update(conn);
}

double GetPF(int pub_id)
{
	return g_pf_table.Find(pub_id);
}

void UpdatePF(struct db_info **conn)
{
	if (conn == NULL) {
		fprintf(stderr, "\nERROR UpdatePF() passed NULL %s:%d\n", __FILE__, __LINE__);
		return;
	}
	g_pf_table.Update(conn);
}

double GetP2P(char** input, int sz, int pub_id, double minp2p, double cp, double p2p, double platform_fee)
{
	return g_mbf_table.Find(input, sz, pub_id, minp2p, cp, p2p, platform_fee);
}

#ifdef MBF_INTEGRATION_TESTING
vector<double> GetProb(const char* const *input, int sz, int pub_id, double minp2p, double cp, double p2p, double platform_fee)
{
	return g_mbf_table.FindProb(input, sz, pub_id, minp2p, cp, p2p, platform_fee);
}
#endif

void UpdateOSList(struct db_info **conn)
{
	if (conn == NULL) {
		fprintf(stderr, "\nERROR UpdateOSList() passed NULL %s:%d\n", __FILE__, __LINE__);
		return;
	}
	g_os_table.Update(conn);
}

void UpdateBrowserList(struct db_info **conn)
{
	if (conn == NULL) {
		fprintf(stderr, "\nERROR UpdateBrowserList() passed NULL %s:%d\n", __FILE__, __LINE__);
		return;
	}
	g_browser_table.Update(conn);
}


const char* GetOS(const char *user_agent)
{
	return g_os_table.Find(user_agent);
}

const char* GetBrowser(const char *user_agent)
{
	return g_browser_table.Find(user_agent);
}


	/*
	struct {
		cmph_t *hfunc;
	};
	*/
void InitMBF(
		const char *name
		)
{
	FLAGS_logtostderr = 1;
	//google::SetLogDestination(google::GLOG_ERROR, g_fatal_log_file);
	//google::SetLogFilenameExtension("");
	google::InitGoogleLogging(name);
}

/*
static void err_sprintf()
{
	LOG(ERROR) << "buffer overflow";
}
*/
//sid, adid, adsize(widthxheight), gctry, greg, gcity, gdma, shr, os, browser, infrm, lang, tz, ufp, sfp, platform, dc, it

// user-agent
static void convert_to_lower(const char *src, char *target, int target_size)
{
        int i = 0;
        if(src == NULL || target == NULL){
                return;
        }
        while(src[i] != '\0' && (i < target_size - 1)){
                target[i] = tolower(src[i]);
                ++i;
        }
        target[i] = '\0';
}

static void fill_info(
		int site_id,
		int ad_id,
		int ad_width,
		int ad_height,
		const char *ctry,
		const char *region,
		const char *city,
		int dma,
		int srvr_hour,
		const char *os,
		const char *browser,
		int in_iframe,
		const char *lang,
		const char *timezone,
		int u_fold_pos,
		int s_fold_pos,
		int it,
		const char *dc_name,
		struct impr_info* out
		)
{
	//	int ret = 0;
	/*
		 ret = snprintf(out->input[SITE_ID_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", site_id);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << site_id;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[SITE_ID_IDX] = strdup(to_string(site_id).c_str());
#else
	out->input[SITE_ID_IDX] = strdup(to_string(static_cast<long long>(site_id)).c_str());
#endif

	/*
		 ret = snprintf(out->input[AD_ID_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", ad_id);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << ad_id;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[AD_ID_IDX] = strdup(to_string(ad_id).c_str());
#else
	out->input[AD_ID_IDX] = strdup(to_string(static_cast<long long>(ad_id)).c_str());
#endif

	/*
		 ret = snprintf(out->input[AD_SIZE_IDX], MAX_TARGETING_KEY_SIZE + 1, "%dx%d", ad_width, ad_height);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << ad_width << ", " << ad_height;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[AD_SIZE_IDX] = strdup((to_string(ad_width) + "x" + to_string(ad_height)).c_str());
#else
	out->input[AD_SIZE_IDX] = strdup((to_string(static_cast<long long>(ad_width)) + "x" + to_string(static_cast<long long>(ad_height))).c_str());
#endif

	/*
		 ret = snprintf(out->input[CTRY_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", ctry);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << ctry;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[CTRY_IDX] = strdup(ctry);
#else
	out->input[CTRY_IDX] = strdup(ctry);
#endif

	/*
		 ret = snprintf(out->input[REGION_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", region);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << region;
		 }
		 */

#if __cplusplus >= 201103L
	out->input[REGION_IDX] = strdup(region);
#else
	out->input[REGION_IDX] = strdup(region);
#endif

	/*
		 ret = snprintf(out->input[CITY_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", city);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << city;
		 }
		 */

#if __cplusplus >= 201103L
	out->input[CITY_IDX] = strdup(city);
#else
	out->input[CITY_IDX] = strdup(city);
#endif

	/*
		 ret = snprintf(out->input[DMA_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", dma);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << dma;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[DMA_IDX] = strdup(to_string(dma).c_str());
#else
	out->input[DMA_IDX] = strdup(to_string(static_cast<long long>(dma)).c_str());
#endif

	/*
		 ret = snprintf(out->input[SERVER_HOUR_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", srvr_hour);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << srvr_hour;
		 }
		 */

#if __cplusplus >= 201103L
	out->input[SERVER_HOUR_IDX] = strdup(to_string(srvr_hour).c_str());
#else
	out->input[SERVER_HOUR_IDX] = strdup(to_string(static_cast<long long>(srvr_hour)).c_str());
#endif

	/*
		 ret = snprintf(out->input[OS_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", os);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << os;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[OS_IDX] = strdup(os);
#else
	out->input[OS_IDX] = strdup(os);
#endif

	/*
		 ret = snprintf(out->input[BROWSER_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", browser);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << browser;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[BROWSER_IDX] = strdup(browser);
#else
	out->input[BROWSER_IDX] = strdup(browser);

#endif
	/*
		 ret = snprintf(out->input[IN_IFRAME_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", in_iframe);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << in_iframe;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[IN_IFRAME_IDX] = strdup(to_string(in_iframe).c_str());
#else
	out->input[IN_IFRAME_IDX] = strdup(to_string(static_cast<long long>(in_iframe)).c_str());

#endif
	/*
		 ret = snprintf(out->input[LANGUAGE_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", lang);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << lang;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[LANGUAGE_IDX] = strdup(lang);
#else
	out->input[LANGUAGE_IDX] = strdup(lang);

#endif
	/*
		 ret = snprintf(out->input[TIMEZONE_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", timezone);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << timezone;
		 }
		 */

#if __cplusplus >= 201103L
	out->input[TIMEZONE_IDX] = strdup(timezone);
#else
	out->input[TIMEZONE_IDX] = strdup(timezone);
#endif

	/*
		 ret = snprintf(out->input[U_FOLD_POS_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", u_fold_pos);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << u_fold_pos;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[U_FOLD_POS_IDX] = strdup(to_string(u_fold_pos).c_str());
#else
	out->input[U_FOLD_POS_IDX] = strdup(to_string(static_cast<long long>(u_fold_pos)).c_str());
#endif

	/*
		 ret = snprintf(out->input[S_FOL_POS_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", s_fold_pos);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << s_fold_pos;
		 }
		 */

#if __cplusplus >= 201103L
	out->input[S_FOL_POS_IDX] = strdup(to_string(s_fold_pos).c_str());
#else
	out->input[S_FOL_POS_IDX] = strdup(to_string(static_cast<long long>(s_fold_pos)).c_str());
#endif

	/*
		 ret = snprintf(out->input[PLATFORM_IDX], MAX_TARGETING_KEY_SIZE + 1, "%d", platform);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << platform;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[IMPR_TYPE_IDX] = strdup(to_string(it).c_str());
#else
	out->input[IMPR_TYPE_IDX] = strdup(to_string(static_cast<long long>(it)).c_str());
#endif

	/*
		 ret = snprintf(out->input[DC_NAME_IDX], MAX_TARGETING_KEY_SIZE + 1, "%s", dc_name);
		 if (ret >= MAX_TARGETING_KEY_SIZE + 1) {
		 LOG(ERROR) << "Buffer overflow detected for " << dc_name;
		 }
		 */
#if __cplusplus >= 201103L
	out->input[DC_NAME_IDX] = strdup(dc_name);
#else
	out->input[DC_NAME_IDX] = strdup(dc_name);
#endif
}

#define MAX_USER_AGENT_MBF (512 + 1)
#define MAX_LANG_MBF (512 + 1)

void GetImprInfo(
		int site_id,
		int ad_id,
		int ad_width,
		int ad_height,
		const char *in_ctry,
		const char *in_region,
		const char *in_city,
		int dma,
		const char *in_ua,
		int in_iframe,
		const char *in_al,
		const char *in_tz,
		int u_fold_pos,
		int s_fold_pos,
		int it,
		const char *dc_name,
		struct impr_info* out
		)
{
	if (out == NULL) {
		LOG(ERROR) << "out(type struc impr_info) is NULL)";
		return;
	}

	const char *ctry = UNKNOWN;
	const char *region = UNKNOWN;
	const char *city = UNKNOWN;

	if (in_ctry != NULL) {
		ctry = in_ctry;
	}
	if (in_region != NULL) {
		region = in_region;
	}
	if (in_city != NULL) {
		city = in_city;
	}

	int pst_hour = PSTHour();
	char user_agent[MAX_USER_AGENT_MBF];
	user_agent[0] = '\0';
	char lang[MAX_LANG_MBF];
	lang[0] = '\0';
	// lower case language
	char lc_lang[MAX_LANG_MBF];
	lc_lang[0] = '\0';

	const int TEMP_BUFF = 32;
	char timezone[TEMP_BUFF];
	timezone[0] = '\0';

	if (IsValidTimeZone(in_tz)) {
		char *pEnd = NULL;
		double temp = strtod(in_tz, &pEnd);
		int ret = snprintf(timezone, TEMP_BUFF, "%.2f", temp);
		if (ret >= TEMP_BUFF) {
			timezone[0] = '\0';
		}
	}

	lang[0] = '\0';

	convert_to_lower(in_ua, user_agent, MAX_USER_AGENT_MBF);

	// Ajay wasn't converting lang to lower as his implementation is case insesitive
	convert_to_lower(in_al, lc_lang, MAX_LANG_MBF);

	Language(lc_lang, lang, MAX_LANG_MBF);

	const char* os = NULL;
	const char* browser = NULL;

	if (user_agent[0] != '\0') {
		os = GetOS(user_agent);
		browser = GetBrowser(user_agent);
	}

	if (os == NULL) {
		os = UNKNOWN;
	}

	if (browser == NULL) {
		browser = UNKNOWN;
	}

	fill_info(
			site_id,
			ad_id,
			ad_width,
			ad_height,
			ctry,
			region,
			city,
			dma,
			//pst_hour == -1? UNKNOWN:pst_hour,
			pst_hour, // no error checking done
			os,
			browser,
			in_iframe,
			lang[0] == '\0'?UNKNOWN:lang,
			timezone[0] == '\0'?UNKNOWN:timezone,
			u_fold_pos,
			s_fold_pos,
			it,
			dc_name == NULL?UNKNOWN:dc_name,
			out
			);
}


void DestroyImpr(struct impr_info *impr)
{
	for (int j = 0; j < MAX_TARGETING_PARAMS; j++) {
		free(impr->input[j]);
	}
}

void Language(const char *in_language, char *out_language, int max_len)
{
	int i = 0;
	while(in_language[i] != '\0' && i < max_len - 1){
		switch(in_language[i]) {
			case ';':
			case '_':
			case ',':
				out_language[i] = '\0';
				return;
			default :
				out_language[i] = in_language[i];
				break;
		}
		++i;
	}
	out_language[i] = '\0';
}

int PSTHour()
{
	time_t rawtime;
	struct tm *tmp;
	struct tm result;
	memset(&result, 0, sizeof(tm));

	time(&rawtime);

	tmp = gmtime_r(&rawtime, &result);
	if (tmp == NULL) {
		LOG(ERROR) << "gmtime_r return NULL";
		return -1;
	}
	return result.tm_hour ;
}

/*
 * if (IsValidTimeZone(in_req_params->timezone) {
	 time_zone = strtod (in_req_params->timezone, &pEnd);
	 sprintf(timezone, "%.2f", time_zone);
	 }
	 */

int IsValidTimeZone(const char *timezone){
	if (timezone == NULL) {
		return false;
	}

	int decimal = 0;
	int digit = 0;
	int i = 0;
	switch (timezone[i]){
		case '+':
		case '-':
			break;
		case '.':
			++decimal;
			break;
		default :
			if(!isdigit(timezone[0])){
				return 0;
			}
			++digit;
			break;
	}
	++i;
	while(timezone[i]){
		switch (timezone[i]){
			case '.':
				decimal++;
				if(decimal > 1){
					return 0;
				}
				break;
			default:
				if(!isdigit(timezone[i])){
					return 0;
				}
				++digit;
				break;

		}
		++i;

	}
	if(digit == 0){
		return 0;
	}
	return 1;
}



//sid, adid, adsize(widthxheight), gctry, greg, gcity, gdma, shr, os, browser, infrm, lang, tz, ufp, sfp, platform, dc, it
//os, browser, shr, tz
