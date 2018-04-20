#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/os/file.h"
#include "qiniu/etag.h"
#include "qiniu/region.h"
#include "qiniu/storage.h"
#include "qiniu/reader_filter.h"
#include "qiniu/region_table.h"
#include "qiniu/ud/variable.h"

#include "qiniu/easy.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_EASY_PUT_EXTRA
{
    struct {
        const char * final_key;     // Final key of the file in the bucket.
        const char * mime_type;     // MIME type of the file in the bucket.
        const char * owner_desc;    // The owner description of the file, used for business identificaiton.
        qn_string local_qetag;      // The local QETAG hash calculated by filters.
    } attr;

    struct {
        unsigned int check_crc32:1;  // Calculate the CRC-32 checksum locally to verify the uploaded content.
        unsigned int check_md5:1;    // Calculate the MD5 checksum locally to verify the uploaded content.
        unsigned int check_qetag:1;  // Calculate the Qiniu-ETAG checksum locally to verify the uploaded content.

        unsigned int extern_ss:1;    // Use an external storage session object.

        // Specifies a const-volatile boolean variable to check if need to abort or not.
        const volatile qn_bool * abort;

        qn_size min_resumable_fsize; // The minimum file size to enable resumable uploading.
                                    // It must be less than 500MB due to the limit set by the server, and larger than
                                    // 4MB since uploading a small file in multiple HTTP round trips is not efficient.
                                    // If set less than 4MB, the default size is 10MB.

        qn_uint32 fcrc32;           // The CRC-32 of the local file provided by the caller.
        qn_fsize fsize;             // The size of the local file provided by the caller.
        qn_io_reader_itf rdr;       // A customized data reader provided by the caller.

        qn_string resumable_info;
        qn_ud_variable_ptr ud_vars; // User-Defined Variables.

        qn_svc_entry_ptr ent;       // Service entry.
    } put_ctrl;

    struct {
        qn_etag_context_ptr qetag;
    } temp;
} qn_easy_put_extra_st;

QN_SDK qn_easy_put_extra_ptr qn_easy_pe_create(void)
{
    qn_easy_put_extra_ptr new_pe = calloc(1, sizeof(qn_easy_put_extra_st));
    if (! new_pe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_pe;
}

QN_SDK void qn_easy_pe_destroy(qn_easy_put_extra_ptr restrict pe)
{
    if (pe) {
        qn_str_destroy(pe->put_ctrl.resumable_info);

        if (pe->attr.local_qetag) qn_str_destroy(pe->attr.local_qetag);

        if (pe->temp.qetag) qn_etag_ctx_destroy(pe->temp.qetag);
        free(pe);
    } // if
}

QN_SDK void qn_easy_pe_reset(qn_easy_put_extra_ptr restrict pe)
{
    if (pe->attr.local_qetag) qn_str_destroy(pe->attr.local_qetag);

    memset(&pe->put_ctrl, 0, sizeof(pe->put_ctrl));
    memset(&pe->attr, 0, sizeof(pe->attr));
}

QN_SDK void qn_easy_pe_set_final_key(qn_easy_put_extra_ptr restrict pe, const char * restrict key)
{
    pe->attr.final_key = key;
}

QN_SDK void qn_easy_pe_set_mime_type(qn_easy_put_extra_ptr restrict pe, const char * restrict mime_type)
{
    pe->attr.mime_type = mime_type;
}

QN_SDK void qn_easy_pe_set_owner_description(qn_easy_put_extra_ptr restrict pe, const char * restrict desc)
{
    pe->attr.owner_desc = desc;
}

QN_SDK void qn_easy_pe_set_crc32_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check)
{
    pe->put_ctrl.check_crc32 = (check) ? 1 : 0;
}

QN_SDK void qn_easy_pe_set_md5_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check)
{
    pe->put_ctrl.check_md5 = (check) ? 1 : 0;
}

QN_SDK void qn_easy_pe_set_qetag_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check)
{
    if (check) {
        pe->put_ctrl.check_qetag = 1;
        if (! pe->temp.qetag) pe->temp.qetag = qn_etag_ctx_create();
    } else {
        pe->put_ctrl.check_qetag = 0;
    } // if
}

QN_SDK const qn_string qn_easy_pe_get_qetag(qn_easy_put_extra_ptr restrict pe)
{
    return pe->attr.local_qetag;
}

