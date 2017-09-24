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

/* ---- Declaration of Uploader (Abbreviation: up) ---- */

struct _QN_UPLOADER;
typedef struct _QN_UPLOADER * qn_uploader_ptr;

/* -- Constructor & Destructor methods -- */ 

QN_SDK extern qn_uploader_ptr qn_up_create(void);
QN_SDK extern qn_uploader_ptr qn_up_create_ex(qn_rpc_ptr restrict rpc, qn_svc_selector_ptr restrict sel);
QN_SDK extern void qn_up_destroy(qn_uploader_ptr restrict up);

/* -- Extra Arugment Setting methods -- */ 

QN_SDK extern qn_bool qn_up_set_mime_type(qn_json_object_ptr restrict ext, const char * restrict mime_type);
QN_SDK extern qn_bool qn_up_set_accept_type(qn_json_object_ptr restrict ext, const char * restrict accept_type);
QN_SDK extern qn_bool qn_up_set_crc32(qn_json_object_ptr restrict ext, const char * restrict crc32);
QN_SDK extern qn_bool qn_up_set_save_key(qn_json_object_ptr restrict ext, const char * restrict save_key);
QN_SDK extern qn_bool qn_up_set_user_defined_value(qn_json_object_ptr restrict ext, const char * restrict key, const char * val);

QN_SDK qn_bool qn_up_set_service_entry(qn_json_object_ptr restrict ext, qn_svc_entry_ptr restrict entry);

/* -- API methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_api_upload(qn_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict ext);

/* -- API Wrapper methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_upload_file(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_json_object_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_up_upload_buffer(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_json_object_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STOR_UPLOADER_H__
