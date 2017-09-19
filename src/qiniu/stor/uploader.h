#ifndef __QN_STOR_UPLOADER_H__
#define __QN_STOR_UPLOADER_H__

#include "qiniu/base/io.h"
#include "qiniu/base/json.h"
#include "qiniu/rpc.h"
#include "qiniu/service_selector.h"
#include "qiniu/reader.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ---- Declaration of Uploader Extra Argument (Abbreviation: upe) ---- */

static inline qn_bool qn_upe_set_mime_type(qn_json_object_ptr restrict upe, const char * restrict mime_type)
{
    assert(upe);
    assert(mime_type);
    return qn_json_set_cstr(upe, "mime_type", mime_type);
}

static inline qn_bool qn_upe_set_accept_type(qn_json_object_ptr restrict upe, const char * restrict accept_type)
{
    assert(upe);
    assert(accept_type);
    return qn_json_set_cstr(upe, "accept_type", accept_type);
}

static inline qn_bool qn_upe_set_crc32(qn_json_object_ptr restrict upe, const char * restrict crc32)
{
    assert(upe);
    assert(crc32);
    return qn_json_set_cstr(upe, "crc32", crc32);
}

static inline qn_bool qn_upe_set_save_key(qn_json_object_ptr restrict upe, const char * restrict save_key)
{
    assert(upe);
    assert(save_key);
    return qn_json_set_cstr(upe, "save_key", save_key);
}

static inline qn_bool qn_upe_set_service_entry(qn_json_object_ptr restrict upe, qn_svc_entry_ptr restrict entry)
{
    assert(upe);
    assert(entry);
    if (! qn_json_set_cstr(upe, "base_url", entry->base_url)) return qn_false;
    if (entry->hostname && ! qn_json_set_cstr(upe, "hostname", entry->hostname)) return qn_false;
    return qn_true;
}

static inline qn_bool qn_upe_set_user_defined_value(qn_json_object_ptr restrict upe, const char * restrict key, const char * val)
{
    qn_json_object ud = NULL;
    assert(upe);
    assert(key && val);
    if (! (ud = qn_json_get_object(upe, "user_defined_vars", NULL))) {
        if (! (ud = qn_json_create_and_set_object(upe, "user_defined_vars"))) return qn_false;
    } // if
    return qn_json_set_cstr(ud, key, val);
}

/* ---- Declaration of Uploader (Abbreviation: up) ---- */

struct _QN_UPLOADER;
typedef struct _QN_UPLOADER * qn_uploader_ptr;

/* -- Constructor & Destructor methods -- */ 

QN_SDK extern qn_uploader_ptr qn_up_create(void);
QN_SDK extern qn_uploader_ptr qn_up_create_ex(qn_rpc_ptr restrict rpc, qn_svc_selector_ptr restrict sel);
QN_SDK extern void qn_up_destroy(qn_uploader_ptr restrict up);

/* -- API methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_api_upload(qn_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict upe);

/* -- API Wrapper methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_upload_file(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_json_object_ptr restrict upe);
QN_SDK extern qn_json_object_ptr qn_up_upload_buffer(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_json_object_ptr restrict upe);

#ifdef __cplusplus
}
#endif

#endif // __QN_STOR_UPLOADER_H__
