#ifndef __QN_RPC_H__
#define __QN_RPC_H__

#include "qiniu/base/json.h"
#include "qiniu/http.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ---- Declaration of Remote Procedure Caller (Abbreviation: rpc) ---- */

struct _QN_RPC;
typedef struct _QN_RPC * qn_rpc_ptr;

/* -- Constructor & Destructor methods -- */

QN_SDK extern qn_rpc_ptr qn_rpc_create(void);
QN_SDK extern void qn_rpc_destroy(qn_rpc_ptr restrict rpc);

/* -- Setting methods -- */

QN_SDK extern qn_bool qn_rpc_set_header(qn_rpc_ptr restrict rpc, const char * restrict hdr, const char * restrict val);
QN_SDK extern void qn_rpc_unset_header(qn_rpc_ptr restrict rpc, const char * restrict hdr);

/* -- Call methods -- */

QN_SDK extern qn_json_object_ptr qn_rpc_post(qn_rpc_ptr restrict rpc, const char * restrict url, qn_io_reader_itf restrict rdr, qn_fsize size);

QN_SDK extern qn_json_object_ptr qn_rpc_post_form(qn_rpc_ptr restrict rpc, const char * restrict url, qn_http_form_ptr restrict form);
/* QN_SDK extern qn_json_object_ptr qn_rpc_post_buffer(qn_rpc_ptr restrict rpc, const char * restrict url, const char * restrict buf, qn_size buf_size); */

QN_SDK extern qn_json_object_ptr qn_rpc_get(qn_rpc_ptr restrict rpc, const char * restrict url);

#ifdef __cplusplus
}
#endif

#endif /* __QN_RPC_H__ */
