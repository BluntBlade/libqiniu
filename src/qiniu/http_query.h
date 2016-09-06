#ifndef __QN_HTTP_QUERY_H__
#define __QN_HTTP_QUERY_H__

#include "qiniu/base/ds/etable.h"
#include "qiniu/base/string.h"

// ---- Declaration of HTTP query ----

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_HTTP_QUERY;
typedef struct _QN_HTTP_QUERY * qn_http_query_ptr;

extern qn_http_query_ptr qn_http_qry_create(void);
extern void qn_http_qry_destroy(qn_http_query_ptr qry);
extern void qn_http_qry_reset(qn_http_query_ptr qry);

extern qn_bool qn_http_qry_set_string(qn_http_query_ptr qry, const char * restrict key, int key_size, const char * restrict val, int val_size);
extern qn_bool qn_http_qry_set_integer(qn_http_query_ptr qry, const char * restrict key, int key_size, int value);

extern qn_string qn_http_qry_to_string(qn_http_query_ptr qry);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_QUERY_H__