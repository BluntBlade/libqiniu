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

/* -- API methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_api_upload(qn_uploader_ptr restrict up, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict upe)
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

    if (upe) {
        extern_ent.base_url = qn_json_get_string(upe, "base_url", NULL);
        extern_ent.hostname = qn_json_get_string(upe, "hostname", NULL);
        ent = &extern_ent;

        mime_type = upe->mime_type;
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
        if (ent != &extern_ent) qn_http_check_and_register_connection_failure(up->sel, ent);
        return NULL;
    } // if
    return ret;
}

/* -- API Wrapper methods -- */ 

QN_SDK extern qn_json_object_ptr qn_up_upload_file(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict fname, qn_json_object_ptr restrict ext)
{
}

QN_SDK extern qn_json_object_ptr qn_up_upload_buffer(qn_uploader_ptr restrict up, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_json_object_ptr restrict ext)
{
}

#ifdef __cplusplus
}
#endif
