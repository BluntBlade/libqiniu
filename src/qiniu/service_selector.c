#include <assert.h>

#include "qiniu/service_selector.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_SVC_SELECTOR_ENTRY
{
    qn_svc_entry_ptr entry;
    unsigned int failures:16;
} qn_svc_selector_entry_st;

typedef struct _QN_SVC_SELECTOR
{
    qn_service_ptr svc;
    unsigned int strategy:8;
    unsigned int filter:8;
    unsigned int count:8;
    unsigned int next:8;
    qn_svc_selector_entry_st entries[1];
} qn_svc_selector_st;

QN_SDK qn_svc_selector_ptr qn_svc_sel_create(qn_service_ptr restrict svc, qn_svc_selector_strategy strategy, qn_svc_selector_filter filter)
{
    qn_svc_selector_ptr new_sel = NULL;
    
    assert(svc);

    new_sel = calloc(1, sizeof(qn_svc_selector_st) + sizeof(qn_svc_selector_entry_st) * (qn_svc_entry_count(svc) - 1)); 
    if (! new_sel) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_sel->svc = svc;
    new_sel->strategy = strategy;
    new_sel->filter = filter;

    qn_svc_sel_reset(new_sel);
    return new_sel;
}

QN_SDK void qn_svc_sel_destroy(qn_svc_selector_ptr restrict sel)
{
    if (sel) {
        free(sel);
    } // if
}

QN_SDK void qn_svc_sel_reset(qn_svc_selector_ptr restrict sel)
{
    unsigned int i = 0;
    qn_svc_entry_ptr ent = NULL;

    assert(sel);

    sel->next = 0;
    sel->count = 0;
    memset(&sel->entries, 0, sizeof(qn_svc_selector_entry_st) * qn_svc_entry_count(sel->svc));
    for (i = 0; i < qn_svc_entry_count(sel->svc); i += 1) {
        ent = qn_svc_get_entry(sel->svc, i);
        if ((sel->filter & QN_SVC_SEL_NO_HTTPS) && qn_svc_is_https_entry(ent)) {
            continue;
        } // if
        if ((sel->filter & QN_SVC_SEL_NO_HTTP) && qn_svc_is_http_entry(ent)) {
            continue;
        } // if

        sel->entries[sel->count].entry = ent;
        sel->count += 1;
    } // for
}

// ----

typedef qn_svc_entry_ptr (*qn_svc_sel_next_entry_fn)(qn_svc_selector_ptr restrict sel);

qn_svc_entry_ptr qn_svc_sel_next_entry_default(qn_svc_selector_ptr restrict sel)
{
    return sel->entries[sel->next].entry;
}

qn_svc_entry_ptr qn_svc_sel_next_entry_round_robin(qn_svc_selector_ptr restrict sel)
{
    qn_svc_entry_ptr ent = sel->entries[sel->next].entry;
    if ((sel->next += 1) == sel->count) {
        sel->next = 0;
    } // if
    return ent;
}

static qn_svc_sel_next_entry_fn qn_svc_sel_next_entry_functions[QN_SVC_SEL_STRATEGY_COUNT] = {
    &qn_svc_sel_next_entry_default,
    &qn_svc_sel_next_entry_default,
    &qn_svc_sel_next_entry_round_robin
};

QN_SDK qn_svc_entry_ptr qn_svc_sel_next_entry(qn_svc_selector_ptr restrict sel)
{
    assert(sel);
    return qn_svc_sel_next_entry_functions[sel->strategy](sel);
}

// ----

typedef void (*qn_svc_sel_register_failed_entry_fn)(qn_svc_selector_ptr restrict sel, unsigned int n);

static void qn_svc_sel_register_failed_entry_default(qn_svc_selector_ptr restrict sel, unsigned int n)
{
    return;
}

static void qn_svc_sel_register_failed_entry_last_succeeded_first(qn_svc_selector_ptr restrict sel, unsigned int n)
{
    if ((sel->next += 1) == sel->count) {
        sel->next = 0;
    } // if
}

static void qn_svc_sel_register_failed_entry_least_failures_first(qn_svc_selector_ptr restrict sel, unsigned int n)
{
    unsigned int p = 0;
    qn_svc_selector_entry_st ent;

    for (p = n + 1; p < sel->count; p += 1) {
        if (sel->entries[n].failures < sel->entries[p].failures) {
            break;
        } // if
    } // while
    if (p - (n + 1) == 0) return;

    ent = sel->entries[n];
    memcpy(&sel->entries[n], &sel->entries[n + 1], sizeof(qn_svc_selector_entry_st) * (p - (n + 1)));
    sel->entries[p - 1] = ent;
}

static qn_svc_sel_register_failed_entry_fn qn_svc_sel_register_failed_entry_functions[QN_SVC_SEL_STRATEGY_COUNT] = {
    &qn_svc_sel_register_failed_entry_last_succeeded_first,
    &qn_svc_sel_register_failed_entry_least_failures_first,
    &qn_svc_sel_register_failed_entry_default
};

QN_SDK void qn_svc_sel_register_failed_entry(qn_svc_selector_ptr restrict sel, qn_svc_entry_ptr restrict ent)
{
    qn_bool is_wrapping = qn_false;
    unsigned int i = 0;
    unsigned int k = 0;

    assert(sel);
    assert(ent);

    if (sel->count == 1) return;
    for (i = 0; i < sel->count; i += 1) {
        if (sel->entries[i].entry == ent) {
            is_wrapping = (sel->entries[i].failures += 1) == 0xFFFF;
            qn_svc_sel_register_failed_entry_functions[sel->strategy](sel, i);
            if (is_wrapping) {
                for (k = 0; k < sel->count; k += 1) sel->entries[k].failures >>= 1;
            } // if
            return;
        } // if
    } // for
}

#ifdef __cplusplus
}
#endif
