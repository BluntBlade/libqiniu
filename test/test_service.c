#include <CUnit/Basic.h>

#include "qiniu/service.h"

void test_create(void)
{
    qn_service_ptr svc = NULL;

    svc = qn_svc_create(QN_SVC_UP);
    CU_ASSERT_PTR_NOT_NULL(svc);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc), 0);

    qn_svc_destroy(svc);
}

void test_add_entry(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
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
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc), 1);

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc), 2);

    // Check result - part 1
    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc, 1);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->hostname);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    // Add repeat ones
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc), 3);

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc), 4);

    // Check result - part 2
    ent_ptr = qn_svc_get_entry(svc, 2);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc, 3);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    // Add fifth one
    ent.base_url = qn_cs_duplicate("http://up.augment.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc), 5);

    // Check result - part 3
    ent_ptr = qn_svc_get_entry(svc, 4);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.augment.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc, 3);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    qn_svc_destroy(svc);
}

void test_duplicate(void)
{
    qn_bool ret = qn_false;
    qn_service_ptr svc = NULL;
    qn_service_ptr svc_dup = NULL;
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
    if (! ret) CU_FAIL_FATAL("Cannot add a new entry");

    // Add second one
    ent.base_url = qn_cs_duplicate("https://127.0.0.1");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = qn_cs_duplicate("upload.fake.com");
    if (! ent.hostname) CU_FAIL_FATAL("Cannot create a test string");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    qn_str_destroy(ent.hostname);
    if (! ret) CU_FAIL_FATAL("Cannot add a new entry");

    // Add repeat ones
    ent.base_url = qn_cs_duplicate("http://up.repeat.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    if (! ret) CU_FAIL_FATAL("Cannot add a new entry");

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new entry");

    // Add fifth one
    ent.base_url = qn_cs_duplicate("http://up.augment.com");
    if (! ent.base_url) CU_FAIL_FATAL("Cannot create a test string");

    ent.hostname = NULL;

    ret = qn_svc_add_entry(svc, &ent);
    qn_str_destroy(ent.base_url);
    if (! ret) CU_FAIL_FATAL("Cannot add a new entry");

    // Do duplication
    svc_dup = qn_svc_duplicate(svc);
    CU_ASSERT_PTR_NOT_NULL(svc_dup);
    CU_ASSERT_EQUAL(qn_svc_entry_count(svc_dup), 5);

    // Check result
    ent_ptr = qn_svc_get_entry(svc_dup, 0);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.fake.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc_dup, 1);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "https://127.0.0.1"), 0);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->hostname);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->hostname, "upload.fake.com"), 0);

    ent_ptr = qn_svc_get_entry(svc_dup, 2);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc_dup, 3);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.repeat.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    ent_ptr = qn_svc_get_entry(svc_dup, 4);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr);
    CU_ASSERT_PTR_NOT_NULL(ent_ptr->base_url);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.augment.com"), 0);
    CU_ASSERT_PTR_NULL(ent_ptr->hostname);

    qn_svc_destroy(svc_dup);
    qn_svc_destroy(svc);
}

void test_get_default_service(void)
{
    qn_service_ptr svc = NULL;
    qn_svc_entry_ptr ent_ptr = NULL;

    // Check UP
    svc = qn_svc_get_default_service(QN_SVC_UP);
    CU_ASSERT_PTR_NOT_NULL(svc);
    CU_ASSERT_TRUE(qn_svc_entry_count(svc) > 0);

    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://up.qiniu.com"), 0);

    // Check IO
    svc = qn_svc_get_default_service(QN_SVC_IO);
    CU_ASSERT_PTR_NOT_NULL(svc);
    CU_ASSERT_TRUE(qn_svc_entry_count(svc) > 0);

    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://iovip.qbox.me"), 0);

    // Check RS
    svc = qn_svc_get_default_service(QN_SVC_RS);
    CU_ASSERT_PTR_NOT_NULL(svc);
    CU_ASSERT_TRUE(qn_svc_entry_count(svc) > 0);

    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://rs.qiniu.com"), 0);

    // Check RSF
    svc = qn_svc_get_default_service(QN_SVC_RSF);
    CU_ASSERT_PTR_NOT_NULL(svc);
    CU_ASSERT_TRUE(qn_svc_entry_count(svc) > 0);

    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://rsf.qbox.me"), 0);

    // Check API
    svc = qn_svc_get_default_service(QN_SVC_API);
    CU_ASSERT_PTR_NOT_NULL(svc);
    CU_ASSERT_TRUE(qn_svc_entry_count(svc) > 0);

    ent_ptr = qn_svc_get_entry(svc, 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(ent_ptr->base_url, "http://api.qiniu.com"), 0);
}

CU_TestInfo test_normal_cases[] = {
    {"test_create", test_create},
    {"test_add_entry", test_add_entry},
    {"test_duplicate", test_duplicate},
    {"test_get_default_service", test_get_default_service},
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

    pSuite = CU_add_suite("Suite_Test_Service", NULL, NULL);
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
