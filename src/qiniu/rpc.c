#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/version.h"

#include "qiniu/rpc.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ---- Definition of Remote Procedure Caller (Abbreviation: rpc) ---- */

typedef struct _QN_RPC
{
    qn_http_connection_ptr conn;
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
    qn_http_json_writer_ptr ret_wrt;
} qn_rpc_st;

typedef enum
{
    QN_RPC_POST = 0,
    QN_RPC_GET
} qn_rpc_type;

/* -- Constructor & Destructor methods -- */

QN_SDK qn_rpc_ptr qn_rpc_create(void)
{
    qn_rpc_ptr new_rpc = calloc(1, sizeof(qn_rpc_st));
    if (! new_rpc) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_rpc->conn = qn_http_conn_create();
    if (! new_rpc->conn) {
        free(new_rpc);
        return NULL;
    } // if

    new_rpc->req = qn_http_req_create();
    if (! new_rpc->req) {
        qn_http_conn_destroy(new_rpc->conn);
        free(new_rpc);
        return NULL;
    } // if

    new_rpc->resp = qn_http_resp_create();
    if (! new_rpc->resp) {
        qn_http_req_destroy(new_rpc->req);
        qn_http_conn_destroy(new_rpc->conn);
        free(new_rpc);
        return NULL;
    } // if

    new_rpc->ret_wrt = qn_http_json_wrt_create();
    if (! new_rpc->ret_wrt) {
        qn_http_resp_destroy(new_rpc->resp);
        qn_http_req_destroy(new_rpc->req);
        qn_http_conn_destroy(new_rpc->conn);
        free(new_rpc);
        return NULL;
    } // if

    return new_rpc;
}

QN_SDK void qn_rpc_destroy(qn_rpc_ptr restrict rpc)
{
    if (rpc) {
        qn_http_json_wrt_destroy(rpc->ret_wrt);
        qn_http_resp_destroy(rpc->resp);
        qn_http_req_destroy(rpc->req);
        qn_http_conn_destroy(rpc->conn);
    } // if
}

/* -- Setting methods -- */

QN_SDK qn_bool qn_rpc_set_header(qn_rpc_ptr restrict rpc, const char * restrict hdr, const char * restrict val)
{
    return qn_http_req_set_header(rpc->req, hdr, val);
}

QN_SDK void qn_rpc_unset_header(qn_rpc_ptr restrict rpc, const char * restrict hdr)
{
    qn_http_req_unset_header(rpc->req, hdr);
}

/* -- Call methods -- */

static qn_bool qn_rpc_set_common_headers(qn_rpc_ptr restrict rpc)
{
    if (! qn_http_req_set_header(rpc->req, "Expect", "")) return qn_false;
    if (! qn_http_req_set_header(rpc->req, "Transfer-Encoding", "")) return qn_false;
    if (! qn_http_req_set_header(rpc->req, "User-Agent", qn_ver_get_full_string())) return qn_false;
    return qn_true;
}

static qn_json_object_ptr qn_rpc_call(qn_rpc_ptr restrict rpc, const char * restrict url, qn_rpc_type type)
{
    qn_bool ok = qn_false;
    qn_json_object_ptr ret = NULL;
    qn_json_object_ptr obj = NULL;
    qn_json_array_ptr arr = NULL;

    if (! qn_rpc_set_common_headers(rpc)) return NULL;

    qn_http_json_wrt_prepare(rpc->ret_wrt, &obj, &arr);
    qn_http_resp_set_data_writer(rpc->resp, rpc->ret_wrt, &qn_http_json_wrt_write_cfn);

    if (! (ret = qn_json_create_object())) return NULL;
    if (! qn_json_set_integer(ret, "code", 0) || ! qn_json_set_cstr(ret, "error", "OK")) {
        qn_json_destroy_object(ret);
        return NULL;
    } // if

    switch (type) {
        case QN_RPC_POST:
            ok = qn_http_conn_post(rpc->conn, url, rpc->req, rpc->resp);
            break;

        case QN_RPC_GET:
            ok = qn_http_conn_get(rpc->conn, url, rpc->req, rpc->resp);
            break;
    } // switch

    if (! ok) {
        qn_json_destroy_object(ret);
        return NULL;
    } // if

    if (obj) {
        if (qn_json_get_string(obj, "error", NULL) == NULL) {
            /* A business error occured */
            qn_json_destroy_object(ret);
            return obj;
        } // if

        if (! qn_json_set_object(ret, "result", obj)) {
            qn_json_destroy_object(obj);
            qn_json_destroy_object(ret);
            return NULL;
        } // if
    } else if (arr) {
        if (! qn_json_set_array(ret, "result", arr)) {
            qn_json_destroy_array(arr);
            qn_json_destroy_object(ret);
            return NULL;
        } // if
    } else {
        qn_json_destroy_object(ret);
        return NULL;
    } // if

    qn_json_set_integer(ret, "code", qn_http_resp_get_code(rpc->resp));
    return ret;
}

QN_SDK qn_json_object_ptr qn_rpc_post(qn_rpc_ptr restrict rpc, const char * restrict url, qn_io_reader_itf restrict rdr, qn_fsize size)
{
    qn_bool ok = qn_false;
    qn_string tmp = NULL;

    assert(rpc);
    assert(url);
    assert(rdr);

    qn_http_req_reset(rpc->req);
    qn_http_resp_reset(rpc->resp);

    if (! qn_http_req_get_header(rpc->req, "Content-Type")) {
        if (! qn_http_req_set_header(rpc->req, "Content-Type", "application/octet-stream")) return NULL;
    } // if

    if (! qn_http_req_get_header(rpc->req, "Content-Length")) {
#if defined(QN_CFG_LARGE_FILE_SUPPORT)
        if (! (tmp =  qn_cs_sprintf("%llu", size))) return NULL;
#else
        if (! (tmp =  qn_cs_sprintf("%lu", size))) return NULL;
#endif

        ok = qn_http_req_set_header(rpc->req, "Content-Length", qn_str_cstr(tmp));
        qn_str_destroy(tmp);
        if (! ok) return NULL;
    } // if

    qn_http_req_set_body_reader(rpc->req, rdr, qn_http_read_cfn, size);
    return qn_rpc_call(rpc, url, QN_RPC_POST);
}

QN_SDK qn_json_object_ptr qn_rpc_post_form(qn_rpc_ptr restrict rpc, const char * restrict url, qn_http_form_ptr restrict form)
{
    assert(rpc);
    assert(url);
    assert(form);

    qn_http_req_reset(rpc->req);
    qn_http_resp_reset(rpc->resp);

    qn_http_req_set_form(rpc->req, form);
    return qn_rpc_call(rpc, url, QN_RPC_POST);
}

QN_SDK qn_json_object_ptr qn_rpc_get(qn_rpc_ptr restrict rpc, const char * restrict url)
{
    assert(rpc);
    assert(url);

    qn_http_req_reset(rpc->req);
    qn_http_resp_reset(rpc->resp);
    return qn_rpc_call(rpc, url, QN_RPC_GET);
}

#ifdef __cplusplus
}
#endif
