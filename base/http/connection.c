#include <assert.h>
#include <ctype.h>
#include <curl/curl.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/errors.h"

#include "qiniu/http/header.h"
#include "qiniu/http/connection.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Declaration of HTTP Form (Abbreviation: http_form) ==== */

typedef struct _QN_HTTP_FORM
{
    struct curl_httppost * first;
    struct curl_httppost * last;
    qn_uint use_data_reader:1;
} qn_http_form_st;

/* == Constructor & Destructor methods == */

QN_SDK qn_http_form_ptr qn_http_form_create(void)
{
    qn_http_form_ptr new_form = calloc(1, sizeof(qn_http_form_st));
    if (! new_form) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_form;
}

QN_SDK void qn_http_form_destroy(qn_http_form_ptr restrict form)
{
    if (form) {
        qn_http_form_reset(form);
        free(form);
    } // form
}

QN_SDK void qn_http_form_reset(qn_http_form_ptr restrict form)
{
    assert(form);

    curl_formfree(form->first);
    form->first = NULL;
    form->last = NULL;
    form->use_data_reader = 0;
}

/* == Add methods == */

QN_SDK qn_bool qn_http_form_add_raw(qn_http_form_ptr restrict form, const char * restrict fld, qn_size fld_size, const char * restrict val, qn_size val_size)
{
    CURLFORMcode ret = 0;

    assert(form);
    assert(fld);
    assert(val);

    if (sizeof(curl_off_t) == 4 && sizeof(curl_off_t) < sizeof(qn_size)) {
        if (UINT32_MAX < fld_size || UINT32_MAX < val_size) {
            qn_err_set_overflow_upper_bound();
            return qn_false;
        } // if
    } // if
    
    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, fld, CURLFORM_NAMELENGTH, fld_size, CURLFORM_COPYCONTENTS, val, CURLFORM_CONTENTLEN, (curl_off_t)val_size, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_string_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

static inline const char * qn_http_get_fname_utf8(const char * restrict fname)
{
    const char * fname_utf8;
    if (! fname) return "LIBQINIU-MANDATORY-FILENAME";
#ifdef QN_OS_WINDOWS
    return ((fname_utf8 = strrchr(fname, '\\'))) ? fname_utf8 + 1 : fname;
#else
    return ((fname_utf8 = strrchr(fname, '/'))) ? fname_utf8 + 1 : fname;
#endif
}

QN_SDK qn_bool qn_http_form_add_file(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_fsize fsize, const char * restrict mime_type)
{
    CURLFORMcode ret = 0;

    assert(form);
    assert(field);
    assert(fname);

    /* BUG NOTE 1 : Golang HTTP servers will fail in case that the fsize is larger than 10MB and the `filename` attribute of the multipart-data section doesn't exist.
     * BUG FIX    : Use a mandatory filename value to prevent Golang HTTP server from failing.
     */
    if (! fname_utf8) fname_utf8 = qn_http_get_fname_utf8(fname);
    if (! mime_type) mime_type = "application/octet-stream";

    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, field, CURLFORM_FILE, fname, CURLFORM_FILENAME, fname_utf8, CURLFORM_CONTENTTYPE, mime_type, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_file_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_http_form_add_file_reader(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_fsize fsize, const char * restrict mime_type, void * restrict user_data)
{
    CURLFORMcode ret = 0;

    assert(form);
    assert(field);
    assert(fname);
    assert(user_data);

    /* -- See BUG NOTE 1. -- */
    if (! fname_utf8) fname_utf8 = qn_http_get_fname_utf8(fname);
    if (! mime_type) mime_type = "application/octet-stream";

    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, field, CURLFORM_STREAM, user_data, CURLFORM_CONTENTSLENGTH, (long)fsize, CURLFORM_FILENAME, fname_utf8, CURLFORM_CONTENTTYPE, mime_type, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_file_field_failed();
        return qn_false;
    } // if
    form->use_data_reader = 1;
    return qn_true;
}

QN_SDK qn_bool qn_http_form_add_buffer(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict buf, qn_size buf_size, const char * restrict mime_type)
{
    CURLFORMcode ret = 0;

    assert(form);
    assert(field);
    assert(fname);
    assert(buf);

    if (sizeof(long) < sizeof(qn_size)) {
        if (UINT32_MAX < buf_size) {
            qn_err_set_overflow_upper_bound();
            return qn_false;
        } // if
    } // if

    if (! mime_type) mime_type = "application/octet-stream";
    
    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, field, CURLFORM_BUFFER, fname, CURLFORM_BUFFERPTR, buf, CURLFORM_BUFFERLENGTH, (long)buf_size, CURLFORM_CONTENTTYPE, mime_type, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_buffer_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

/* ==== Definition of HTTP Connection (Abbreviation: http_conn) ==== */

typedef struct _QN_HTTP_CONNECTION
{
    struct
    {
        qn_uint code;
        qn_string ver;
        qn_string msg;
    } http;

    struct
    {
        qn_http_header_ptr hdr;
    } resp;

    CURL * curl;
} qn_http_connection;

/* == Constructor & Destructor methods == */

QN_SDK qn_http_connection_ptr qn_http_conn_create(void)
{
    qn_http_connection_ptr new_conn = NULL;

    new_conn = calloc(1, sizeof(qn_http_connection));
    if (! new_conn) {
        qn_err_set_out_of_memory();
        return NULL;
    } /* if */

    new_conn->curl = curl_easy_init();
    if (! new_conn->curl) {
        free(new_conn);
        qn_err_3rdp_set_curl_easy_error_occurred(CURLE_FAILED_INIT);
        return NULL;
    }
    return new_conn;
}

QN_SDK void qn_http_conn_destroy(qn_http_connection_ptr restrict conn)
{
    if (conn) {
        if (conn->http.msg) qn_str_destroy(conn->http.msg);
        if (conn->http.ver) qn_str_destroy(conn->http.ver);
        curl_easy_cleanup(conn->curl);
        free(conn);
    } /* if */
}

/* == Get methods == */

QN_SDK qn_uint qn_http_conn_get_code(qn_http_connection_ptr restrict conn)
{
    assert(conn);
    return conn->http.code;
}

QN_SDK qn_string qn_http_conn_get_version(qn_http_connection_ptr restrict conn)
{
    assert(conn);
    return conn->http.ver;
}

QN_SDK qn_string qn_http_conn_get_message(qn_http_connection_ptr restrict conn)
{
    assert(conn);
    return conn->http.msg;
}

/* == Action methods == */

static size_t qn_http_conn_read_data_cfn(char * ptr, size_t size, size_t nmemb, void * user_data)
{
    ssize_t ret = qn_io_rdr_read((qn_io_reader_itf)user_data, ptr, size * nmemb);
    return (ret < 0) ? CURL_READFUNC_ABORT : ret;
}

static size_t qn_http_conn_write_response_body_cfn(char * ptr, size_t size, size_t nmemb, void * user_data)
{
    return qn_http_body_parse((qn_http_body_itf)user_data, ptr, size * nmemb);
}

static size_t qn_http_conn_write_response_head_cfn(char * buf, size_t size, size_t nitems, void * user_data)
{
    qn_http_connection_ptr conn = (qn_http_connection_ptr) user_data;
    size_t buf_size = size * nitems;
    int i = 0;
    char * begin = NULL;
    char * end = NULL;
    char * val_begin = NULL;
    char * val_end = NULL;

    if (conn->http.code == 0) {
        /* Parse response status line. */
        begin = strchr(buf, '/');

        /* Pase HTTP version. */
        if (! begin) return 0;
        begin += 1;
        end = strchr(begin, ' ');
        if (! end) return 0;
        if (! (conn->http.ver = qn_cs_clone(begin, end - begin))) return 0;

        /* Pase HTTP code. */
        begin = end + 1;
        end = strchr(begin, ' ');
        if (! end) return 0;

        for (i = 0; i < end - begin; i += 1) {
            if (! isdigit(begin[i])) return 0;
            conn->http.code = conn->http.code * 10 + (begin[i] - '0');
        } /* for */

        /* Pase HTTP message. */
        begin = end + 1;
        end = buf + buf_size;
        if (end[-1] != '\n') return 0;
        end -= (end[-2] == '\r') ? 2 : 1;
        if (! (conn->http.msg = qn_cs_clone(begin, end - begin))) return 0;
    } else {
        /* Parse response headers. */
        begin = buf;
        end = strchr(begin, ':');

        if (! end) {
            if ((begin[0] == '\r' && begin[1] == '\n') || begin[0] == '\n') return buf_size;
            return 0;
        } /* if */

        while (isspace(begin[0])) begin += 1;
        while (isspace(end[-1])) end -= 1;

        val_begin = end + 1;
        val_end = buf + buf_size;
        val_end -= (val_end[-2] == '\r') ? 2 : 1;
        while (isspace(val_begin[0])) val_begin += 1;
        while (isspace(val_end[-1])) val_end -= 1;

        if (! qn_http_hdr_set_raw(conn->resp.hdr, begin, end - begin, val_begin, val_end - val_begin)) return 0;
    } /* if */
    return buf_size;
}

static inline qn_bool qn_http_conn_prepare(qn_http_connection_ptr restrict conn, const char * restrict url)
{
    CURLcode curl_code = CURLE_OK;

    if (conn->http.ver) {
        qn_str_destroy(conn->http.ver);
        conn->http.ver = NULL;
    } /* if */
    if (conn->http.msg) {
        qn_str_destroy(conn->http.msg);
        conn->http.msg = NULL;
    } /* if */
    conn->resp.hdr = NULL;

    curl_easy_reset(conn->curl);

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_NOSIGNAL, 1L)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_URL, url)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    return qn_true;
}

