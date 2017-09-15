#ifndef __QN_STOR_UPLOADER_H__
#define __QN_STOR_UPLOADER_H__

#include "qiniu/base/io.h"
#include "qiniu/base/json.h"
#include "qiniu/service.h"
#include "qiniu/reader.h"
#include "qiniu/ud/variable.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_UP_EXTRA
{
    const char * mime_type;
    const char * accept_type;
    const char * crc32;
    const char * save_key;

    qn_ud_variable_ptr user_defined_vars;

    qn_svc_entry_ptr entry;
} qn_up_extra_st, *qn_up_extra_ptr;

struct _QN_UPLOADER;
typedef struct _QN_UPLOADER * qn_uploader_ptr;

/* -- Constructor & Destructor methods -- */ 

QN_SDK qn_uploader_ptr qn_up_create(qn_service_ptr restrict svc);
QN_SDK void qn_up_destroy(qn_uploader_ptr restrict up);

/* -- API methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_api_upload(qn_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_up_extra_ptr restrict ext);

/* -- API Wrapper methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_upload_file(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_up_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_up_upload_buffer(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_up_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STOR_UPLOADER_H__
