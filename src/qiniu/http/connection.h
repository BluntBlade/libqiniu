#ifndef __QN_HTTP_CONNECTION_H__
#define __QN_HTTP_CONNECTION_H__

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/http/header.h"
#include "qiniu/service_selector.h"
#include "qiniu/os/file.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of body reader and writer

typedef size_t (*qn_http_body_reader_callback_fn)(void * restrict reader, char * restrict buf, size_t size);
typedef size_t (*qn_http_data_writer_callback_fn)(void * restrict writer, char * restrict buf, size_t size);

// ----

struct _QN_HTTP_JSON_WRITER;
typedef struct _QN_HTTP_JSON_WRITER * qn_http_json_writer_ptr;

QN_SDK extern qn_http_json_writer_ptr qn_http_json_wrt_create(void);
QN_SDK extern void qn_http_json_wrt_destroy(qn_http_json_writer_ptr restrict writer);

QN_SDK extern void qn_http_json_wrt_prepare(qn_http_json_writer_ptr restrict writer, qn_json_object_ptr * restrict obj, qn_json_array_ptr * restrict arr);
QN_SDK extern size_t qn_http_json_wrt_write_cfn(void * restrict writer, char * restrict buf, size_t buf_size);

// ---- Declaration of HTTP request ----

struct _QN_HTTP_REQUEST;
typedef struct _QN_HTTP_REQUEST * qn_http_request_ptr;

QN_SDK extern qn_http_request_ptr qn_http_req_create(void);
QN_SDK extern void qn_http_req_destroy(qn_http_request_ptr restrict req);
QN_SDK extern void qn_http_req_reset(qn_http_request_ptr restrict req);

// ----

QN_SDK extern const char * qn_http_req_get_header(qn_http_request_ptr restrict req, const char * restrict hdr);

QN_SDK extern qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr restrict req, const char * restrict hdr, const char * restrict val1, const char * val2, ...);
QN_SDK extern qn_bool qn_http_req_set_header(qn_http_request_ptr restrict req, const char * restrict hdr, const char * restrict value);
QN_SDK extern void qn_http_req_unset_header(qn_http_request_ptr restrict req, const char * restrict hdr);

// ----

QN_SDK extern qn_http_form_ptr qn_http_req_prepare_form(qn_http_request_ptr restrict req);
QN_SDK extern qn_http_form_ptr qn_http_req_get_form(qn_http_request_ptr restrict req);
QN_SDK extern void qn_http_req_set_form(qn_http_request_ptr restrict req, qn_http_form_ptr restrict form);

// ----

QN_SDK extern void qn_http_req_set_body_data(qn_http_request_ptr restrict req, const char * restrict body_data, qn_size body_size);
QN_SDK extern void qn_http_req_set_body_reader(qn_http_request_ptr restrict req, void * restrict body_reader, qn_http_body_reader_callback_fn body_reader_cb, qn_fsize body_size);
QN_SDK extern const char * qn_http_req_body_data(qn_http_request_ptr restrict req);
QN_SDK extern qn_fsize qn_http_req_body_size(qn_http_request_ptr restrict req);

// ---- Declaration of HTTP response ----

struct _QN_HTTP_RESPONSE;
typedef struct _QN_HTTP_RESPONSE * qn_http_response_ptr;

QN_SDK extern qn_http_response_ptr qn_http_resp_create(void);
QN_SDK extern void qn_http_resp_destroy(qn_http_response_ptr restrict resp);
QN_SDK extern void qn_http_resp_reset(qn_http_response_ptr restrict resp);

// ----

QN_SDK extern int qn_http_resp_get_code(qn_http_response_ptr restrict resp);
QN_SDK extern int qn_http_resp_get_writer_retcode(qn_http_response_ptr restrict resp);

// ----

QN_SDK extern qn_http_hdr_iterator_ptr qn_http_resp_get_header_iterator(qn_http_response_ptr restrict resp);
QN_SDK extern const char * qn_http_resp_get_header(qn_http_response_ptr restrict resp, const char * restrict hdr);

QN_SDK extern qn_bool qn_http_resp_set_header(qn_http_response_ptr restrict resp, const char * restrict hdr, const char * restrict val, qn_size val_size);
QN_SDK extern void qn_http_resp_unset_header(qn_http_response_ptr restrict resp, const char * restrict hdr);

// ----

QN_SDK extern void qn_http_resp_set_data_writer(qn_http_response_ptr restrict resp, void * restrict body_writer, qn_http_data_writer_callback_fn body_writer_cb);

/* ==== Declaration of HTTP Connection (Abbreviation: http_conn) ==== */

struct _QN_HTTP_CONNECTION;
typedef struct _QN_HTTP_CONNECTION * qn_http_connection_ptr;

/* == Constructor & Destructor methods == */

QN_SDK extern qn_http_connection_ptr qn_http_conn_create(void);
QN_SDK extern void qn_http_conn_destroy(qn_http_connection_ptr restrict conn);

/* == Get methods == */

QN_SDK extern int qn_http_conn_get_code(qn_http_connection_ptr restrict conn);

/* == Action methods == */

QN_SDK extern qn_bool qn_http_conn_get(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);

QN_SDK extern qn_bool qn_http_conn_post(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_io_reader_itf restrict req_body, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);
QN_SDK extern qn_bool qn_http_conn_post_form(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_http_form_ptr restrict req_body, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);
QN_SDK extern qn_bool qn_http_conn_post_buffer(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, const char * restrict req_body, qn_fsize req_body_size, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body);

// ---- Declaration of common functions ----

QN_SDK extern void qn_http_check_and_register_connection_failure(qn_svc_selector_ptr restrict sel, qn_svc_entry_ptr restrict ent);
QN_SDK extern size_t qn_http_read_cfn(void * user_data, char * buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __QN_HTTP_CONNECTION_H__ */

