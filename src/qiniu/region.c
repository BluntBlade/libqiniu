#include <ctype.h>
#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/http.h"
#include "qiniu/region.h"
#include "qiniu/version.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of Region ----

typedef struct _QN_REGION
{
    qn_service_ptr services[QN_SVC_COUNT];
} qn_region_st;

QN_SDK qn_region_ptr qn_rgn_create(void)
{
    qn_region_ptr new_rgn = calloc(1, sizeof(qn_region_st));
    if (! new_rgn) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_rgn;
}

QN_SDK void qn_rgn_destroy(qn_region_ptr restrict rgn)
{
    if (rgn) {
        qn_rgn_reset(rgn);
        free(rgn);
    } // if
}

QN_SDK void qn_rgn_reset(qn_region_ptr restrict rgn)
{
    int i = 0;
    assert(rgn);
    for (i = 0; i < QN_SVC_COUNT; i += 1) {
        if (rgn->services[i]) {
            qn_svc_destroy(rgn->services[i]);
            rgn->services[i] = NULL;
        } // if
    } // for
}

QN_SDK qn_region_ptr qn_rgn_duplicate(qn_region_ptr restrict rgn)
{
    unsigned int i = 0;
    qn_region_ptr new_rgn = NULL;

    assert(rgn);
    
    if (! (new_rgn = qn_rgn_create())) return NULL;
    for (i = 0; i < QN_SVC_COUNT; i += 1) {
        if (rgn->services[i] && ! (new_rgn->services[i] = qn_svc_duplicate(rgn->services[i]))) {
            qn_rgn_destroy(new_rgn);
            return NULL;
        } // if
    } // if
    return new_rgn;
}

QN_SDK void qn_rgn_set_service(qn_region_ptr restrict rgn, qn_svc_type type, qn_service_ptr restrict svc)
{
    assert(rgn);
    if (rgn->services[type]) qn_svc_destroy(rgn->services[type]);
    rgn->services[type] = svc;
}

QN_SDK qn_service_ptr qn_rgn_get_service(qn_region_ptr restrict rgn, qn_svc_type type)
{
    assert(rgn);
    assert(type < QN_SVC_COUNT);
    return rgn->services[type];
}

// ---- Definition of Region Service ----

typedef struct _QN_RGN_SERVICE
{
    qn_http_connection_ptr conn;
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
    qn_http_json_writer_ptr resp_json_wrt;
} qn_rgn_service;

QN_SDK qn_rgn_service_ptr qn_rgn_svc_create(void)
{
    qn_rgn_service_ptr new_svc = calloc(1, sizeof(qn_rgn_service));
    if (!new_svc) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_svc->conn = qn_http_conn_create();
    if (!new_svc->conn) {
        free(new_svc);
        return NULL;
    } // if

    new_svc->req = qn_http_req_create();
    if (!new_svc->req) {
        qn_http_conn_destroy(new_svc->conn);
        free(new_svc);
        return NULL;
    } // if

    new_svc->resp = qn_http_resp_create();
    if (!new_svc->resp) {
        qn_http_req_destroy(new_svc->req);
        qn_http_conn_destroy(new_svc->conn);
        free(new_svc);
        return NULL;
    } // if

    new_svc->resp_json_wrt = qn_http_json_wrt_create();
    if (!new_svc->resp_json_wrt) {
        qn_http_resp_destroy(new_svc->resp);
        qn_http_req_destroy(new_svc->req);
        qn_http_conn_destroy(new_svc->conn);
        free(new_svc);
        return NULL;
    } // if
    return new_svc;
}

QN_SDK void qn_rgn_svc_destroy(qn_rgn_service_ptr restrict svc)
{
    if (svc) {
        qn_http_json_wrt_destroy(svc->resp_json_wrt);
        qn_http_resp_destroy(svc->resp);
        qn_http_req_destroy(svc->req);
        qn_http_conn_destroy(svc->conn);
        free(svc);
    } // if
}

static qn_bool qn_rgn_svc_parse_and_add_entry(qn_string restrict txt, qn_service_ptr restrict svc)
{
    qn_bool ret = qn_false;
    const char * end = NULL;
    const char * base_url = NULL;
    const char * hostname = NULL;
    qn_size base_url_size = 0;
    qn_size hostname_size = 0;
    qn_svc_entry_st new_ent;

    memset(&new_ent, 0, sizeof(new_ent));

    if ((hostname = posix_strstr(qn_str_cstr(txt), "-H"))) {
        hostname += 2;
        while (isspace(*hostname)) hostname += 1;
        end = strchr(hostname, ' ');
        hostname_size = end - hostname;

        base_url = posix_strstr(end, "http");
        base_url_size = qn_str_cstr(txt) + qn_str_size(txt) - hostname;
    } else {
        base_url = qn_str_cstr(txt);
        while (isspace(*base_url)) base_url++;

        end = qn_str_cstr(txt) + qn_str_size(txt);
        while (isspace(end[-1])) end -= 1;

        base_url_size = end - base_url;
    } // if

    if (! (new_ent.base_url = qn_cs_clone(base_url, base_url_size))) return qn_false;
    if (hostname && ! (new_ent.hostname = qn_cs_clone(hostname, hostname_size))) {
        qn_str_destroy(new_ent.base_url);
        return qn_false;
    } // if

    ret = qn_svc_add_entry(svc, &new_ent);
    qn_str_destroy(new_ent.base_url);
    qn_str_destroy(new_ent.hostname);
    if (! ret) {
        return qn_false;
    } // if
    return qn_true;
}

