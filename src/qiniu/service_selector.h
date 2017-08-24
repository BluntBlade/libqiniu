#ifndef __QN_SERVICE_SELECTOR_H__
#define __QN_SERVICE_SELECTOR_H__

#include "qiniu/service.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Service Selector ----

struct _QN_SVC_SELECTOR;
typedef struct _QN_SVC_SELECTOR * qn_svc_selector_ptr;

typedef enum
{
    QN_SVC_SEL_LAST_SUCCEEDED_FIRST = 0,
    QN_SVC_SEL_LEAST_FAILURES_FIRST,
    QN_SVC_SEL_ROUND_ROBIN,
    QN_SVC_SEL_STRATEGY_COUNT
} qn_svc_selector_strategy;

typedef enum
{
    QN_SVC_SEL_ANY = 0,
    QN_SVC_SEL_NO_HTTP = 0x1,
    QN_SVC_SEL_NO_HTTPS = 0x2
} qn_svc_selector_filter;

QN_SDK extern qn_svc_selector_ptr qn_svc_sel_create(qn_service_ptr restrict svc, qn_svc_selector_strategy strategy, qn_svc_selector_filter filter);
QN_SDK extern void qn_svc_sel_destroy(qn_svc_selector_ptr restrict sel);
QN_SDK extern void qn_svc_sel_reset(qn_svc_selector_ptr restrict sel);

QN_SDK extern qn_svc_entry_ptr qn_svc_sel_next_entry(qn_svc_selector_ptr restrict sel);
QN_SDK extern void qn_svc_sel_register_failed_entry(qn_svc_selector_ptr restrict sel, qn_svc_entry_ptr restrict ent);

#ifdef __cplusplus
}
#endif

#endif // __QN_SERVICE_SELECTOR_H__
