#include <stdio.h>
#include <time.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/region.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    const char * bucket;
    const char * key;
    const char * fname;
    qn_integer ttl;
    qn_mac_ptr mac;
    qn_string uptoken;
    qn_string hdr_ent;
    qn_string put_ret_str;
    qn_json_object_ptr put_policy;
    qn_json_object_ptr put_ret;
    qn_storage_ptr stor;
    qn_stor_upload_extra_ptr upe;
    qn_region_ptr rgn;
    qn_rgn_auth_st rgn_auth;
    qn_rgn_service_ptr rgn_svc;
    qn_service_ptr svc;
    qn_svc_entry_ptr ent;
    qn_http_hdr_iterator_ptr hdr_itr;

    if (argc < 5) {
        printf("Demo qputf - Put single file in one HTTP session.\n");
        printf("Usage: qputf <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME> [MSG_QUEUE MSG_BODY MSG_MIME_TYPE]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    put_policy = qn_stor_pp_create(bucket, key, time(NULL) + 3600);
    if (! put_policy) {
        qn_mac_destroy(mac);
        printf("Cannot create a put policy due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    if (argc > 6) {
        if (! qn_stor_pp_upload_message(put_policy, argv[6], argv[7], argv[8])) {
            qn_json_obj_destroy(put_policy);
            qn_mac_destroy(mac);
            printf("Cannot fill the message informaiton into queue fields due to application error `%s`.\n", qn_err_get_message());
            return 1;
        } // if
    } // if

    uptoken = qn_stor_pp_to_uptoken(put_policy, mac);
    qn_json_obj_destroy(put_policy);
    qn_mac_destroy(mac);
    if (! uptoken) {
        printf("Cannot make an uptoken due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    memset(&rgn_auth, 0, sizeof(rgn_auth));
    rgn_auth.v1.access_key = argv[1];

    rgn_svc = qn_rgn_svc_create();
    if (!rgn_svc) {
        qn_str_destroy(uptoken);
        printf("Cannot create a region service object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    rgn = qn_rgn_svc_grab_entry_info(rgn_svc, &rgn_auth, bucket, &ttl);
    qn_rgn_svc_destroy(rgn_svc);
    if (! rgn) {
        qn_rgn_destroy(rgn);
        qn_str_destroy(uptoken);
        printf("Cannot fetch region information of the bucket `%s` due to application error `%s`.\n", bucket, qn_err_get_message());
        return 1;
    } // if

    upe = qn_stor_upe_create();
    if (! upe) {
        qn_rgn_destroy(rgn);
        qn_str_destroy(uptoken);
        printf("Cannot create a put extra due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    svc = qn_rgn_get_service(rgn, QN_SVC_UP);
    if (! svc) svc = qn_svc_get_default_service(QN_SVC_UP);
    ent = qn_svc_get_entry(svc, 0);

    qn_stor_upe_set_final_key(upe, key);
    qn_stor_upe_set_service_entry(upe, ent);

    stor = qn_stor_create();
    if (! stor) {
        qn_stor_upe_destroy(upe);
        qn_rgn_destroy(rgn);
        qn_str_destroy(uptoken);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    put_ret = qn_stor_up_api_upload_file(stor, uptoken, fname, upe);
    qn_stor_upe_destroy(upe);
    qn_rgn_destroy(rgn);
    qn_str_destroy(uptoken);

    if (! put_ret) {
        qn_stor_destroy(stor);
        printf("Cannot put the file `%s` to `%s:%s` due to application error `%s`.\n", fname, bucket, key, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
    qn_http_hdr_itr_destroy(hdr_itr);

    put_ret_str = qn_json_object_to_string(put_ret);
    qn_stor_destroy(stor);
    if (! put_ret_str) {
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", put_ret_str);
    qn_str_destroy(put_ret_str);

    return 0;
}
