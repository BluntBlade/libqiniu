#ifndef __QN_HTTP_CONNECTION_H__
#define __QN_HTTP_CONNECTION_H__

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/http/header.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Declaration of HTTP Connection (Abbreviation: http_conn) ==== */

struct _QN_HTTP_CONNECTION;
typedef struct _QN_HTTP_CONNECTION * qn_http_connection_ptr;

/* == Constructor & Destructor methods == */

QN_SDK extern qn_http_connection_ptr qn_http_conn_create(void);
QN_SDK extern void qn_http_conn_destroy(qn_http_connection_ptr restrict conn);

/* == Get methods == */

QN_SDK extern qn_uint qn_http_conn_get_code(qn_http_connection_ptr restrict conn);
QN_SDK extern qn_string qn_http_conn_get_version(qn_http_connection_ptr restrict conn);
QN_SDK extern qn_string qn_http_conn_get_message(qn_http_connection_ptr restrict conn);

/* == Action methods == */

QN_SDK extern qn_bool qn_http_conn_get(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);

QN_SDK extern qn_bool qn_http_conn_post(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_io_reader_itf restrict req_body, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);
QN_SDK extern qn_bool qn_http_conn_post_form(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_http_form_ptr restrict req_body, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);
QN_SDK extern qn_bool qn_http_conn_post_buffer(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, const char * restrict req_body, qn_fsize req_body_size, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);

/* == Check Functions == */

QN_SDK extern qn_bool qn_http_failed_in_communication(void);

#ifdef __cplusplus
}
#endif

#endif /* __QN_HTTP_CONNECTION_H__ */

