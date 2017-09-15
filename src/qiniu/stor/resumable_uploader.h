#ifndef __QN_STOR_RESUMABLE_UPLOADER_H__
#define __QN_STOR_RESUMABLE_UPLOADER_H__

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

/* ---- Declaration of Resumable Uploader Progress (Abbreviation: ru_prog) ---- */

typedef qn_json_object_ptr qn_ru_block_ptr;

struct _QN_RU_PROGESS;
typedef struct _QN_RU_PROGESS * qn_ru_progress_ptr;

/* -- Constructor & Destructor methods -- */ 

QN_SDK extern qn_ru_progress_ptr qn_ru_prog_create(qn_io_reader_itf restrict data_rdr);
QN_SDK extern void qn_ru_prog_destroy(qn_ru_progress_ptr restrict prog);

/* -- Property methods -- */ 

QN_SDK extern qn_uint qn_ru_prog_block_count(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_fsize qn_ru_prog_total_file_size(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_fsize qn_ru_prog_uploaded_file_size(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_bool qn_ru_prog_file_uploaded(qn_ru_progress_ptr restrict prog);

/* -- Block & Context methods -- */ 

QN_SDK extern qn_ru_block_ptr qn_ru_prog_get_block(qn_ru_progress_ptr restrict prog, qn_uint idx);
QN_SDK extern qn_ru_block_ptr qn_ru_prog_update_block(qn_ru_progress_ptr restrict prog, qn_uint idx, qn_json_object_ptr restrict up_ret);

QN_SDK extern qn_size qn_ru_prog_uploaded_block_size(qn_ru_block_ptr restrict blk);
QN_SDK extern qn_bool qn_ru_prog_block_uploaded(qn_ru_block_ptr restrict blk);

QN_SDK extern qn_io_reader_itf qn_ru_prog_create_block_reader(qn_ru_progress_ptr restrict prog, qn_uint idx, qn_ru_block_ptr * restrict blk);
QN_SDK extern qn_io_reader_itf qn_ru_prog_to_context_reader(qn_ru_progress_ptr restrict prog);

/* -- Serialization methods -- */ 

QN_SDK extern qn_string qn_ru_prog_to_string(qn_ru_progress_ptr restrict prog);
QN_SDK extern qn_ru_progress_ptr qn_ru_prog_from_string(const char * restrict str, qn_size str_len);

QN_SDK extern qn_bool qn_ru_prog_attach_reader(qn_ru_progress_ptr restrict prog, qn_io_reader_itf restrict data_rdr);

/* ---- Declaration of Resumable Uploader (Abbreviation: ru) ---- */

enum
{
    QN_RU_CHUNK_DEFAULT_SIZE = (1024 * 256),
    QN_RU_BLOCK_MAX_SIZE = (1024 * 1024 * 4),
    QN_RU_BLOCK_LAST_INDEX = (-1)
};

typedef struct _QN_RU_EXTRA
{
    const char * mime_type;
    const char * accept_type;
    const char * crc32;
    const char * save_key;

    qn_ud_variable_ptr user_defined_vars;

    qn_svc_entry_ptr entry;
} qn_ru_extra_st, *qn_ru_extra_ptr;

struct _QN_RESUMABLE_UPLOADER;
typedef struct _QN_RESUMABLE_UPLOADER * qn_resumable_uploader_ptr;

/* -- Constructor & Destructor methods -- */ 

QN_SDK qn_resumable_uploader_ptr qn_ru_create(qn_service_ptr restrict svc);
QN_SDK void qn_ru_destroy(qn_resumable_uploader_ptr restrict ru);

/* -- API methods -- */ 

QN_SDK extern qn_json_object_ptr qn_ru_api_mkblk(qn_resumable_uploader_ptr restrict ru, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_ru_block_ptr restrict blk, qn_uint chk_size, qn_ru_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_ru_api_bput(qn_resumable_uploader_ptr restrict ru, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_ru_block_ptr restrict blk, qn_uint chk_size, qn_ru_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_ru_api_mkfile(qn_resumable_uploader_ptr restrict ru, const char * restrict uptoken, qn_io_reader_itf restrict ctx_rdr, qn_ru_block_ptr restrict last_blk, qn_fsize fsize, qn_ru_extra_ptr restrict ext);

/* -- API Wrapper methods -- */ 

QN_SDK extern qn_json_object_ptr qn_ru_upload_file(qn_resumable_uploader_ptr restrict ru, const char * restrict uptoken, const char * restrict fname, qn_ru_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_ru_upload_buffer(qn_resumable_uploader_ptr restrict ru, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_ru_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STOR_RESUMABLE_UPLOADER_H__