static qn_region_ptr qn_rgn_svc_make_region(qn_json_object_ptr root)
{
    qn_bool ret;
    qn_rgn_host_ptr up;
    qn_rgn_host_ptr io;
    qn_json_object_ptr scheme_table = NULL;
    qn_json_array_ptr entry_list;
    qn_json_integer ttl = 86400;
    qn_string str = NULL;
    int i;

    qn_json_obj_get_integer(root, "ttl", &ttl);
    rgn->time_to_live = ttl;

    if (! (up_svc = qn_svc_create(QN_SVC_UP))) {
        qn_json_destroy_object(root);
        return NULL;
    } // if
    if (! (io_svc = qn_svc_create(QN_SVC_IO))) {
        qn_svc_destroy(io_svc);
        qn_json_destroy_object(root);
        return NULL;
    } // if

    if (qn_json_obj_get_object(root, "http", &scheme_table)) {
        entry_list = NULL;
        if (! qn_json_obj_get_array(scheme_table, "io", &entry_list)) return qn_false;
        for (i = 0; entry_list && i < qn_json_arr_size(entry_list); i += 1) {
            str = NULL;
            if (! qn_json_arr_get_string(entry_list, i, &str)) return qn_false;
            ret = qn_rgn_svc_parse_and_add_entry(str, io);
            if (!ret) return qn_false;
        } // for

        entry_list = NULL;
        if (! qn_json_obj_get_array(scheme_table, "up", &entry_list)) return qn_false;
        for (i = 0; entry_list && i < qn_json_arr_size(entry_list); i += 1) {
            str = NULL;
            if (! qn_json_arr_get_string(entry_list, i, &str)) return qn_false;
            ret = qn_rgn_svc_parse_and_add_entry(str, up);
            if (!ret) return qn_false;
        } // for
    } // if

    scheme_table = NULL;
    if (qn_json_obj_get_object(root, "https", &scheme_table)) {
        entry_list = NULL;
        if (! qn_json_obj_get_array(scheme_table, "io", &entry_list)) return qn_false;
        for (i = 0; entry_list && i < qn_json_arr_size(entry_list); i += 1) {
            str = NULL;
            if (! qn_json_arr_get_string(entry_list, i, &str)) return qn_false;
            ret = qn_rgn_svc_parse_and_add_entry(str, io);
            if (!ret) return qn_false;
        } // for

        entry_list = NULL;
        if (! qn_json_obj_get_array(scheme_table, "up", &entry_list)) return qn_false;
        for (i = 0; entry_list && i < qn_json_arr_size(entry_list); i += 1) {
            str = NULL;
            if (! qn_json_arr_get_string(entry_list, i, &str)) return qn_false;
            ret = qn_rgn_svc_parse_and_add_entry(str, up);
            if (!ret) return qn_false;
        } // for
    } // if

    if (! (new_rgn = qn_rgn_create())) goto QN_RGN_SVC_MAKE_REGION_ERROR_HANDLING;
    qn_rgn_set_service(new_rgn, QN_SVC_UP, up_svc);
    qn_rgn_set_service(new_rgn, QN_SVC_IO, io_svc);

    qn_json_destroy_object(root);
    return new_rgn;

QN_RGN_SVC_MAKE_REGION_ERROR_HANDLING:
    qn_svc_destroy(io_svc);
    qn_svc_destroy(up_svc);
    qn_json_destroy_object(root);
    return NULL;
}

QN_SDK qn_region_ptr qn_rgn_svc_grab_entry_info(qn_rgn_service_ptr restrict svc, qn_rgn_auth_ptr restrict auth, const char * restrict bucket, qn_integer * restrict ttl)
{
    qn_bool ret = qn_false;
    qn_string url = NULL;
    qn_string encoded_bucket = NULL;
    qn_json_object_ptr root = NULL;

    // ---- Prepare the query URL
    encoded_bucket = qn_cs_percent_encode(bucket, posix_strlen(bucket));
    if (! encoded_bucket) return NULL;

    url = qn_cs_sprintf("%s/v1/query?ak=%s&bucket=%s", "http://uc.qbox.me", auth->v1.access_key, qn_str_cstr(encoded_bucket));
    qn_str_destroy(encoded_bucket);
    if (! url) return NULL;

    // ---- Prepare the request and response object
    qn_http_req_reset(svc->req);
    qn_http_resp_reset(svc->resp);

    if (!qn_http_req_set_header(svc->req, "Expect", "")) {
        qn_str_destroy(url);
        return NULL;
    } // if
    if (!qn_http_req_set_header(svc->req, "Transfer-Encoding", "")) {
        qn_str_destroy(url);
        return NULL;
    } // if
    if (! qn_http_req_set_header(svc->req, "User-Agent", qn_ver_get_full_string())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_req_set_body_data(svc->req, "", 0);

    qn_http_json_wrt_prepare(svc->resp_json_wrt, &root, NULL);
    qn_http_resp_set_data_writer(svc->resp, svc->resp_json_wrt, &qn_http_json_wrt_write_cfn);

    ret = qn_http_conn_get(svc->conn, url, svc->req, svc->resp);
    qn_str_destroy(url);

    // ---- Grab the region info of the givan bucket

    if (ret) {
        if (! (new_rgn = qn_rgn_create(bucket))) {
            qn_json_obj_destroy(root);
            return qn_false;
        } // if

        if (!qn_rgn_svc_extract_and_add_entries(root, new_rgn)) {
            qn_rgn_destroy(new_rgn);
            qn_json_obj_destroy(root);
            return qn_false;
        } // if

        ret = qn_rgn_tbl_set_region(rtbl, bucket, new_rgn);
        qn_rgn_destroy(new_rgn);
    } // if
    // TODO: Deal with the case that API return no value.
    qn_json_obj_destroy(root);
    return ret;
}

#ifdef __cplusplus
}
#endif

