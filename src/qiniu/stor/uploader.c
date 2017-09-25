#include "qiniu/stor/uploader.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ---- Definition of Uploader (Abbreviation: up) ---- */

typedef struct _QN_UPLOADER
{
    unsigned int use_external_rpc:1;
    unsigned int use_external_sel:1;
    qn_rpc_ptr rpc;
    qn_svc_selector_ptr sel;
} qn_uploader_st;

/* -- Constructor & Destructor methods -- */ 

static inline qn_svc_selector_ptr qn_up_create_default_selector(void)
{
    return qn_svc_sel_create(qn_svc_get_default_service(QN_SVC_UP), QN_SVC_SEL_LAST_SUCCEEDED_FIRST, QN_SVC_SEL_ANY);
}

QN_SDK qn_uploader_ptr qn_up_create(void)
{
    qn_uploader_ptr new_up = calloc(1, sizeof(qn_uploader_st));
    if (! new_up) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    if (! (new_up->rpc = qn_rpc_create())) {
        free(new_up);
        return NULL;
    } // if

    if (! (new_up->sel = qn_up_create_default_selector())) {
        qn_rpc_destroy(new_up->rpc);
        free(new_up);
        return NULL;
    } // if
    return new_up;
}

QN_SDK qn_uploader_ptr qn_up_create_ex(qn_rpc_ptr restrict rpc, qn_svc_selector_ptr restrict sel)
{
    qn_uploader_ptr new_up = calloc(1, sizeof(qn_uploader_st));
    if (! new_up) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    if (rpc) {
        new_up->rpc = rpc;
        new_up->use_external_rpc = 1;
    } else if (! (new_up->rpc = qn_rpc_create())) {
        free(new_up);
        return NULL;
    } // if

    if (sel) {
        new_up->sel = sel;
        new_up->use_external_sel = 1;
    } else if (! (new_up->sel = qn_up_create_default_selector())) {
        if (! new_up->use_external_rpc) qn_rpc_destroy(new_up->rpc);
        free(new_up);
        return NULL;
    } // if
    return new_up;
}

QN_SDK void qn_up_destroy(qn_uploader_ptr restrict up)
{
    if (up) {
        if (! up->use_external_rpc) qn_rpc_destroy(up->rpc);
        if (! up->use_external_sel) qn_sel_destroy(up->sel);
        free(up);
    } // if
}

/* -- Extra Arugment Setting methods -- */ 

QN_SDK qn_bool qn_up_set_mime_type(qn_json_object_ptr restrict ext, const char * restrict mime_type)
{
    assert(ext);
    assert(mime_type);
    return qn_json_set_cstr(ext, "mime_type", mime_type);
}

QN_SDK qn_bool qn_up_set_accept_type(qn_json_object_ptr restrict ext, const char * restrict accept_type)
{
    assert(ext);
    assert(accept_type);
    return qn_json_set_cstr(ext, "accept_type", accept_type);
}

QN_SDK qn_bool qn_up_set_crc32(qn_json_object_ptr restrict ext, const char * restrict crc32)
{
    assert(ext);
    assert(crc32);
    return qn_json_set_cstr(ext, "crc32", crc32);
}

QN_SDK qn_bool qn_up_set_save_key(qn_json_object_ptr restrict ext, const char * restrict save_key)
{
    assert(ext);
    assert(save_key);
    return qn_json_set_cstr(ext, "save_key", save_key);
}

QN_SDK qn_bool qn_up_set_user_defined_value(qn_json_object_ptr restrict ext, const char * restrict key, const char * val)
{
    qn_json_object ud = NULL;
    assert(ext);
    assert(key && val);
    if (! (ud = qn_json_get_object(ext, "user_defined_vars", NULL))) {
        if (! (ud = qn_json_create_and_set_object(ext, "user_defined_vars"))) return qn_false;
    } // if
    return qn_json_set_cstr(ud, key, val);
}

QN_SDK qn_bool qn_up_set_service_entry(qn_json_object_ptr restrict ext, qn_svc_entry_ptr restrict entry)
{
    assert(ext);
    assert(entry);
    if (! qn_json_set_cstr(ext, "base_url", entry->base_url)) return qn_false;
    if (entry->hostname && ! qn_json_set_cstr(ext, "hostname", entry->hostname)) return qn_false;
    return qn_true;
}