QN_SDK void qn_easy_pe_set_abort_variable(qn_easy_put_extra_ptr restrict pe, const volatile qn_bool * abort)
{
    pe->put_ctrl.abort = abort;
}

QN_SDK void qn_easy_pe_set_min_resumable_fsize(qn_easy_put_extra_ptr restrict pe, qn_size fsize)
{
    pe->put_ctrl.min_resumable_fsize = fsize;
}

QN_SDK void qn_easy_pe_set_local_crc32(qn_easy_put_extra_ptr restrict pe, qn_uint32 crc32)
{
    pe->put_ctrl.fcrc32 = crc32;
}

QN_SDK void qn_easy_pe_set_source_reader(qn_easy_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize)
{
    pe->put_ctrl.rdr = rdr;
    pe->put_ctrl.fsize = fsize;
}

QN_SDK void qn_easy_pe_set_user_defined_variables(qn_easy_put_extra_ptr restrict pe, qn_ud_variable_ptr ud_vars)
{
    pe->put_ctrl.ud_vars = ud_vars;
}

QN_SDK qn_bool qn_easy_pe_clone_and_set_resumable_info(qn_easy_put_extra_ptr restrict pe, const char * restrict str, qn_size str_size)
{
    qn_string tmp = qn_cs_clone(str, str_size);
    if (! tmp) return qn_false;

    qn_str_destroy(pe->put_ctrl.resumable_info);
    pe->put_ctrl.resumable_info = tmp;
    return qn_true;
}

QN_SDK qn_string qn_easy_pe_get_resumable_info(qn_easy_put_extra_ptr restrict pe)
{
    return pe->put_ctrl.resumable_info;
}

QN_SDK void qn_easy_pe_set_service_entry(qn_easy_put_extra_ptr restrict pe, qn_svc_entry_ptr restrict ent)
{
    pe->put_ctrl.ent = ent;
}

// ----

typedef struct _QN_EASY
{
    qn_storage_ptr stor;
    qn_json_parser_ptr json_prs;
    qn_rgn_table_ptr rtbl;
    qn_rgn_service_ptr rsvc;
    qn_svc_selector_ptr sel;
    qn_string bucket;
} qn_easy_st;

QN_SDK qn_easy_ptr qn_easy_create(void)
{
    qn_easy_ptr new_easy = calloc(1, sizeof(qn_easy_st));
    if (! new_easy) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_easy->stor = qn_stor_create();
    if (! new_easy->stor) {
        free(new_easy);
        return NULL;
    } // if
    return new_easy;
}

QN_SDK void qn_easy_destroy(qn_easy_ptr restrict easy)
{
    if (easy) {
        if (easy->json_prs) qn_json_prs_destroy(easy->json_prs);
        if (easy->bucket) qn_str_destroy(easy->bucket);
        if (easy->sel) qn_svc_sel_destroy(easy->sel);
        if (easy->rsvc) qn_rgn_svc_destroy(easy->rsvc);
        if (easy->rtbl) qn_rgn_tbl_destroy(easy->rtbl);
        qn_stor_destroy(easy->stor);
        free(easy);
    } // if
}

#define QN_EASY_MB_UNIT (1 << 20)

static void qn_easy_init_put_extra(qn_easy_put_extra_ptr ext, qn_easy_put_extra_ptr real_ext)
{
    memcpy(real_ext, ext, sizeof(qn_easy_put_extra_st));

    if (real_ext->put_ctrl.min_resumable_fsize < (4L * QN_EASY_MB_UNIT)) {
        real_ext->put_ctrl.min_resumable_fsize = (10L * QN_EASY_MB_UNIT);
    } else if (real_ext->put_ctrl.min_resumable_fsize > (500L * QN_EASY_MB_UNIT)) {
        real_ext->put_ctrl.min_resumable_fsize = (500L * QN_EASY_MB_UNIT);
    } // if

    if (real_ext->temp.qetag) qn_etag_ctx_init(real_ext->temp.qetag);
}

