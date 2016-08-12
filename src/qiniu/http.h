#ifndef __QN_HTTP_H__
#define __QN_HTTP_H__

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/http_header.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of body reader and writer

typedef int (*qn_http_body_reader)(void * reader, char * buf, int size);
typedef int (*qn_http_body_writer)(void * writer, char * buf, int size);

struct _QN_HTTP_BODY_JSON;
typedef struct _QN_HTTP_BODY_JSON * qn_http_body_json_ptr;

extern qn_http_body_json_ptr qn_http_body_json_create(void);
extern void qn_http_body_json_destroy(qn_http_body_json_ptr writer_data);

extern void qn_http_body_json_prepare_for_object(qn_http_body_json_ptr writer, qn_json_object_ptr * obj);
extern void qn_http_body_json_prepare_for_array(qn_http_body_json_ptr writer, qn_json_array_ptr * arr);
extern int qn_http_body_json_write(void * writer, char * buf, int buf_size);

struct _QN_HTTP_HEADER_WRITER;
typedef struct _QN_HTTP_HEADER_WRITER * qn_http_header_writer_ptr;

extern qn_http_header_writer_ptr qn_http_header_writer_create(void);
extern void qn_http_header_writer_destroy(qn_http_header_writer_ptr writer);

extern void qn_http_header_writer_prepare(qn_http_header_writer_ptr writer, qn_http_header_ptr hdr);
extern int qn_http_header_writer_callback(void * writer, char * buf, int buf_size);

// ---- Declaration of HTTP request ----

struct _QN_HTTP_REQUEST;
typedef struct _QN_HTTP_REQUEST * qn_http_request_ptr;

extern qn_http_request_ptr qn_http_req_create(void);
extern void qn_http_req_destroy(qn_http_request_ptr req);
extern void qn_http_req_reset(qn_http_request_ptr req);

extern qn_bool qn_http_req_get_header_raw(qn_http_request_ptr req, const char * hdr, qn_size hdr_size, const char ** val, qn_size * val_size);

static inline qn_bool qn_http_req_get_header(qn_http_request_ptr req, const qn_string hdr, const char ** val, qn_size * val_size)
{
    return qn_http_req_get_header_raw(req, qn_str_cstr(hdr), qn_str_size(hdr), val, val_size);
}

extern qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr req, const qn_string header, const qn_string val1, const qn_string val2, ...);
extern qn_bool qn_http_req_set_header(qn_http_request_ptr req, const qn_string header, const qn_string value);
extern qn_bool qn_http_req_set_header_raw(qn_http_request_ptr req, const char * hdr, int hdr_size, const char * val, int val_size);
extern void qn_http_req_unset_header(qn_http_request_ptr req, const qn_string header);

extern void qn_http_req_set_body_reader(qn_http_request_ptr req, void * body_reader, qn_http_body_reader body_reader_callback, qn_size body_size);

// ---- Declaration of HTTP response ----

struct _QN_HTTP_RESPONSE;
typedef struct _QN_HTTP_RESPONSE * qn_http_response_ptr;

extern qn_http_response_ptr qn_http_resp_create(void);
extern void qn_http_resp_destroy(qn_http_response_ptr resp);
extern void qn_http_resp_reset(qn_http_response_ptr resp);

extern int qn_http_resp_get_code(qn_http_response_ptr resp);
extern int qn_http_resp_get_writer_retcode(qn_http_response_ptr resp);

extern qn_bool qn_http_resp_get_header_raw(qn_http_response_ptr resp, const char * hdr, qn_size hdr_size, const char ** val, qn_size * val_size);

static inline qn_bool qn_http_resp_get_header(qn_http_response_ptr resp, const qn_string hdr, const char ** val, qn_size * val_size)
{
    return qn_http_resp_get_header_raw(resp, qn_str_cstr(hdr), qn_str_size(hdr), val, val_size);
}

extern qn_bool qn_http_resp_set_header(qn_http_response_ptr resp, const qn_string header, const qn_string value);
extern qn_bool qn_http_resp_set_header_raw(qn_http_response_ptr resp, const char * hdr, int hdr_size, const char * val, int val_size);
extern void qn_http_resp_unset_header(qn_http_response_ptr resp, const qn_string header);

extern void qn_http_resp_set_body_writer(qn_http_response_ptr resp, void * writer, qn_http_body_writer writer_callback);

// ---- Declaration of HTTP connection ----

struct _QN_HTTP_CONNECTION;
typedef struct _QN_HTTP_CONNECTION * qn_http_connection_ptr;

extern qn_http_connection_ptr qn_http_conn_create(void);
extern void qn_http_conn_destroy(qn_http_connection_ptr conn);

extern qn_bool qn_http_conn_get(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp);
extern qn_bool qn_http_conn_post(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_H__

