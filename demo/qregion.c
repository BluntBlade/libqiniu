#include <stdio.h>
#include <time.h>
#include "qiniu/region.h"

int main(int argc, char * argv[])
{
    const char * access_key = NULL;
    const char * bucket = NULL;
    const char * base_url = NULL;
    const char * hostname = NULL;
    int i = 0;
    qn_integer ttl = 0;
    qn_rgn_service_ptr rgn_svc = NULL;
    qn_region_ptr rgn = NULL;
    qn_service_ptr svc = NULL;
    qn_svc_entry_ptr ent = NULL;
    qn_rgn_auth_st rgn_auth;

    if (argc < 3) {
        printf("Usage: qregion <ACCESS_KEY> <BUCKET>\n");
        return 0;
    } // if

    access_key = argv[1];
    bucket = argv[2];

    memset(&rgn_auth, 0, sizeof(rgn_auth));
    rgn_auth.v1.access_key = access_key;

    rgn_svc = qn_rgn_svc_create();
    if (!rgn_svc) {
        printf("Cannot initialize a new region service object.\n");
        return 1;
    } // if

    if (! (rgn = qn_rgn_svc_grab_entry_info(rgn_svc, &rgn_auth, bucket, &ttl))) {
        qn_rgn_svc_destroy(rgn_svc);
        printf("Cannot grab the region info for the `%s` bucket\n", bucket);
        return 2;
    } // if

    svc = qn_rgn_get_service(rgn, QN_SVC_UP);
    for (i = 0; i < qn_svc_entry_count(svc); i += 1) {
        ent = qn_svc_get_entry(svc, i);
        base_url = qn_str_cstr(ent->base_url);
        hostname = qn_str_cstr(ent->hostname);
        printf("host=[up] base_url:[%s] hostname:[%s] ttl:[%lu]\n", base_url, hostname ? hostname : "", ttl);
    } // for

    svc = qn_rgn_get_service(rgn, QN_SVC_IO);
    for (i = 0; i < qn_svc_entry_count(svc); i += 1) {
        ent = qn_svc_get_entry(svc, i);
        base_url = qn_str_cstr(ent->base_url);
        hostname = qn_str_cstr(ent->hostname);
        printf("host=[io] base_url:[%s] hostname:[%s] ttl:[%lu]\n", base_url, hostname ? hostname : "", ttl);
    } // for

    qn_rgn_destroy(rgn);
    qn_rgn_svc_destroy(rgn_svc);
    return 0;
}
