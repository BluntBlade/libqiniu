#include <assert.h>
#include <curl/curl.h>

#include "qiniu/base/errors.h"
#include "qiniu/http/form.h"

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
    CURLFORMcode ret = CURLE_OK;

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
    CURLFORMcode ret = CURLE_OK;

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
    CURLFORMcode ret = CURLE_OK;

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
    CURLFORMcode ret = CURLE_OK;

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