static qn_bool qn_http_conn_do_request(qn_http_connection_ptr restrict conn, qn_http_header_ptr restrict req_hdr, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body)
{
    CURLcode curl_code = CURLE_OK;
    struct curl_slist * headers = NULL;
    struct curl_slist * headers2 = NULL;
    qn_string entry = NULL;
    qn_http_hdr_iterator_ptr itr = NULL;

    /* Prepare for writting response headers. */
    conn->resp.hdr = resp_hdr;
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HEADERFUNCTION, qn_http_conn_write_response_head_cfn)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HEADERDATA, conn)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */

    /* Prepare for writting response body. */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, qn_http_conn_write_response_body_cfn)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, resp_body)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */

    /* Prepare for sending request headers. */
    if (qn_http_hdr_count(req_hdr) > 0) {
        if (! (itr = qn_http_hdr_itr_create(req_hdr))) {
            curl_slist_free_all(headers);
            return qn_false;
        } /* if */

        while ((entry = qn_http_hdr_itr_next_entry(itr))) {
            headers2 = curl_slist_append(headers, entry);

            if (! headers2) {
                curl_slist_free_all(headers);
                qn_http_hdr_itr_destroy(itr);
                return qn_false;
            } /* if */
            headers = headers2;
        } /* while */

        qn_http_hdr_itr_destroy(itr);
    } /* if */

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */

    curl_code = curl_easy_perform(conn->curl);
    curl_slist_free_all(headers);
    if (curl_code != CURLE_OK) {
        switch (curl_code) {
            case CURLE_COULDNT_CONNECT:
            case CURLE_OPERATION_TIMEDOUT:
                qn_err_set_try_again();
                return qn_false;

            case CURLE_COULDNT_RESOLVE_HOST:
            case CURLE_COULDNT_RESOLVE_PROXY:
                qn_err_comm_set_dns_failed();
                return qn_false;

            case CURLE_SEND_ERROR:
            case CURLE_RECV_ERROR:
                qn_err_comm_set_transmission_failed();
                return qn_false;

            /* case CURLE_HTTP2: */
            /* case CURLE_HTTP2_STREAM: */
            /* case CURLE_SSL_CONNECT_ERROR: */
            case CURLE_PARTIAL_FILE:
                qn_err_http_set_mismatching_file_size();
                return qn_false;

            default:
                qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
                break;
        } /* switch */
    } /* if */
    return qn_true;
}

