#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/service.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_SERVICE
{
    qn_svc_entry_st * entries;
    unsigned int cnt:8;
    unsigned int cap:8;
    unsigned int type:8;
} qn_service_st;

static qn_service_ptr qn_svc_allocate(unsigned int cap)
{
    qn_service_ptr new_svc = calloc(1, sizeof(qn_service_st));
    if (! new_svc) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_svc->cnt = 0;
    new_svc->cap = cap;
    new_svc->entries = calloc(new_svc->cap, sizeof(qn_svc_entry_st));
    if (! new_svc->entries) {
        free(new_svc);
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_svc;
}

QN_SDK qn_service_ptr qn_svc_create(qn_svc_type type)
{
    qn_service_ptr new_svc = qn_svc_allocate(4);
    if (new_svc) {
        new_svc->type = type;
    } // if
    return new_svc;
}

QN_SDK void qn_svc_destroy(qn_service_ptr restrict svc)
{
    if (svc) {
        qn_svc_reset(svc);
        free(svc->entries);
        svc->entries = NULL;
        free(svc);
    } // if
}

QN_SDK void qn_svc_reset(qn_service_ptr restrict svc)
{
    unsigned int i = 0;
    assert(svc);
    for (i = 0; i < svc->cnt; i += 1) {
        qn_str_destroy(svc->entries[i].base_url);
        qn_str_destroy(svc->entries[i].hostname);
    } // for
    memset(svc->entries, 0, sizeof(qn_svc_entry_st) * svc->cnt);
    svc->cnt = 0;
}

QN_SDK qn_service_ptr qn_svc_duplicate(qn_service_ptr restrict svc)
{
    unsigned int i = 0;
    qn_service_ptr new_svc = NULL;

    assert(svc);

    if ((svc = qn_svc_allocate(svc->cap))) {
        for (i = 0; i < svc->cnt; i += 1) {
            if (! (new_svc->entries[i].base_url = qn_str_duplicate(svc->entries[i].base_url)) ) {
                qn_svc_destroy(new_svc);
                return NULL;
            } // if

            if (svc->entries[i].hostname) {
                if (! (new_svc->entries[i].hostname = qn_str_duplicate(svc->entries[i].hostname)) ) {
                    qn_str_destroy(new_svc->entries[i].base_url);
                    qn_svc_destroy(new_svc);
                    return NULL;
                } // if
            } // if

            new_svc->cnt += 1;
        } // if
        new_svc->type = svc->type;
    } // if
    return new_svc;
}

QN_SDK unsigned int qn_svc_entry_count(qn_service_ptr restrict svc)
{
    assert(svc);
    return svc->cnt;
}

QN_SDK qn_svc_entry_ptr qn_svc_get_entry(qn_service_ptr restrict svc, unsigned int n)
{
    assert(svc);
    if (svc->cnt < n) return NULL;
    return &svc->entries[n];
}

static qn_bool qn_svc_augment(qn_service_ptr restrict svc)
{
    unsigned int new_cap = svc->cap + (svc->cap >> 1); // 1.5 times
    qn_svc_entry_st * new_entries = calloc(new_cap < 255 ? new_cap : 255, sizeof(qn_svc_entry_st));

    if (! new_entries) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_entries, svc->entries, sizeof(qn_svc_entry_st) * svc->cnt);
    free(svc->entries);

    svc->entries = new_entries;
    svc->cap = new_cap & 0xFF;
    return qn_true;
}

static inline qn_bool qn_svc_try_to_augment(qn_service_ptr restrict svc)
{
    if (svc->cnt == svc->cap) {
        if (svc->cap == 255) {
            qn_err_set_out_of_capacity();
            return qn_false;
        } // if
        return qn_svc_augment(svc);
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_svc_add_entry(qn_service_ptr restrict svc, qn_svc_entry_ptr restrict ent)
{
    assert(svc);
    assert(ent);

    if (! qn_svc_try_to_augment(svc)) return qn_false;

    svc->entries[svc->cnt].base_url = qn_str_duplicate(ent->base_url);
    if (! svc->entries[svc->cnt].base_url) return qn_false;

    if (ent->hostname && ! (svc->entries[svc->cnt].hostname = qn_str_duplicate(ent->hostname))) {
        qn_str_destroy(svc->entries[svc->cnt].base_url);
        svc->entries[svc->cnt].base_url = NULL;
        return qn_false;
    } // if

    svc->cnt += 1;
    return qn_true;
}

static qn_svc_entry_st qn_svc_default_up_entry = { "http://up.qiniu.com", NULL };
static qn_svc_entry_st qn_svc_default_io_entry = { "http://iovip.qbox.me", NULL };
static qn_svc_entry_st qn_svc_default_rs_entry = { "http://rs.qiniu.com", NULL };
static qn_svc_entry_st qn_svc_default_rsf_entry = { "http://rsf.qbox.me", NULL };
static qn_svc_entry_st qn_svc_default_api_entry = { "http://api.qiniu.com", NULL };

static qn_service_st qn_svc_default_services[QN_SVC_COUNT] = {
    { &qn_svc_default_up_entry, 1, 1, QN_SVC_UP },
    { &qn_svc_default_io_entry, 1, 1, QN_SVC_IO },
    { &qn_svc_default_rs_entry, 1, 1, QN_SVC_RS },
    { &qn_svc_default_rsf_entry, 1, 1, QN_SVC_RSF },
    { &qn_svc_default_api_entry, 1, 1, QN_SVC_API }
};

QN_SDK qn_service_ptr qn_svc_get_default_service(qn_svc_type type)
{
    return &qn_svc_default_services[type];
}

#ifdef __cplusplus
}
#endif
