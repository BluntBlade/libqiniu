#include <CUnit/Basic.h>

#include "qiniu/service.h"
#include "qiniu/service_selector.h"

void test_create(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    sel = qn_svc_sel_create(svc, QN_SVC_SEL_LAST_SUCCEEDED_FIRST, QN_SVC_SEL_ANY);
    CU_ASSERT_PTR_NOT_NULL(sel);

    qn_svc_sel_destroy(sel);
}

void test_last_succeed_first(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add third one
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    sel = qn_svc_sel_create(svc, QN_SVC_SEL_LAST_SUCCEEDED_FIRST, QN_SVC_SEL_ANY);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    // Check first
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check second
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    // Check third
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped first
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped second
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    qn_svc_sel_destroy(sel);
}

void test_least_failures_first(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add third one
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    sel = qn_svc_sel_create(svc, QN_SVC_SEL_LEAST_FAILURES_FIRST, QN_SVC_SEL_ANY);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    // Check first
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check second
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    // Check third
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped first
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped second
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    qn_svc_sel_register_failed_entry(sel, ent_ptr);
    qn_svc_sel_register_failed_entry(sel, ent_ptr);
    qn_svc_sel_register_failed_entry(sel, ent_ptr);
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    // Check wrapped third
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped fourth
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped fifth
    qn_svc_sel_register_failed_entry(sel, ent_ptr);
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    qn_svc_sel_destroy(sel);
}

void test_round_robin(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add third one
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    sel = qn_svc_sel_create(svc, QN_SVC_SEL_ROUND_ROBIN, QN_SVC_SEL_ANY);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    // Check first
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check second
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    // Check third
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped first
    qn_svc_sel_register_failed_entry(sel, ent_ptr);
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check wrapped second
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    // Check wrapped third
    qn_svc_sel_register_failed_entry(sel, ent_ptr);

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    qn_svc_sel_destroy(sel);
}

void test_no_https(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add third one
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    sel = qn_svc_sel_create(svc, QN_SVC_SEL_ROUND_ROBIN, QN_SVC_SEL_NO_HTTPS);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    // Check first
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Check second
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    qn_svc_sel_destroy(sel);
}

void test_no_http(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add third one
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    sel = qn_svc_sel_create(svc, QN_SVC_SEL_ROUND_ROBIN, QN_SVC_SEL_NO_HTTP);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    // Check first
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    // Check wrapped first
    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    qn_svc_sel_destroy(sel);
}

void test_no_entries(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_svc_selector_ptr sel = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;
    qn_svc_entry_st ent;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service object");

    // Add first one
    ent.base_url = qn_cs_duplicate("http://up.fake.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Add third one
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new test entry");

    // Check first
    sel = qn_svc_sel_create(svc, QN_SVC_SEL_LAST_SUCCEEDED_FIRST, QN_SVC_SEL_NO_HTTPS | QN_SVC_SEL_NO_HTTP);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NULL(ent_ptr);

    qn_svc_sel_destroy(sel);

    // Check second
    sel = qn_svc_sel_create(svc, QN_SVC_SEL_LEAST_FAILURES_FIRST, QN_SVC_SEL_NO_HTTPS | QN_SVC_SEL_NO_HTTP);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NULL(ent_ptr);

    qn_svc_sel_destroy(sel);

    // Check third
    sel = qn_svc_sel_create(svc, QN_SVC_SEL_ROUND_ROBIN, QN_SVC_SEL_NO_HTTPS | QN_SVC_SEL_NO_HTTP);
    if (! sel) CU_FAIL_FATAL("Cannot create a new selector");

    ent_ptr = qn_svc_sel_next_entry(sel);
    CU_ASSERT_PTR_NULL(ent_ptr);

    qn_svc_sel_destroy(sel);
}

CU_TestInfo test_normal_cases[] = {
    {"test_create", test_create},
    {"test_last_succeed_first", test_last_succeed_first},
    {"test_least_failures_first", test_least_failures_first},
    {"test_round_robin", test_round_robin},
    {"test_no_https", test_no_https},
    {"test_no_http", test_no_http},
    {"test_no_entries", test_no_entries},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo suites[] = {
    {"test_normal_cases", NULL, NULL, test_normal_cases},
    CU_SUITE_INFO_NULL
};

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    } // if

    pSuite = CU_add_suite("Suite_Test_Service_Selector", NULL, NULL);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    } // if

    if (CU_register_suites(suites) != CUE_SUCCESS) {
        printf("Cannot register test suites.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
} // main