static qn_io_reader_itf qn_easy_create_put_reader(const char * restrict fname, qn_easy_put_extra_ptr real_ext)
{
    qn_file_ptr fl = NULL;
    qn_io_reader_itf io_rdr;
    qn_reader_ptr rdr;
    qn_rdr_pos filter_num = 0;

    if (real_ext->put_ctrl.rdr) {
        io_rdr = real_ext->put_ctrl.rdr;
    } else {
        fl = qn_fl_open(fname, NULL);
        if (! fl) return NULL;

        real_ext->put_ctrl.fsize = qn_fl_fsize(fl);
        io_rdr = qn_fl_to_io_reader(fl);
    } // if

/*
    if (real_ext->put_ctrl.check_crc32) filter_num += 1;
    if (real_ext->put_ctrl.check_md5) filter_num += 1;
    if (real_ext->put_ctrl.abort) filter_num += 1;
*/

    if (real_ext->put_ctrl.check_qetag) filter_num += 1;

    if (filter_num == 0) return io_rdr;

    rdr = qn_rdr_create(io_rdr, filter_num);
    qn_fl_close(fl);
    if (! rdr) return NULL;

    if (real_ext->put_ctrl.check_qetag) qn_rdr_add_post_filter(rdr, real_ext->temp.qetag, &qn_flt_etag_update_cfn);

    // TODO: Implement filters.

    return qn_rdr_to_io_reader(rdr);
}

static qn_json_object_ptr qn_easy_put_file_in_one_piece(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_io_reader_itf restrict io_rdr, qn_easy_put_extra_ptr restrict ext)
{
    qn_json_object_ptr ret;
    qn_stor_upload_extra_ptr upe;

    if (! (upe = qn_stor_upe_create())) return NULL;

    qn_stor_upe_set_final_key(upe, ext->attr.final_key);
    qn_stor_upe_set_mime_type(upe, ext->attr.mime_type);
    qn_stor_upe_set_user_defined_variables(upe, ext->put_ctrl.ud_vars);
    if (ext->put_ctrl.ent) qn_stor_upe_set_service_entry(upe, ext->put_ctrl.ent);

    if (io_rdr) {
        ret = qn_stor_up_api_upload(easy->stor, uptoken, io_rdr, upe);
    } else {
        ret = qn_stor_up_api_upload_file(easy->stor, uptoken, fname, upe);
    } // if
    qn_stor_upe_destroy(upe);
    return ret;
}

static qn_json_object_ptr qn_easy_put_huge_imp(qn_easy_ptr restrict easy, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_easy_put_extra_ptr restrict ext)
{
    int start_idx = 0;
    qn_json_integer code = 0;
    qn_string resumable_info = NULL;
    qn_stor_upload_extra_ptr upe = NULL;
    qn_json_object_ptr put_ret = NULL;
    qn_stor_resumable_upload_ptr ru = NULL;

    if (ext) {
        if (! (upe = qn_stor_upe_create())) return NULL;

        qn_stor_upe_set_final_key(upe, ext->attr.final_key);
        qn_stor_upe_set_mime_type(upe, ext->attr.mime_type);
        qn_stor_upe_set_user_defined_variables(upe, ext->put_ctrl.ud_vars);
        if (ext->put_ctrl.ent) qn_stor_upe_set_service_entry(upe, ext->put_ctrl.ent);

        resumable_info = ext->put_ctrl.resumable_info;
    } // if

    if (resumable_info) {
        ru = qn_stor_ru_from_string(qn_str_cstr(resumable_info), qn_str_size(resumable_info));
        if (! ru || ! qn_stor_ru_attach(ru, data_rdr)) goto QN_EASY_PUT_HUGE_ERROR_HANDLING;
    } else {
        ru = qn_stor_ru_create(data_rdr);
        if (! ru) goto QN_EASY_PUT_HUGE_ERROR_HANDLING;
    } // if

    put_ret = qn_stor_ru_upload_huge(easy->stor, uptoken, ru, &start_idx, QN_STOR_RU_CHUNK_DEFAULT_SIZE, upe);
    qn_str_destroy(ext->put_ctrl.resumable_info);

    if (ext) {
        if (! put_ret) {
            ext->put_ctrl.resumable_info = qn_stor_ru_to_string(ru);
        } else {
            code = -1;
            qn_json_obj_get_integer(put_ret, "fn-code", &code);
            if (code == 200) {
                ext->put_ctrl.resumable_info = NULL;
            } else {
                ext->put_ctrl.resumable_info = qn_stor_ru_to_string(ru);
            } // if
        } // if
    } // if

QN_EASY_PUT_HUGE_ERROR_HANDLING:
    qn_stor_ru_destroy(ru);
    qn_stor_upe_destroy(upe);
    return put_ret;
}

