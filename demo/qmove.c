#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/base/errors.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string src_bucket;
    qn_string src_key;
    qn_string dest_bucket;
    qn_string dest_key;
    qn_string move_ret_str;
    qn_json_object_ptr move_ret;
    qn_storage_ptr stor;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 7) {
        printf("Usage: qmove <ACCESS_KEY> <SECRET_KEY> <SRC_BUCKET> <SRC_KEY> <DEST_BUCKET> <DEST_BUCKET>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    src_bucket = argv[3];
    src_key = argv[4];
    dest_bucket = argv[5];
    dest_key = argv[6];

    stor = qn_stor_create();
    if (! stor) {
        qn_mac_destroy(mac);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    move_ret = qn_stor_mn_api_move(stor, mac, src_bucket, src_key, dest_bucket, dest_key, NULL);
    qn_mac_destroy(mac);
    if (! move_ret) {
        qn_stor_destroy(stor);
        printf("Cannot move the `%s:%s` file to `%s:%s` due to application error `%s`.\n", src_bucket, src_key, dest_bucket, dest_bucket, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
    qn_http_hdr_itr_destroy(hdr_itr);

    move_ret_str = qn_json_object_to_string(move_ret);
    qn_stor_destroy(stor);

    if (! move_ret_str) {
        printf("Cannot format move result string due to application error `%s`\n", qn_err_get_message());
        return 1;
    } // if

    printf("%s\n", move_ret_str);
    qn_str_destroy(move_ret_str);
    
    return 0;
}
