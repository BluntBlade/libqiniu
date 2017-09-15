#ifndef __QN_STOR_RESUMABLE_UPLOADER_H__
#define __QN_STOR_RESUMABLE_UPLOADER_H__

#include "qiniu/service.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Resumable Uploader Progress (Abbreviation: ru_prog)

struct _QN_RU_PROGESS;
typedef struct _QN_RU_PROGESS * qn_ru_progress_ptr;

QN_SDK extern qn_ru_progress_ptr qn_ru_prog_create(qn_io_reader_itf restrict data_rdr);
QN_SDK extern void qn_ru_prog_destroy(qn_ru_progress_ptr restrict prog);

// ----

QN_SDK extern unsigned int qn_ru_prog_block_count(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_fsize qn_ru_prog_total_file_size(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_fsize qn_ru_prog_uploaded_file_size(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_bool qn_ru_prog_file_uploaded(qn_ru_progress_ptr restrict prog);

// ----

QN_SDK extern qn_json_object_ptr qn_ru_prog_get_block(qn_ru_progress_ptr restrict prog, unsigned int idx);
QN_SDK extern qn_json_object_ptr qn_ru_prog_update_block(qn_ru_progress_ptr restrict prog, unsigned int idx, qn_json_object_ptr restrict up_ret);

QN_SDK extern qn_size qn_ru_prog_uploaded_block_size(qn_json_object_ptr restrict blk);
QN_SDK extern qn_bool qn_ru_prog_block_uploaded(qn_json_object_ptr restrict blk);

QN_SDK extern qn_io_reader_itf qn_ru_prog_create_block_reader(qn_ru_progress_ptr restrict prog, unsigned idx, qn_json_object_ptr * restrict blk);
QN_SDK extern qn_io_reader_itf qn_ru_prog_to_context_reader(qn_ru_progress_ptr restrict prog);

// ----

QN_SDK extern qn_string qn_ru_prog_to_string(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_ru_progress_ptr qn_ru_prog_from_string(const char * restrict str, qn_size str_len);

QN_SDK extern qn_bool qn_ru_prog_attach_reader(qn_ru_progress_ptr restrict prog, qn_io_reader_itf restrict data_rdr);

// ---- Declaration of Resumable Uploader (Abbreviation: ru)

struct _QN_RESUMABLE_UPLOADER;
typedef struct _QN_RESUMABLE_UPLOADER * qn_resumable_uploader_ptr;

QN_SDK qn_resumable_uploader_ptr qn_ru_create(qn_service_ptr restrict svc);
QN_SDK void qn_ru_destroy(qn_resumable_uploader_ptr restrict up);

QN_SDK extern qn_json_object_ptr qn_ru_api_upload(qn_resumable_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_ru_extra_ptr restrict ext);

QN_SDK extern qn_json_object_ptr qn_ru_upload_file(qn_resumable_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_ru_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_ru_upload_buffer(qn_resumable_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_ru_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STOR_RESUMABLE_UPLOADER_H__