static qn_bool qn_easy_parse_putting_policy(qn_easy_ptr restrict easy, const char * restrict str, qn_size str_size, qn_json_object_ptr * restrict pp)
{
    qn_size pp_size;
    qn_bool ret;
    qn_string pp_str;

    if (! easy->json_prs) {
        easy->json_prs = qn_json_prs_create();
        if (! easy->json_prs) return qn_false;
    } // if

    pp_str = qn_cs_decode_base64_urlsafe(str, str_size);
    if (! pp_str) return qn_false;

    pp_size = qn_str_size(pp_str);
    ret = qn_json_prs_parse_object(easy->json_prs, pp_str, &pp_size, pp);
    qn_str_destroy(pp_str);
    if (! ret) qn_err_easy_set_invalid_put_policy();
    return ret;
}

static qn_svc_entry_ptr qn_easy_select_putting_entry(qn_easy_ptr restrict easy, const char * restrict uptoken, qn_json_object_ptr * restrict pp, qn_easy_put_extra_ptr restrict ext)
{
    qn_integer ttl = 0;
    const char * pos = NULL;
    qn_bool rgn_updated = qn_false;
    qn_string access_key = NULL;
    qn_string bucket = NULL;
    qn_string scope = NULL;
    qn_region_ptr rgn = NULL;
    qn_service_ptr svc = NULL;
    qn_rgn_auth_st rgn_auth;

    if (! easy->rtbl && ! (easy->rtbl = qn_rgn_tbl_create())) return NULL;

    // ---- Parse the access key and the bucket.
    pos = qn_str_find_char(uptoken, ':');
    if (! pos) {
        qn_err_easy_set_invalid_uptoken();
        return NULL;
    } // if

    access_key = qn_cs_clone(uptoken, pos - uptoken);

    if (!*pp) {
        pos = qn_str_find_char(pos + 1, ':');
        if (! pos) {
            qn_str_destroy(access_key);
            qn_err_easy_set_invalid_uptoken();
            return NULL;
        } // if

        if (! qn_easy_parse_putting_policy(easy, pos + 1, posix_strlen(uptoken) - (pos + 1 - uptoken), pp)) {
            qn_str_destroy(access_key);
            return NULL;
        } // if
    } // if

    scope = NULL;
    if (! qn_json_obj_get_string(*pp, "scope", &scope)) {
        qn_str_destroy(access_key);
        return NULL;
    }

    if (! scope) {
        qn_str_destroy(access_key);
        qn_err_easy_set_invalid_put_policy();
        return NULL;
    } // if

    pos = qn_str_find_char(qn_str_cstr(scope), ':');
    if (pos) {
        bucket = qn_cs_clone(qn_str_cstr(scope), pos - qn_str_cstr(scope));
    } else {
        bucket = qn_str_duplicate(scope);
    } // if

    // ---- Check if the region entry information is still alive
    if (! (rgn = qn_rgn_tbl_get_region(easy->rtbl, bucket))) {
        if (! easy->rsvc && ! (easy->rsvc = qn_rgn_svc_create())) goto QN_EASY_SELECT_PUTTING_ENTRY_ERROR_HANDLING;

        rgn_auth.v1.access_key = qn_str_cstr(access_key);

        rgn = qn_rgn_svc_grab_entry_info(easy->rsvc, &rgn_auth, bucket, &ttl);
        if (! rgn || ! qn_rgn_tbl_set_region(easy->rtbl, bucket, ttl, rgn)) goto QN_EASY_SELECT_PUTTING_ENTRY_ERROR_HANDLING;
        rgn_updated = qn_true;
    } // if

    if (qn_str_compare(easy->bucket, bucket) != 0 || rgn_updated) {
        if (! (svc = qn_rgn_get_service(rgn, QN_SVC_UP))) svc = qn_svc_get_default_service(QN_SVC_UP);

        if (easy->sel) qn_svc_sel_destroy(easy->sel);
        if (! (easy->sel = qn_svc_sel_create(svc, QN_SVC_SEL_LAST_SUCCEEDED_FIRST, QN_SVC_SEL_ANY))) goto QN_EASY_SELECT_PUTTING_ENTRY_ERROR_HANDLING;

        qn_str_destroy(easy->bucket);
        easy->bucket = bucket;
        bucket = NULL;
    } // if

    if (bucket) qn_str_destroy(bucket);
    qn_str_destroy(access_key);
    return qn_svc_sel_next_entry(easy->sel);

QN_EASY_SELECT_PUTTING_ENTRY_ERROR_HANDLING:
    qn_str_destroy(bucket);
    qn_str_destroy(access_key);
    return NULL;
}

