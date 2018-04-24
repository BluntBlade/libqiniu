#ifndef __QN_HTTP_CONNECTION_H__
#define __QN_HTTP_CONNECTION_H__

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/io.h"
#include "qiniu/http/header.h"
#include "qiniu/http/body.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Declaration of HTTP Form (Abbreviation: http_form) ==== */

struct _QN_HTTP_FORM;
typedef struct _QN_HTTP_FORM * qn_http_form_ptr;

/* == Constructor & Destructor methods == */

QN_SDK extern qn_http_form_ptr qn_http_form_create(void);
QN_SDK extern void qn_http_form_destroy(qn_http_form_ptr restrict form);
QN_SDK extern void qn_http_form_reset(qn_http_form_ptr restrict form);

/* == Add methods == */

QN_SDK extern qn_bool qn_http_form_add_raw(qn_http_form_ptr restrict form, const char * restrict field, qn_size field_size, const char * restrict value, qn_size value_size);

static inline qn_bool qn_http_form_add_text(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict value, qn_size value_size)
{
    return qn_http_form_add_raw(form, field, posix_strlen(field), value, value_size);
}

static inline qn_bool qn_http_form_add_string(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict value)
{
    return qn_http_form_add_raw(form, field, posix_strlen(field), value, posix_strlen(value));
}

QN_SDK extern qn_bool qn_http_form_add_file(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_fsize fsize, const char * restrict mime_type);
QN_SDK extern qn_bool qn_http_form_add_file_reader(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_fsize fsize, const char * restrict mime_type, void * restrict req);
QN_SDK extern qn_bool qn_http_form_add_buffer(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict buf, qn_size buf_size, const char * restrict mime_type);

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

