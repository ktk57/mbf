#ifndef __IMPR_H__
#define __IMPR_H__
#ifdef __cplusplus 
extern "C" {
#endif

	/*
#ifdef MBF_FUNCTIONAL_TESTING
#define MAX_TARGETING_PARAMS (19)
#elif MBF_BENCHMARK
#define MAX_TARGETING_PARAMS (22)
#else
#define MAX_TARGETING_PARAMS (17)
#endif
*/
#if defined(MBF_BENCHMARK) || defined(MBF_INTEGRATION_TESTING)
#define MAX_TARGETING_PARAMS (21)
#else
#define MAX_TARGETING_PARAMS (17)
#endif
	//#define MAX_TARGETING_KEY_SIZE (255)
	enum {
		SITE_ID_IDX = 0,
		AD_ID_IDX,
		AD_SIZE_IDX,
		CTRY_IDX,
		REGION_IDX,
		CITY_IDX,
		DMA_IDX,
		SERVER_HOUR_IDX,
		OS_IDX,
		BROWSER_IDX,
		IN_IFRAME_IDX,
		LANGUAGE_IDX,
		TIMEZONE_IDX,
		U_FOLD_POS_IDX,
		S_FOL_POS_IDX,
		IMPR_TYPE_IDX,
		DC_NAME_IDX
#if defined(MBF_FUNCTIONAL_TESTING) || defined(MBF_INTEGRATION_TESTING)
		,P2P_IDX,
		MINP2P_IDX,
		CP_IDX,
		PF_IDX
#endif
	};

	struct impr_info {
		//	char input[MAX_TARGETING_PARAMS][MAX_TARGETING_KEY_SIZE + 1];
		char* input[MAX_TARGETING_PARAMS];
	};
#ifdef __cplusplus 
}
#endif
#endif
