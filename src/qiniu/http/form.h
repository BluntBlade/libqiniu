#ifndef __QN_HTTP_FORM_H__
#define __QN_HTTP_FORM_H__

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

#ifdef __cplusplus
}
#endif

#endif /* __QN_HTTP_FORM_H__ */