QN_SDK qn_bool qn_http_conn_get(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body)
{
    CURLcode curl_code = CURLE_OK;

    assert(conn);
    assert(url);
    assert(req_hdr);
    assert(resp_hdr);
    assert(resp_body);

    if (! qn_http_conn_prepare(conn, url)) return qn_false;

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POST, 0)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    return qn_http_conn_do_request(conn, req_hdr, resp_hdr, resp_body);
}

QN_SDK qn_bool qn_http_conn_post(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_io_reader_itf restrict req_body, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body)
{
    CURLcode curl_code = CURLE_OK;

    assert(conn);
    assert(url);
    assert(req_hdr);
    assert(req_body);
    assert(resp_hdr);
    assert(resp_body);

    if (! qn_http_conn_prepare(conn, url)) return qn_false;

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POST, 1)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_READFUNCTION, qn_http_conn_read_data_cfn)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_READDATA, req_body)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    return qn_http_conn_do_request(conn, req_hdr, resp_hdr, resp_body);
}

QN_SDK qn_bool qn_http_conn_post_form(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, qn_http_form_ptr restrict req_body, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body)
{
    CURLcode curl_code = CURLE_OK;

    assert(conn);
    assert(url);
    assert(req_hdr);
    assert(req_body);
    assert(resp_hdr);
    assert(resp_body);

    if (! qn_http_conn_prepare(conn, url)) return qn_false;

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POST, 1)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HTTPPOST, req_body->first)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if (req_body->use_data_reader) {
        if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_READFUNCTION, qn_http_conn_read_data_cfn)) != CURLE_OK) {
            qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
            return qn_false;
        } /* if */
    } /* if */
    return qn_http_conn_do_request(conn, req_hdr, resp_hdr, resp_body);
}

QN_SDK qn_bool qn_http_conn_post_buffer(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_header_ptr restrict req_hdr, const char * restrict req_body, qn_fsize req_body_size, qn_http_header_ptr restrict resp_hdr, qn_http_body_itf restrict resp_body)
{
    CURLcode curl_code = CURLE_OK;

    assert(conn);
    assert(url);
    assert(req_hdr);
    assert(req_body);
    assert(resp_hdr);
    assert(resp_body);

    if (! qn_http_conn_prepare(conn, url)) return qn_false;

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POST, 1)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POSTFIELDS, req_body)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POSTFIELDSIZE, (long)req_body_size)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } /* if */
    return qn_http_conn_do_request(conn, req_hdr, resp_hdr, resp_body);
}

/* == Check Functions == */

QN_SDK qn_bool qn_http_failed_in_communication(void)
{
    return (qn_err_is_try_again() || qn_err_comm_is_transmission_failed());
}

#ifdef __cplusplus
}
#endif
