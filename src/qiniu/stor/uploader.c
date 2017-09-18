#include "qiniu/stor/uploader.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ---- Definition of Uploader (Abbreviation: up) ---- */

typedef struct _QN_UPLOADER
{
} qn_uploader_st, *qn_uploader_ptr;

/* -- Constructor & Destructor methods -- */ 

QN_SDK qn_uploader_ptr qn_up_create(void)
{
}

QN_SDK qn_uploader_ptr qn_up_create_ex(qn_http_connection_ptr restrict conn, qn_svc_selector restrict sel)
{
}

QN_SDK void qn_up_destroy(qn_uploader_ptr restrict up)
{
}

/* -- API methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_api_upload(qn_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_up_extra_ptr restrict ext)
{
}

/* -- API Wrapper methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_upload_file(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_up_extra_ptr restrict ext)
{
}

QN_SDK extern qn_json_object_ptr qn_up_upload_buffer(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_up_extra_ptr restrict ext)
{
}

#ifdef __cplusplus
}
#endif
