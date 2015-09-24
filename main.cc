#include "mbf.h"
#include "glog/logging.h"
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <errno.h>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include "utils/db.h"
#include "mbf.h"
#include "utils/util.h"
using namespace std;
//extern const char *g_req_attr_name[];
const char *g_odbc_dsn_name1 = "KomliAdServer";
const char *g_odbc_dsn_name2 = "AdFlex";
const char *g_odbc_dsn_user_name = "kdbuser";
const char *g_odbc_dsn_user_password= "KdBuSeR12!";

extern vector<string> g_req_attr_name;
//vector<string> g_req_attr_name {"sid", "adid", "adsize", "gctry", "greg", "gdma", "shr", "os", "browser", "infrm", "lang", "tz", "ufp", "sfp", "platform", "dc", "it", "gcity"};
//extern int g_req_attr_vec_size;

/*
const char target[] = "{\"li\":[{\"keys\": [\"sid\", \"gctry\"], \"values\": [[\"3\", \"1\", \"2\"], [\"US\", \"IN\"]]}, {\"keys\": [\"browser\", \"gctry\"], \"values\": [[\"chrome\", \"safari\", \"firefox\"], [\"AU\", \"US\"]]}, {\"keys\": [], \"values\": []}, {\"keys\": [\"gctry\", \"browser\"], \"values\": [[], []]}], \"p2p\":[[\"1_5\", \"6_10\", \"10_50\", ], [\"10_50\", \"60_100\"], [\"100_200\"], [\"50_60\"]], \"prob\":[[\"10\", \"20\", \"30\"], [\"40\", \"90\"], [\"100\"], [\"90\"]]}";

const char target[] = "{\"li\":[{\"keys\": [\"sid\", \"gctry\"], \"values\": [[\"1\", \"2\", \"3\"], [\"US\", \"IN\"]]}, {\"keys\": \"browser\", \"values\": [[\"chrome\", \"safari\", \"firefox\"]]},  ], \"p2p\":[[\"1_5\", \"6_10\"], [\"10_50\", \"60_100\"]], \"prob\":[[\"10\", \"20\"], [\"40\", \"90\"]]}";

*/
/*
 * This thread runs in infinite loop and updates and does regular updates to global structures
 */

//output per thread
struct op {
#ifdef MBF_INTEGRATION_TESTING
	vector<vector<double>> vvd;
#elif defined(MBF_BENCHMARK)
	vector<double> vvd;
#endif
#if defined(MBF_INTEGRATION_TESTING) || defined(MBF_BENCHMARK)
	char padding_[64 - sizeof(vvd)];
#endif
	op(){}
	op(const op&) = delete;
	op& operator=(const op&) = delete;
};

op *g_out;

struct impr_list {
	int sz;
	impr_info imp[];
};

static void destroy(impr_list *list)
{
	int sz = list->sz;
	for (int i = 0; i < sz; i++) {
		char **imp = list->imp[i].input;
		for (int j = 0; j < MAX_TARGETING_PARAMS; j++) {
			free(imp[j]);
		}
	}
	free(list);
}

struct ctrl_params {
	int tid;
	int iterations;
	impr_info *imprs;
	int size;
};

/*
static void destroy(ctrl_params *params, int n)
{
	for (int i = 0; i < n; i++) {
		free(params->imprs);
		fclose(params->os);
		fclose(params->in);
	}
}
*/

struct update_ctx {
	struct db_info *adflex_db_handle;
	struct db_info *komliadserver_db_handle;
	// name of program
	const char *name;
	// in seconds
	int wait_time;
};

#define INITIAL_IMP_SIZE (50000)

