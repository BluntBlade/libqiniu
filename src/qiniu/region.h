#ifndef __QN_REGION_H__
#define __QN_REGION_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"
#include "qiniu/service.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Region ----

struct _QN_REGION;
typedef struct _QN_REGION * qn_region_ptr;

QN_SDK extern qn_region_ptr qn_rgn_create(void);
QN_SDK extern void qn_rgn_destroy(qn_region_ptr restrict rgn);
QN_SDK extern void qn_rgn_reset(qn_region_ptr restrict rgn);

QN_SDK extern qn_service_ptr qn_rgn_get_service(qn_region_ptr restrict rgn, qn_svc_type type);

QN_SDK extern qn_region_ptr qn_rgn_duplicate(qn_region_ptr restrict rgn);

QN_SDK extern void qn_rgn_set_service(qn_region_ptr restrict rgn, qn_svc_type type, qn_service_ptr restrict svc);

// ---- Declaration of Region Service ----

typedef struct _QN_RGN_AUTH
{
    struct {
        const char * access_key;
    } v1;
} qn_rgn_auth_st, *qn_rgn_auth_ptr;

struct _QN_RGN_SERVICE;
typedef struct _QN_RGN_SERVICE * qn_rgn_service_ptr;

QN_SDK extern qn_rgn_service_ptr qn_rgn_svc_create(void);
QN_SDK extern void qn_rgn_svc_destroy(qn_rgn_service_ptr restrict svc);

QN_SDK extern qn_region_ptr qn_rgn_svc_grab_entry_info(qn_rgn_service_ptr restrict svc, qn_rgn_auth_ptr restrict auth, const char * restrict bucket, qn_integer * restrict ttl);

#ifdef __cplusplus
}
#endif

#endif // __QN_REGION_H__

