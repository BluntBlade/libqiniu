#ifndef __QN_REGION_TABLE_H__
#define __QN_REGION_TABLE_H__

#include "qiniu/os/types.h"
#include "qiniu/region.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Region Table ----

struct _QN_RGN_TABLE;
typedef struct _QN_RGN_TABLE * qn_rgn_table_ptr;

QN_SDK extern qn_rgn_table_ptr qn_rgn_tbl_create(void);
QN_SDK extern void qn_rgn_tbl_destroy(qn_rgn_table_ptr restrict rtbl);
QN_SDK extern void qn_rgn_tbl_reset(qn_rgn_table_ptr restrict rtbl);

QN_SDK extern const qn_region_ptr qn_rgn_tbl_get_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name);
QN_SDK extern qn_bool qn_rgn_tbl_set_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name, qn_integer deadline, const qn_region_ptr restrict rgn);

// ---- Declaration of Region Interator ----

struct _QN_RGN_ITERATOR;
typedef struct _QN_RGN_ITERATOR * qn_rgn_iterator_ptr;

QN_SDK extern qn_rgn_iterator_ptr qn_rgn_itr_create(qn_rgn_table_ptr restrict rtbl);
QN_SDK extern void qn_rgn_itr_destroy(qn_rgn_iterator_ptr restrict itr);
QN_SDK extern void qn_rgn_itr_rewind(qn_rgn_iterator_ptr restrict itr);

QN_SDK extern qn_bool qn_rgn_itr_next_pair(qn_rgn_iterator_ptr restrict itr, qn_region_ptr * restrict rgn);

#ifdef __cplusplus
}
#endif

#endif // __QN_REGION_TABLE_H__
