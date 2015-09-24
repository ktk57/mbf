#ifndef __MBF_H__
#define __MBF_H__

//#include <vector>
//#include <string>
//#include "cmph.h"
#include "utils/db.h"
#include "utils/impr.h"
#if defined(MBF_BENCHMARK) || defined(MBF_INTEGRATION_TESTING)
#include <vector>
#endif
//using namespace std;
//	void build_and_test_info(const vector<string>&, const char*, int);
//
#ifdef MBF_INTEGRATION_TESTING
std::vector<double> GetProb(const char* const *input, int sz, int pub_id, double minp2p, double cp, double p2p, double platform_fee);
#endif

#ifdef __cplusplus 
extern "C" {
#endif

//const vector<string> g_targeting_attr {"sid", "adid", "adsize", "gctry", "greg", "gdma", "shr", "os", "browser", "infrm", "lang", "tz", "ufp", "sfp", "platform", "dc", "it", "gcity"};
//void init_impr_info(struct impr_info *in);

void InitMBF(const char *name);

void UpdateMBF(struct db_info** info);

double GetP2P(char** input, int sz, int pub_id, double minp2p, double cp, double p2p, double platform_fee);

double GetPF(int pub_id);

void UpdatePF(struct db_info **conn);

void UpdateOSList(struct db_info **conn);

void UpdateBrowserList(struct db_info **conn);

void DestroyImpr(struct impr_info *impr);


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
		);

void Language(const char *in_language, char *out_language, int max_len);

int PSTHour();
int IsValidTimeZone(const char *timezone);
const char* GetOS(const char *user_agent);
const char* GetBrowser(const char *user_agent);
//sid, adid, adsize(widthxheight), gctry, greg, gcity, gdma, shr, os, browser, infrm, lang, tz, ufp, sfp, platform, dc, it
//os, browser, shr, tz


#ifdef __cplusplus 
}
#endif
#endif
