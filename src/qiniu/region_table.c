#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/errors.h"
#include "qiniu/os/time.h"
#include "qiniu/region_table.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of Region Table ----

typedef struct _QN_RGN_ENTRY
{
    qn_string name;
    qn_time deadline;
    qn_region_ptr rgn;
} qn_rgn_entry_st;

typedef struct _QN_RGN_TABLE
{
    qn_rgn_entry_st * entries;
    unsigned int cnt:16;
    unsigned int cap:16;
} qn_rgn_table_st;

QN_SDK qn_rgn_table_ptr qn_rgn_tbl_create(void)
{
    qn_rgn_table_ptr new_rtbl = calloc(1, sizeof(qn_rgn_table_st));
    if (! new_rtbl) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_rtbl->cap = 4;
    new_rtbl->entries = calloc(new_rtbl->cap, sizeof(qn_rgn_entry_st));
    if (! new_rtbl->entries) {
        free(new_rtbl);
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_rtbl;
}

QN_SDK void qn_rgn_tbl_destroy(qn_rgn_table_ptr restrict rtbl)
{
   if (rtbl) { 
        qn_rgn_tbl_reset(rtbl);
        free(rtbl->entries);
        free(rtbl);
    } // if
}

QN_SDK void qn_rgn_tbl_reset(qn_rgn_table_ptr restrict rtbl)
{
    while (rtbl->cnt-- > 0) {
        qn_rgn_destroy(rtbl->entries[rtbl->cnt].rgn);
        qn_str_destroy(rtbl->entries[rtbl->cnt].name);
    } // while
}

// ----

static qn_bool qn_rgn_tbl_augment(qn_rgn_table_ptr restrict rtbl)
{
    unsigned int new_cap = rtbl->cap + (rtbl->cap >> 1); // 1.5 times.
    qn_rgn_entry_st * new_entries = calloc(new_cap, sizeof(qn_rgn_entry_st));
    if (! new_entries) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_entries, rtbl->entries, rtbl->cnt * sizeof(qn_rgn_entry_st));

    free(rtbl->entries);
    rtbl->entries = new_entries;
    rtbl->cap = new_cap;
    return qn_true;
}

QN_SDK const qn_region_ptr qn_rgn_tbl_get_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name)
{
    unsigned int i = 0;

    for (i = 0; i < rtbl->cnt; i += 1) {
        if (qn_str_compare_raw(rtbl->entries[i].name, name) == 0) {
            if (qn_tm_time() > rtbl->entries[i].deadline) {
                qn_err_rgn_set_entry_info_expired();
                return NULL;
            } // if
            return rtbl->entries[i].rgn;
        } // if
    } // for
    qn_err_rgn_set_lack_of_entry_info();
    return NULL;
}

QN_SDK qn_bool qn_rgn_tbl_set_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name, qn_integer deadline, const qn_region_ptr restrict rgn)
{
    unsigned int i = 0;
    qn_time now = qn_tm_time();

    for (i = 0; i < rtbl->cnt; i += 1) {
        if (qn_str_compare_raw(rtbl->entries[i].name, name) == 0) {
            qn_rgn_destroy(rtbl->entries[i].rgn);

            rtbl->entries[i].deadline = now + deadline;
            rtbl->entries[i].rgn = rgn;
            return qn_true;
        } // if
    } // for

    if ((i == rtbl->cap) && ! qn_rgn_tbl_augment(rtbl)) return qn_false;
    if (! (rtbl->entries[i].name = qn_cs_duplicate(name))) return qn_false;

    rtbl->entries[i].deadline = now + deadline;
    rtbl->entries[i].rgn = rgn;

    rtbl->cnt += 1;
    return qn_true;
}

// ---- Definition of Region Interator ----

typedef struct _QN_RGN_ITERATOR
{
    qn_rgn_table_ptr rtbl;
    unsigned int pos;
} qn_rgn_iterator;

QN_SDK qn_rgn_iterator_ptr qn_rgn_itr_create(qn_rgn_table_ptr restrict rtbl)
{
    qn_rgn_iterator_ptr new_itr = calloc(1, sizeof(qn_rgn_iterator));
    if (!new_itr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_itr->rtbl = rtbl;
    new_itr->pos = 0;
    return new_itr;
}

QN_SDK void qn_rgn_itr_destroy(qn_rgn_iterator_ptr restrict itr)
{
    if (itr) {
        free(itr);
    } // if
}

QN_SDK void qn_rgn_itr_rewind(qn_rgn_iterator_ptr restrict itr)
{
    itr->pos = 0;
}

QN_SDK qn_bool qn_rgn_itr_next_pair(qn_rgn_iterator_ptr restrict itr, qn_region_ptr * restrict rgn)
{
    if (itr->pos == itr->rtbl->cnt) return qn_false;
    *rgn = itr->rtbl->entries[itr->pos].rgn;
    itr->pos += 1;
    return qn_true;
}

#ifdef __cplusplus
}
#endif
