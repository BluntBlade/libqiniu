#ifndef __QN_HTTP_BODY_H__
#define __QN_HTTP_BODY_H__

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Definition of HTTP Body Interface (Abbreviation: http_body) ==== */

struct _QN_HTTP_BODY;
typedef struct _QN_HTTP_BODY * qn_http_body_ptr;
typedef qn_http_body_ptr * qn_http_body_itf;

typedef size_t (*qn_http_body_parse_fn)(qn_http_body_itf restrict itf, char * restrict buf, size_t buf_size);

typedef struct _QN_HTTP_BODY
{
    qn_http_body_parse_fn parse;
} qn_http_body_st;

static inline size_t qn_http_body_parse(qn_http_body_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return (*itf)->parse(itf, buf, buf_size);
}

/* ==== Declaration of HTTP JSON-Body Parser (Abbreviation: http_json) ==== */

struct _QN_HTTP_JSON;
typedef struct _QN_HTTP_JSON * qn_http_json;

/* == Constructor & Destructor methods == */

QN_SDK extern qn_http_json_ptr qn_http_json_create(void);
QN_SDK extern void qn_http_json_destroy(qn_http_json_ptr restrict body);
QN_SDK extern void qn_http_json_reset(qn_http_json_ptr restrict body);

/* == Cast methods == */

QN_SDK extern qn_http_body_itf qn_http_json_to_body_interface(qn_http_json_ptr restrict body);

/* == Get methods == */

QN_SDK extern qn_json_object_ptr qn_http_json_get_object(qn_http_json_ptr restrict body);
QN_SDK extern qn_json_array_ptr qn_http_json_get_array(qn_http_json_ptr restrict body);

/* == Interface methods == */

QN_SDK extern size_t qn_http_json_parse(qn_http_json_ptr restrict body, char * restrict buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* __QN_HTTP_BODY_H__ */

