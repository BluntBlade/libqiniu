#include <assert.h>

#include "qiniu/base/json_parser.h"
#include "qiniu/base/errors.h"
#include "base/http/body_json.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Definition of HTTP JSON-Body Parser (Abbreviation: http_json) ==== */

typedef enum _QN_HTTP_JSON_STATUS
{
    QN_HTTP_JSON_PARSING_READY = 0,
    QN_HTTP_JSON_PARSING_OBJECT = 1,
    QN_HTTP_JSON_PARSING_ARRAY = 2,
    QN_HTTP_JSON_PARSING_DONE = 3,
    QN_HTTP_JSON_PARSING_ERROR = 4
} qn_http_json_status;

typedef struct _QN_HTTP_JSON
{
    qn_http_body_ptr body_vtbl;
    qn_json_parser_ptr prs;
    qn_json_object_ptr obj;
    qn_json_array_ptr arr;
    qn_http_json_status sts;
} qn_http_json_st;

/* == Constructor & Destructor methods == */

static inline qn_http_json_ptr qn_http_json_from_body(qn_http_body_itf restrict itf)
{
    return (qn_http_json_ptr)( ( (char *) itf ) - (char *)( &((qn_http_json_ptr)0)->body_vtbl ) );
}

static size_t qn_http_json_parse_vfn(qn_http_body_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_http_json_parse(qn_http_json_from_body(itf), buf, buf_size);
}

static const char * qn_http_json_mime_type_vfn(qn_http_body_itf restrict itf)
{
    return qn_http_json_mime_type(qn_http_json_from_body(itf));
}

static qn_fsize qn_http_json_size_vfn(qn_http_body_itf restrict itf)
{
    return qn_http_json_size(qn_http_json_from_body(itf));
}

static qn_http_body_st qn_http_json_body_vtable = {
    &qn_http_json_parse_vfn,

    &qn_http_json_mime_type_vfn,
    &qn_http_json_size_vfn
};

QN_SDK qn_http_json_ptr qn_http_json_create(void)
{
    qn_http_json_ptr new_body = malloc(sizeof(qn_http_json_st));
    if (! new_body) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_body->prs = qn_json_prs_create();
    if (! new_body->prs) {
        free(new_body);
        return NULL;
    } // if

    new_body->body_vtbl = &qn_http_json_body_vtable;
    return new_body;
}

QN_SDK void qn_http_json_destroy(qn_http_json_ptr restrict body)
{
    if (body) {
        qn_http_json_reset(body);
        qn_json_prs_destroy(body->prs);
        free(body);
    } // if
}

QN_SDK void qn_http_json_reset(qn_http_json_ptr restrict body)
{
    assert(body);

    if (body->obj) {
        qn_json_obj_destroy(body->obj);
        body->obj = NULL;
    } // if
    if (body->arr) {
        qn_json_arr_destroy(body->arr);
        body->arr = NULL;
    } // if
    body->sts = QN_HTTP_JSON_PARSING_READY;
}

/* == Cast methods == */

QN_SDK qn_http_body_itf qn_http_json_to_body_interface(qn_http_json_ptr restrict body)
{
    return &body->body_vtbl;
}

/* == Get methods == */

QN_SDK qn_json_object_ptr qn_http_json_get_object(qn_http_json_ptr restrict body)
{
    assert(body);
    return body->obj;
}

QN_SDK qn_json_array_ptr qn_http_json_get_array(qn_http_json_ptr restrict body)
{
    assert(body);
    return body->arr;
}

/* == Interface methods == */

static size_t qn_http_json_parse_object(qn_http_json_ptr body, char * restrict buf, size_t buf_size)
{
    qn_size size = buf_size;
    if (qn_json_prs_parse_object(body->prs, buf, &size, &body->obj)) {
        /* Parsing object is done. */
        body->sts = QN_HTTP_JSON_PARSING_DONE;
        qn_err_set_succeed();
        return buf_size;
    } // if

    /* Handle errors. */
    if (qn_err_json_is_need_more_text_input()) {
        body->sts = QN_HTTP_JSON_PARSING_OBJECT;
        return buf_size;
    } // if

    /* Parsing object failed in other chunks of the body. */
    if (! qn_err_json_is_bad_text_input()) body->sts = QN_HTTP_JSON_PARSING_ERROR;
    return 0;
}

static size_t qn_http_json_parse_array(qn_http_json_ptr body, char * restrict buf, size_t buf_size)
{
    qn_size size = buf_size;
    if (qn_json_prs_parse_array(body->prs, buf, &size, &body->arr)) {
        /* Parsing array is done. */
        body->sts = QN_HTTP_JSON_PARSING_DONE;
        qn_err_set_succeed();
        return buf_size;
    } // if

    /* Handle errors. */
    if (qn_err_json_is_need_more_text_input()) {
        body->sts = QN_HTTP_JSON_PARSING_ARRAY;
        return buf_size;
    } // if

    /* Parsing array failed in other chunks of the body. */
    if (! qn_err_json_is_bad_text_input()) body->sts = QN_HTTP_JSON_PARSING_ERROR;
    return 0;
}

QN_SDK size_t qn_http_json_parse(qn_http_json_ptr restrict body, char * restrict buf, size_t buf_size)
{
    size_t consumed_bytes = 0;

    assert(body);
    assert(buf);

    // **NOTE**: If the writing is done, or encounter an error, always return the buf_size for consuming all data received.
    switch (body->sts) {
        case QN_HTTP_JSON_PARSING_READY:
            /* Try to parse as a JSON object first. */
            consumed_bytes = qn_http_json_parse_object(body, buf, buf_size);

            if (body->sts == QN_HTTP_JSON_PARSING_READY) {
                /* If the first chunk of the body does not start a JSON object,
                 * try to parse as a JSON array.
                 */

                consumed_bytes = qn_http_json_parse_array(body, buf, buf_size);
                if (body->sts == QN_HTTP_JSON_PARSING_READY) {
                    qn_err_json_set_bad_text_input();
                    consumed_bytes = 0;
                } // if
            } // if
            return consumed_bytes;

        case QN_HTTP_JSON_PARSING_OBJECT: return qn_http_json_parse_object(body, buf, buf_size);
        case QN_HTTP_JSON_PARSING_ARRAY: return qn_http_json_parse_array(body, buf, buf_size);
        case QN_HTTP_JSON_PARSING_ERROR: return 0;
        case QN_HTTP_JSON_PARSING_DONE: break;
    } // switch
    return buf_size;
}

QN_SDK const char * qn_http_json_mime_type(qn_http_body_itf restrict itf)
{
    return "application/json";
}

QN_SDK qn_fsize qn_http_json_size(qn_http_body_itf restrict itf)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