static qn_bool qn_easy_check_putting_key(qn_easy_ptr restrict easy, const char * restrict uptoken, qn_json_object_ptr * restrict pp, qn_easy_put_extra_ptr restrict ext)
{
    const char * pos;
    qn_string scope;

    if (!*pp) {
        pos = qn_str_find_char(uptoken, ':');
        if (! pos) {
            qn_err_easy_set_invalid_uptoken();
            return qn_false;
        } // if

        pos = qn_str_find_char(pos + 1, ':');
        if (! pos) {
            qn_err_easy_set_invalid_uptoken();
            return qn_false;
        } // if

        if (! qn_easy_parse_putting_policy(easy, pos + 1, posix_strlen(uptoken) - (pos + 1 - uptoken), pp)) return qn_false;
    } // if

    scope = NULL;
    if (! qn_json_obj_get_string(*pp, "scope", &scope)) return qn_false;
    if (! scope) {
        qn_err_easy_set_invalid_put_policy();
        return qn_false;
    } // if

    pos = qn_str_find_char(qn_str_cstr(scope), ':');
    if (pos) ext->attr.final_key = pos + 1;
    return qn_true;
}

QN_SDK qn_json_object_ptr qn_easy_put_file(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_easy_put_extra_ptr restrict ext)
{
    qn_json_integer code = 0;
    qn_string tmp_str;
    qn_io_reader_itf io_rdr;
    qn_json_object_ptr put_ret;
    qn_easy_put_extra_st real_ext;
    qn_json_object_ptr pp = NULL;

    qn_easy_init_put_extra(ext, &real_ext);
    
    // ---- Make some information checks and choose some strategies.
    if (! real_ext.attr.final_key) {
        if (! qn_easy_check_putting_key(easy, uptoken, &pp, &real_ext)) {
            qn_json_obj_destroy(pp);
            return NULL;
        } // if
    } // if

    io_rdr = qn_easy_create_put_reader(fname, &real_ext);
    if (! io_rdr) {
        qn_json_obj_destroy(pp);
        return NULL;
    } // if

    if (real_ext.put_ctrl.fsize <= real_ext.put_ctrl.min_resumable_fsize) {
        put_ret = qn_easy_put_file_in_one_piece(easy, uptoken, fname, io_rdr, &real_ext);
    } else {
        put_ret = qn_easy_put_huge_imp(easy, uptoken, io_rdr, &real_ext);
    } // if

    qn_io_rdr_close(io_rdr);
    qn_json_obj_destroy(pp);

    if (! put_ret) return put_ret;
    
    qn_json_obj_get_integer(put_ret, "fn-code", &code);
    if (code == 200) {
        if (real_ext.put_ctrl.check_qetag) {
            if (ext->attr.local_qetag) qn_str_destroy(ext->attr.local_qetag);
            ext->attr.local_qetag = qn_etag_ctx_final(real_ext.temp.qetag);
            tmp_str = NULL;
            if (! qn_json_obj_get_string(put_ret, "hash", &tmp_str)) return NULL;

            if (ext->attr.local_qetag && tmp_str && qn_str_compare(ext->attr.local_qetag, tmp_str) != 0) {
                qn_json_obj_set_integer(put_ret, "fn-code", 9999);
                qn_json_obj_set_string(put_ret, "fn-error", "[EASY] Failed in QETAG hash checking");
            } // if
        } // if
    } // if
    return put_ret;
}

