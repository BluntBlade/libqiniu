#ifndef __QN_HTTP_H__
#define __QN_HTTP_H__

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/http_header.h"
#include "qiniu/os/file.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of body reader and writer

typedef qn_size (*qn_http_body_reader_callback)(void * restrict reader, char * restrict buf, qn_size size);
typedef qn_size (*qn_http_data_writer_callback)(void * restrict writer, char * restrict buf, qn_size size);

// ----

struct _QN_HTTP_JSON_WRITER;
typedef struct _QN_HTTP_JSON_WRITER * qn_http_json_writer_ptr;

QN_API extern qn_http_json_writer_ptr qn_http_json_wrt_create(void);
QN_API extern void qn_http_json_wrt_destroy(qn_http_json_writer_ptr restrict writer);

QN_API extern void qn_http_json_wrt_prepare_for_object(qn_http_json_writer_ptr restrict writer, qn_json_object_ptr * restrict obj);
QN_API extern void qn_http_json_wrt_prepare_for_array(qn_http_json_writer_ptr restrict writer, qn_json_array_ptr * restrict arr);
QN_API extern qn_size qn_http_json_wrt_callback(void * restrict writer, char * restrict buf, qn_size buf_size);

// ---- Declaration of HTTP form

struct _QN_HTTP_FORM;
typedef struct _QN_HTTP_FORM * qn_http_form_ptr;

QN_API extern qn_http_form_ptr qn_http_form_create(void);
QN_API extern void qn_http_form_destroy(qn_http_form_ptr restrict form);

QN_API extern qn_bool qn_http_form_add_string(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict value, qn_size size);
QN_API extern qn_bool qn_http_form_add_file(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_size fsize);
QN_API extern qn_bool qn_http_form_add_buffer(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict buf, int buf_size);

// ---- Declaration of HTTP request ----

struct _QN_HTTP_REQUEST;
typedef struct _QN_HTTP_REQUEST * qn_http_request_ptr;

QN_API extern qn_http_request_ptr qn_http_req_create(void);
QN_API extern void qn_http_req_destroy(qn_http_request_ptr restrict req);
QN_API extern void qn_http_req_reset(qn_http_request_ptr restrict req);

// ----

QN_API extern qn_bool qn_http_req_get_header_raw(qn_http_request_ptr restrict req, const char * restrict hdr, qn_size hdr_size, const char ** restrict val, qn_size * restrict val_size);

static inline qn_bool qn_http_req_get_header(qn_http_request_ptr restrict req, const qn_string restrict hdr, const char ** restrict val, qn_size * restrict val_size)
{
    return qn_http_req_get_header_raw(req, qn_str_cstr(hdr), qn_str_size(hdr), val, val_size);
}

QN_API extern qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr restrict req, const qn_string restrict header, const qn_string restrict val1, const qn_string val2, ...);
QN_API extern qn_bool qn_http_req_set_header(qn_http_request_ptr restrict req, const qn_string restrict header, const qn_string restrict value);
QN_API extern qn_bool qn_http_req_set_header_raw(qn_http_request_ptr restrict req, const char * restrict hdr, int hdr_size, const char * restrict val, int val_size);
QN_API extern void qn_http_req_unset_header(qn_http_request_ptr restrict req, const qn_string restrict header);

// ----

QN_API extern qn_http_form_ptr qn_http_req_prepare_form(qn_http_request_ptr restrict req);
QN_API extern qn_http_form_ptr qn_http_req_get_form(qn_http_request_ptr restrict req);
QN_API extern void qn_http_req_set_form(qn_http_request_ptr restrict req, qn_http_form_ptr restrict form);

// ----

QN_API extern void qn_http_req_set_body_data(qn_http_request_ptr restrict req, char * restrict body_data, qn_size body_size);
QN_API extern void qn_http_req_set_body_reader(qn_http_request_ptr restrict req, void * restrict body_reader, qn_http_body_reader_callback body_reader_callback, qn_size body_size);
QN_API extern char * qn_http_req_body_data(qn_http_request_ptr restrict req);
QN_API extern qn_size qn_http_req_body_size(qn_http_request_ptr restrict req);

// ---- Declaration of HTTP response ----

struct _QN_HTTP_RESPONSE;
typedef struct _QN_HTTP_RESPONSE * qn_http_response_ptr;

QN_API extern qn_http_response_ptr qn_http_resp_create(void);
QN_API extern void qn_http_resp_destroy(qn_http_response_ptr restrict resp);
QN_API extern void qn_http_resp_reset(qn_http_response_ptr restrict resp);

// ----

QN_API extern int qn_http_resp_get_code(qn_http_response_ptr restrict resp);
QN_API extern int qn_http_resp_get_writer_retcode(qn_http_response_ptr restrict resp);

// ----

QN_API extern qn_http_hdr_iterator_ptr qn_http_resp_get_header_iterator(qn_http_response_ptr restrict resp);
QN_API extern qn_bool qn_http_resp_get_header_raw(qn_http_response_ptr restrict resp, const char * restrict hdr, qn_size hdr_size, const char ** restrict val, qn_size * restrict val_size);

static inline qn_bool qn_http_resp_get_header(qn_http_response_ptr restrict resp, const qn_string restrict hdr, const char ** restrict val, qn_size * restrict val_size)
{
    return qn_http_resp_get_header_raw(resp, qn_str_cstr(hdr), qn_str_size(hdr), val, val_size);
}

QN_API extern qn_bool qn_http_resp_set_header(qn_http_response_ptr restrict resp, const qn_string restrict header, const qn_string restrict value);
QN_API extern qn_bool qn_http_resp_set_header_raw(qn_http_response_ptr restrict resp, const char * restrict hdr, int hdr_size, const char * restrict val, int val_size);
QN_API extern void qn_http_resp_unset_header(qn_http_response_ptr restrict resp, const qn_string restrict header);

// ----

QN_API extern void qn_http_resp_set_data_writer(qn_http_response_ptr restrict resp, void * restrict body_writer, qn_http_data_writer_callback body_writer_callback);

// ---- Declaration of HTTP connection ----

struct _QN_HTTP_CONNECTION;
typedef struct _QN_HTTP_CONNECTION * qn_http_connection_ptr;

QN_API extern qn_http_connection_ptr qn_http_conn_create(void);
QN_API extern void qn_http_conn_destroy(qn_http_connection_ptr restrict conn);

QN_API extern qn_bool qn_http_conn_get(qn_http_connection_ptr restrict conn, const qn_string restrict url, qn_http_request_ptr restrict req, qn_http_response_ptr restrict resp);
QN_API extern qn_bool qn_http_conn_post(qn_http_connection_ptr restrict conn, const qn_string restrict url, qn_http_request_ptr restrict req, qn_http_response_ptr restrict resp);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_H__

