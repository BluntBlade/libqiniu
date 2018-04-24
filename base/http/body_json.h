#ifndef __QN_HTTP_BODY_JSON_H__
#define __QN_HTTP_BODY_JSON_H__

#include "qiniu/macros.h"
#include "base/http/body.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Declaration of HTTP JSON-Body Parser (Abbreviation: http_json) ==== */

struct _QN_HTTP_JSON;
typedef struct _QN_HTTP_JSON * qn_http_json_ptr;

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
QN_SDK extern const char * qn_http_json_mime_type(qn_http_body_itf restrict itf);
QN_SDK extern qn_fsize qn_http_json_size(qn_http_body_itf restrict itf);

#ifdef __cplusplus
}
#endif

#endif /* __QN_HTTP_BODY_JSON_H__ */