QN_SDK qn_json_object_ptr qn_easy_put_huge_file(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_easy_put_extra_ptr restrict ext)
{
    qn_json_object_ptr pp = NULL;
    qn_json_object_ptr put_ret;
    qn_file_ptr fl;
    qn_easy_put_extra_st real_ext;

    // ---- Check preconditions.
    assert(easy);
    assert(uptoken);
    assert(fname);

    qn_easy_init_put_extra(ext, &real_ext);
    if (! real_ext.attr.final_key) {
        if (! qn_easy_check_putting_key(easy, uptoken, &pp, &real_ext)) {
            qn_json_obj_destroy(pp);
            return NULL;
        } // if
    } // if

    fl = qn_fl_open(fname, NULL);
    if (! fl) {
        qn_json_obj_destroy(pp);
        return NULL;
    } // if

    put_ret = qn_easy_put_huge_imp(easy, uptoken, qn_fl_to_io_reader(fl), &real_ext);
    qn_fl_close(fl);
    qn_json_obj_destroy(pp);
    return put_ret;
}

// ----

typedef struct _QN_EASY_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    unsigned int limit;
} qn_easy_list_extra_st;

QN_SDK qn_easy_list_extra_ptr qn_easy_le_create(void)
{
    qn_easy_list_extra_ptr new_le = calloc(1, sizeof(qn_easy_list_extra_st));
    if (! new_le) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_le;
}

QN_SDK void qn_easy_le_destroy(qn_easy_list_extra_ptr restrict le)
{
    if (le) {
        free(le);
    } // if
}

QN_SDK void qn_easy_le_set_prefix(qn_easy_list_extra_ptr restrict le, const char * restrict prefix, const char * delimiter)
{
    le->prefix = prefix;
    le->delimiter = delimiter;
}

QN_SDK void qn_easy_le_set_limit(qn_easy_list_extra_ptr restrict le, unsigned int limit)
{
    le->limit = limit;
}

// ----

QN_SDK qn_json_object_ptr qn_easy_list(qn_easy_ptr restrict easy, const qn_mac_ptr restrict mac, const char * restrict bucket, void * restrict itr_data, qn_easy_le_itr_callback_fn itr_cb, qn_easy_list_extra_ptr restrict ext)
{
    qn_json_integer code = 0;
    qn_string marker = NULL;
    qn_json_object_ptr list_ret;
    qn_json_object_ptr item;
    qn_json_array_ptr items;
    qn_stor_list_extra_ptr lse;
    qn_easy_list_extra_st real_ext;
    int i;

    assert(easy);
    assert(bucket);
    assert(itr_cb);

    if (ext) {
        memcpy(&real_ext, ext, sizeof(qn_easy_list_extra_st));
    } else {
        memset(&real_ext, 0, sizeof(qn_easy_list_extra_st));
    } // if

    if (real_ext.limit == 0 || real_ext.limit > 1000) real_ext.limit = 1000;

    lse = qn_stor_lse_create();
    if (! lse) return NULL;

    if (real_ext.prefix) qn_stor_lse_set_prefix(lse, real_ext.prefix, real_ext.delimiter);
    if (real_ext.limit) qn_stor_lse_set_limit(lse, real_ext.limit);

    do {
        if (marker) qn_stor_lse_set_marker(lse, qn_str_cstr(marker));

        list_ret = qn_stor_ls_api_list(easy->stor, mac, bucket, lse);
#if defined(QN_CFG_SUPPORT_MULTITHREAD)
        qn_str_destroy(marker);
#endif
        if (! list_ret) {
            qn_stor_lse_destroy(lse);
            return NULL;
        } // if

        code = 0;
        qn_json_obj_get_integer(list_ret, "fn-code", &code);
        if (code != 200) {
            qn_stor_lse_destroy(lse);
            return list_ret;
        } // if

        items = NULL;
        if (! qn_json_obj_get_array(list_ret, "items", &items)) return NULL;
        if (! items) {
            qn_stor_lse_destroy(lse);
            return list_ret;
        } // if

        for (i = 0; i < qn_json_arr_size(items); i += 1) {
            item = NULL;
            if (! qn_json_arr_get_object(items, i, &item)) return NULL;
            if (! itr_cb(itr_data, item)) {
                qn_stor_lse_destroy(lse);
                return NULL;
            } // if
        } // for

        marker = NULL;
        if (! qn_json_obj_get_string(list_ret, "marker", &marker)) {
            qn_stor_lse_destroy(lse);
            return NULL;
        }
    } while (qn_json_arr_size(items) == real_ext.limit && marker);

    return list_ret;
}

#ifdef __cplusplus
}
#endif