/* -- API methods -- */ 

static int qn_up_user_defined_variables_iterator_cfn(void * user_data, qn_json_class cls, qn_json_variant_ptr restrict val)
{
    qn_http_form_ptr form = (qn_http_form_ptr) user_data;
    if (cls == QN_JSON_STRING) {
        if (! qn_http_form_add_string(form, "crc32", val.string)) return QN_JSON_ITR_NO_MORE;
    } // if
    return QN_JSON_ITR_OK;
}

static qn_bool qn_up_set_arguments(qn_http_form_ptr restrict form, qn_json_object_ptr restrict ext)
{
    int i = 0;
    qn_string tmp = NULL;
    qn_json_array_ptr arr = NULL;
    qn_json_iterator_ptr itr = NULL;

    if ((tmp = qn_json_get_string(ext, "mime_type", NULL))) {
        if (! qn_http_form_add_string(form, "mime_type", tmp)) return qn_false;
    } // if

    if ((tmp = qn_json_get_string(ext, "accept_type", NULL))) {
        if (! qn_http_form_add_string(form, "accept_type", tmp)) return qn_false;
    } // if

    if ((tmp = qn_json_get_string(ext, "crc32", NULL))) {
        if (! qn_http_form_add_string(form, "crc32", tmp)) return qn_false;
    } // if

    if ((tmp = qn_json_get_string(ext, "save_key", NULL))) {
        if (! qn_http_form_add_string(form, "save_key", tmp)) return qn_false;
    } // if

    if ((arr = qn_json_get_array(ext, "user_defined_vars", NULL))) {
        if (! (itr = qn_json_itr_create())) return NULL;

        if (! qn_json_itr_push_array(itr, arr)) {
            qn_json_itr_destroy(itr);
            return NULL;
        } // if

        // TODO: Add user-defined variables.
    } // if
}

QN_SDK qn_json_object_ptr qn_up_api_upload(qn_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict ext)
{
    qn_bool ok = qn_false;
    const char * mime_type = NULL;
    qn_svc_entry_ptr ent = NULL;
    qn_http_form_ptr form = NULL;
    qn_json_object_ptr ret = NULL;
    qn_svc_entry_st extern_ent;

    assert(up);
    assert(uptoken);
    assert(data_rdr);

    if (ext) {
        extern_ent.base_url = qn_json_get_string(ext, "base_url", NULL);
        extern_ent.hostname = qn_json_get_string(ext, "hostname", NULL);
        ent = &extern_ent;

        mime_type = ext->mime_type;
    } // if
    if (! ent->base_url) ent = qn_svc_sel_next_entry(up->sel);

    if (! (form = qn_http_form_create())) return NULL;

    ok = qn_http_form_add_file_reader(form, "file", qn_str_cstr(qn_io_rdr_name(data_rdr)), NULL, qn_io_rdr_size(data_rdr), mime_type, data_rdr);
    if (! ok) {
        qn_http_form_destroy(form);
        return NULL;
    } // if

    if (ent->hostname && ! qn_rpc_set_request_header(up->rpc, "Host", qn_str_cstr(ent->hostname))) return NULL;
    ret = qn_rpc_post_form(up->rpc, qn_str_cstr(ent->base_url), form);
    if (! ret) {
        if (ent != &extern_ent && qn_http_failed_in_communication()) {
            qn_svc_sel_register_failed_entry(up->sel, ent);
        } // if
        return NULL;
    } // if
    return ret;
}

/* -- API Wrapper methods -- */ 

QN_SDK qn_json_object_ptr qn_up_upload_file(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_json_object_ptr restrict ext)
{
    qn_bool ok = qn_false;
    qn_json_object_ptr ret = NULL;
    qn_file_ptr fl = NULL;

    assert(up);
    assert(uptoken);
    assert(fname);

    if (! (fl = qn_fl_open(fname, NULL))) return NULL;
    ret = qn_up_api_upload(up, uptoken, qn_fl_to_io_reader(fl), ext);
    qn_fl_close(fl);
    return ret;
}

QN_SDK qn_json_object_ptr qn_up_upload_buffer(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_json_object_ptr restrict ext)
{
    return NULL;
}

#ifdef __cplusplus
}
#endif
