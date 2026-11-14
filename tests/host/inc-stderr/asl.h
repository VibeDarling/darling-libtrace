#pragma once
// Stand-ins for libasl: asl_log prints what the daemon would receive, on stdout, so a test sees both streams.
typedef void *asl_object_t;
typedef void *aslmsg;
#define ASL_TYPE_MSG 0
#define ASL_LEVEL_ALERT 1
#define ASL_LEVEL_ERR 3
#define ASL_LEVEL_INFO 6
#define ASL_LEVEL_DEBUG 7
aslmsg asl_new(unsigned type);
int asl_set(aslmsg msg, const char *key, const char *value);
asl_object_t asl_open(const char *ident, const char *facility, unsigned opts);
int asl_log(asl_object_t client, aslmsg msg, int level, const char *format, ...);
void asl_release(aslmsg msg);