/*
static int tokenize(
		char *str,
		struct impr_info *imp
		)
{
	char *ctx = NULL;
	char *token = strtok_r(str, "\t", &ctx);
	int idx = 0;
	while (token != NULL) {
		// TODO(@ktk57):- No checks here, this can cause buffer overflow
		//strcpy(imp->input[idx++], token);
		imp->input[idx++] = strdup(token);
		if (idx >= MAX_TARGETING_PARAMS) {
			break;
		}
		token = strtok_r(NULL, "\t", &ctx);
	}
	return idx;
}
*/
static int tokenize(
		char *str,
		struct impr_info *imp
		)
{
	if (str == NULL || str[0] == '\0') {
		return 0;
	}
	char *ctx = str;
	char *ptr = strchr(str, '\t');
	int idx = 0;
	while (ptr != NULL) {
		// TODO(@ktk57):- No checks here, this can cause buffer overflow
		//strcpy(imp->input[idx++], idx);
		*ptr = '\0';
		if (ctx[0] == '\0') {
			imp->input[idx++] = strdup("UNKNOWN");
		} else {
			imp->input[idx++] = strdup(ctx);
		}
		if (idx >= MAX_TARGETING_PARAMS) {
			return idx;
		}
		ctx = ptr + 1;
		ptr = strchr(ctx, '\t');
	}
	if (ctx[0] == '\0') {
		imp->input[idx++] = strdup("UNKNOWN");
	} else {
		imp->input[idx++] = strdup(ctx);
	}
	return idx;
}

static impr_list* getImprList(const char* path)
{
	int rc = 0;
	uint8_t *buf = NULL;
	int len = 0;

	rc = ReadFileToBuf(path, &buf, &len, true);

	if (rc != 0) {
		fprintf(stderr, "\nERROR ReadFileToBuf() for %s failed %s:%d\n", path, __FILE__, __LINE__);
		return NULL;
	}

	//unique_ptr<uint8_t[]> pbuf(buf, &free);

	int alloc_count = INITIAL_IMP_SIZE;

	struct impr_list *list = (struct impr_list*) malloc(sizeof(struct impr_list) + sizeof(struct impr_info) * alloc_count);

	if (list == NULL) {
		LOG(ERROR) << "malloc() failed for " << sizeof(struct impr_list) + sizeof(struct impr_info) * alloc_count;
		free(buf);
		return NULL;
	}

	int size = 0;

	char* ctx = NULL;
	char* token = strtok_r((char*) buf, "\n", &ctx);

	while (token != NULL) {

		if (size >= alloc_count) {
			alloc_count *= 2;
			struct impr_list *temp = (struct impr_list*) realloc(list, sizeof(struct impr_list) + sizeof(struct impr_info) * alloc_count);
			if (temp == NULL) {
				LOG(ERROR) << "realloc() failed for " << sizeof(struct impr_list) + sizeof(struct impr_info) * alloc_count;
				free(list);
				free(buf);
				return NULL;
			}
			list = temp;
		}

		/*
			 rc = sscanf(token, "%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s%[^\t]s", list->imp[size].field[0], list->imp[size].field[1], list->imp[size].field[2], list->imp[size].field[3], list->imp[size].field[4], list->imp[size].field[5], list->imp[size].field[6], list->imp[size].field[7], list->imp[size].field[8], list->imp[size].field[9], list->imp[size].field[10], list->imp[size].field[11], list->imp[size].field[12], list->imp[size].field[13]);
			 */
		/*
			 rc = sscanf(token, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t", list->imp[size].field[0], list->imp[size].field[1], list->imp[size].field[2], list->imp[size].field[3], list->imp[size].field[4], list->imp[size].field[5], list->imp[size].field[6], list->imp[size].field[7], list->imp[size].field[8], list->imp[size].field[9], list->imp[size].field[10], list->imp[size].field[11], list->imp[size].field[12], list->imp[size].field[13], list->imp[size].field[14], list->imp[size].field[15], list->imp[size].field[16], list->imp[size].field[17], list->imp[size].field[18]);
			 */
		rc = tokenize(token, &(list->imp[size]));

		if (rc != MAX_TARGETING_PARAMS) {
			LOG(ERROR) << "tokenize returned " << rc << " whereas # of targeting attributes = " << MAX_TARGETING_PARAMS << " for " << size << " impression";
			free(list);
			free(buf);
			return NULL;
		}

		/*
		for (int i = 0; i < MAX_TARGETING_PARAMS; i++) {
			if (list->imp[size].input[i] != NULL && strcmp(list->imp[size].input[i], " ") == 0) {
				list->imp[size].input[i][0] = '\0';
			}
		}
		*/
		size++;

		token = strtok_r(NULL, "\n", &ctx);
	}


	list->sz = size;
	free(buf);
	return list;
}




