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

typedef size_t (*qn_http_body_parse_ifn)(qn_http_body_itf restrict itf, char * restrict buf, size_t buf_size);
typedef const char * (*qn_http_body_mime_type_ifn)(qn_http_body_itf restrict itf);
typedef qn_fsize (*qn_http_body_size_ifn)(qn_http_body_itf restrict itf);

typedef struct _QN_HTTP_BODY
{
    qn_http_body_parse_ifn parse;

    qn_http_body_mime_type_ifn mime_type;   
    qn_http_body_size_ifn size;
} qn_http_body_st;

static inline size_t qn_http_body_parse(qn_http_body_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return (*itf)->parse(itf, buf, buf_size);
}

static inline const char * qn_http_body_mime_type(qn_http_body_itf restrict itf)
{
    return (*itf)->mime_type(itf);
}

static inline qn_fsize qn_http_body_size(qn_http_body_itf restrict itf)
{
    return (*itf)->size(itf);
}

#ifdef __cplusplus
}
#endif

#endif /* __QN_HTTP_BODY_H__ */