static void* update_(void *arg)
{
	struct update_ctx *ctx = reinterpret_cast<struct update_ctx*>(arg);

	// All Initialization functions go here BEGIN
	InitMBF(ctx->name);

	struct timespec wait;
	// All Initialization functions go here END
	while (true) {
		// Update the OS List
		UpdateOSList(&(ctx->adflex_db_handle));
		// Update the Browser List
		UpdateBrowserList(&(ctx->adflex_db_handle));

		// Update MBF
		UpdateMBF(&(ctx->adflex_db_handle));

		/*
		uint32_t timestamp = LastUpdateTimeStamp(&(ctx->adflex_db_handle),  "pub_mbf_info", "modification_time");
		DLOG(INFO) << "table modified on " << timestamp;
		*/

		// Ensure that we reset it every time to avoid problems due to interrupts
		memset(&wait, 0, sizeof(wait));
		wait.tv_sec = ctx->wait_time;
		nanosleep(&wait, NULL);
	}

	return NULL;
}

static int init_update_thread(const char *name)
{
	pthread_t tid;

	struct update_ctx *ctx = (update_ctx*) malloc(sizeof(update_ctx));
	if (ctx == NULL) {
		fprintf(stderr, "\nERROR malloc() failed %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	struct db_info *handle1;
	handle1 = init_db_info(g_odbc_dsn_name1, g_odbc_dsn_user_name, g_odbc_dsn_user_password);

	if (handle1 == NULL) {
		free(ctx);
		fprintf(stderr, "\nERROR init_db_info() failed for (dsn_name, dsn_user_name, dsn_password) = (%s, %s, %s)", g_odbc_dsn_name1, g_odbc_dsn_user_name, g_odbc_dsn_user_password);
		return -1;
	}

	struct db_info *handle2;
	handle2 = init_db_info(g_odbc_dsn_name2, g_odbc_dsn_user_name, g_odbc_dsn_user_password);

	if (handle2 == NULL) {
		fprintf(stderr, "\nERROR init_db_info() failed for (dsn_name, dsn_user_name, dsn_password) = (%s, %s, %s)", g_odbc_dsn_name2, g_odbc_dsn_user_name, g_odbc_dsn_user_password);

		destroy_db_info(handle1);
		free(ctx);
		return -1;
	}

	ctx->adflex_db_handle = handle2;
	ctx->komliadserver_db_handle = handle1;

	ctx->wait_time = 1800;
	ctx->name = name;

	int rc = pthread_create(&tid, NULL, update_, (void*)ctx);
	if (rc != 0) {
		destroy_db_info(handle1);
		destroy_db_info(handle2);
		free(ctx);
		fprintf(stderr, "\nERROR pthread_create() failed with rc = %d %s:%d\n", rc, __FILE__, __LINE__);
	}

	return rc;
}

static void* perf(void *arg)
{
	const ctrl_params *params = (const ctrl_params*) arg;

	int tid = params->tid;
	int iterations = params->iterations;
	int size = params->size;
	const impr_info *impr = params->imprs;
	op &out = g_out[tid];

	for (int j = 0; j < iterations; j++) {
		for (int i = 0; i < size; i++) {
			DLOG(INFO) <<"********Checking for impression*******" << i;
#ifdef MBF_INTEGRATION_TESTING
			//out.vvd.push_back(GetProb(impr[i].input, MAX_TARGETING_PARAMS - 1, 57, 0.1, 1.0, stod(impr[i].input[P2P_IDX]), 0.10));
			out.vvd.push_back(GetProb(impr[i].input, MAX_TARGETING_PARAMS - 4, 57, stod(impr[i].input[MINP2P_IDX]), stod(impr[i].input[CP_IDX]), stod(impr[i].input[P2P_IDX]), stod(impr[i].input[PF_IDX])));
#elif defined(MBF_BENCMARK)
			out.vvd.push_back(Find(impr[i].input, MAX_TARGETING_PARAMS - 4, stod(impr[i].input[MINP2P_IDX]), stod(impr[i].input[CP_IDX]), stod(impr[i].input[P2P_IDX]), stod(impr[i].input[PF_IDX])));
#else
			(void) impr;
			(void) out;
			(void) tid;
			(void) iterations;
			(void) size;
#endif
		}
	}
#ifndef NDEBUG
	fprintf(stderr, "\nINFO: Thread %d returning after processing %d impressions(each impression %d times)\n", params->tid, size, iterations);
#endif
	return NULL;
}

static void writeToFile(FILE *os, op *out, int sz)
{
#if defined(MBF_INTEGRATION_TESTING) || defined(MBF_BENCHMARK)
	for (int i = 0; i < sz; i++) {
		const auto& p = out[i].vvd;
		for (auto j = 0U; j != p.size(); j++) {
#ifdef MBF_INTEGRATION_TESTING
			for (auto k = 0U; k != p[j].size(); k++) {
				fprintf(os, "%lf\t", p[j][k]);
			}
#elif defined(MBF_BENCHMARK)
			fprintf(os, "%lf\t", p[j]);
#endif
			fprintf(os, "\n");
		}
	}
#else
	(void) os;
	(void) out;
	(void) sz;
#endif
}

int main(int argc, char *argv[])
{
	fprintf(stderr, "argc = %d\n", argc);
	if (argc != 5 && argc != 6) {
		fprintf(stderr, "\nERROR usage <exe> <# of threads> <# of iterations> <impr_file_path> <output_file_path> <tree_file_path> ..exiting\n");
		exit(1);
	}

	int threads = atoi(argv[1]);
	int iterations = atoi(argv[2]);
	const char* impr_file_path = argv[3];
	const char* output_file_path = argv[4];
	const char* tree_file_path = NULL;
	if (argc == 6) {
		tree_file_path = argv[5];
	}

	impr_list *imprs = getImprList(impr_file_path);
	if (imprs == NULL) {
		fprintf(stderr, "\nERROR getImprList(%s) failed", impr_file_path);
		exit(1);
	}

	FILE *os = fopen(output_file_path, "w");
	if (os == NULL) {
		fprintf(stderr, "\nERROR fopen(%s, w) failed", output_file_path);
		destroy(imprs);
		exit(1);
	}

	FILE *is = NULL;

	if (tree_file_path != NULL) {
		is = fopen(tree_file_path, "r");
		if (is == NULL) {
			fprintf(stderr, "\nERROR fopen(%s, r) failed", tree_file_path);
			destroy(imprs);
			fclose(os);
			exit(1);
		}
	}

	g_out = new op[threads];
	//(void) g_out;

	ctrl_params *params = (ctrl_params*) calloc(threads, sizeof(ctrl_params));
	if (params == NULL) {
		fprintf(stderr, "\nERROR calloc() failed");
		exit(1);
	}

	int size = imprs->sz;
	int impr_per_thread = size / threads;

	for (int i = 0; i < threads; i++) {
		params[i].imprs = &(imprs->imp[0]) + (i * impr_per_thread);
		if (i == threads - 1) {
			// assign the remaining # of impressions to the last thread
			params[i].size = size - (i * impr_per_thread);
		} else {
			// assign equal # of impressions to each thread
			params[i].size = impr_per_thread;
		}
		params[i].tid = i;
		params[i].iterations = iterations;
	}

	pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t) * threads);
	if (tid == NULL) {
		fprintf(stderr, "\nERROR malloc() failed");
		//destroy(params, threads);
		exit(1);
	}

	int ret = 0;

	fprintf(stderr, "\nInitializing update thread\n");

	ret = init_update_thread(argv[0]);
	if (ret != 0) {
		fprintf(stderr, "\nERROR init_update_thread() failed...exiting.. %s:%d\n", __FILE__, __LINE__);
		//destroy(&params);
		exit(1);
	}

	// Allow the update thread to read data from db
	struct timespec wait;
	memset(&wait, 0, sizeof(wait));
	wait.tv_sec = 5;

	fprintf(stderr, "\nWaiting for %ld seconds\n", wait.tv_sec);

	nanosleep(&wait, NULL);

	// For measuring time
	struct timespec t0;
	struct timespec t1;

	memset(&t0, 0, sizeof(t0));
	memset(&t1, 0, sizeof(t1));

	clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

	// Spawn the load generating threads
	for (auto i = 0; i < threads; i++) {
		pthread_create(&tid[i], NULL, perf, &params[i]);
	}

	for (int i = 0; i < threads; i++) {
		// No error handling
		pthread_join(tid[i], NULL);
	}

	// No error handling

	clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

	double before = t0.tv_sec * 1000000.0 + t0.tv_nsec / 1000.0;
	double after = t1.tv_sec * 1000000.0 + t1.tv_nsec / 1000.0;

	double diff_ms = after - before;

	LOG(INFO) << "Tested " << imprs->sz << " impressions using " << threads << " thread[/s] for " << iterations << " times on a tree in " << diff_ms << " microseconds(Discounting time for populating Data Structures from files/db)";

	writeToFile(os, g_out, threads);

	free(params);
	free(tid);
	destroy(imprs);

	fclose(os);
	if (is != NULL) {
		fclose(is);
	}

	/*
		 if (argc != 2) {
		 LOG(ERROR) << "Usage <exe> <path_of_json_file>";
		 return -1;
		 }

	// TODO(@ktk57): move this to a util function
	FILE *f = fopen(argv[1], "r");

	if (f == NULL) {
	LOG(ERROR) << "Unable to open file : " << argv[1];
	return -1;
	}

	int rval = fseek(f, 0L, SEEK_END);
	if (rval < 0) {
	LOG(ERROR) << "Unable to seek file : " << argv[1] << " to the end, rc = " << rval << " errno = " << errno;
	return -1;
	}

	long sz = ftell(f);
	rewind(f);

	//std::string target;
	//target.reserve(sz);
	///std::unique_ptr target {new char[sz + 1]};

	//char *target = new char[sz + 1];
	unique_ptr<char[]> ptarget{new char[sz + 1]};
	rval = fread(ptarget.get(), sizeof(char), sz, f);
	if (rval != sz) {
	LOG(ERROR) << "Unable to read file " << argv[1] << " to memory";
	return -1;
	}

	ptarget[sz] = '\0';

*/

	//fprintf(stderr, "\n %ld, %s\n", sz, target);
	//DLOG(INFO) << "Json size :\n" << sz << "\n" << "value: " << target;

	/*
		 bool b = google::SendEmail("kartik.mahajan@pubmatic.com", "TEst email from glog", "Wow I can't believe this");
		 if (b) {
		 LOG(INFO) << "Email Sent";
		 }
		 */
	/*
	 *
	 */
	//build_info(g_req_attr_name, g_req_attr_vec_size, target, sizeof(target) - 1);
	//	vector<string> g_req_attr_name {"sid", "adid", "adsize", "gctry", "greg", "gdma", "shr", "os", "browser", "infrm", "lang", "tz", "ufp", "sfp", "platform", "dc", "it", "gcity"};
	//	fprintf(stderr, "\nPrinting data\n");
	//	for (auto i = 0U; i != g_req_attr_name.size(); i++) {
	//		fprintf(stderr, "%s\n", g_req_attr_name[i].c_str());
	//	}
	//build_and_test_info(g_req_attr_name, ptarget.get(), sz);

	/*
		 struct timespec wait;
		 memset(&wait, 0, sizeof(wait));
		 wait.tv_sec = 25;
		 nanosleep(&wait, NULL);
		 LOG(INFO) << "main thread exiting";
		 return 0;
		 */
	return 0;

}
