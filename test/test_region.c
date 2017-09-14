#include <CUnit/Basic.h>

#include "qiniu/region.h"

void test_create(void)
{
    qn_region_ptr rgn = NULL;

    rgn = qn_rgn_create();
    CU_ASSERT_PTR_NOT_NULL(rgn);

    qn_rgn_destroy(rgn);
}

void test_get_init_service(void)
{
    qn_region_ptr rgn = NULL;
    qn_service_ptr svc_ret = (qn_service_ptr)(~0);

    rgn = qn_rgn_create();
    if (! rgn) CU_FAIL_FATAL("Cannot create a new region");

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_UP);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_IO);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_RS);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_RSF);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_API);
    CU_ASSERT_PTR_NULL(svc_ret);

    qn_rgn_destroy(rgn);
}

void test_set_service(void)
{
    qn_region_ptr rgn = NULL;
    qn_service_ptr svc = NULL;
    qn_service_ptr svc_ret = NULL;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service");

    rgn = qn_rgn_create();
    if (! rgn) CU_FAIL_FATAL("Cannot create a new region");

    qn_rgn_set_service(rgn, QN_SVC_UP, svc);
    svc_ret = qn_rgn_get_service(rgn, QN_SVC_UP);
    CU_ASSERT_PTR_NOT_NULL(svc_ret);
    CU_ASSERT_PTR_EQUAL(svc_ret, svc);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_IO);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_RS);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_RSF);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn, QN_SVC_API);
    CU_ASSERT_PTR_NULL(svc_ret);

    qn_rgn_set_service(rgn, QN_SVC_UP, NULL);
    svc_ret = qn_rgn_get_service(rgn, QN_SVC_UP);
    CU_ASSERT_PTR_NULL(svc_ret);

    qn_rgn_destroy(rgn);
}

void test_duplicate(void)
{
    qn_region_ptr rgn = NULL;
    qn_region_ptr rgn2 = NULL;
    qn_service_ptr svc = NULL;
    qn_service_ptr svc_ret = NULL;

    svc = qn_svc_create(QN_SVC_UP);
    if (! svc) CU_FAIL_FATAL("Cannot create a new service");

    rgn = qn_rgn_create();
    if (! rgn) CU_FAIL_FATAL("Cannot create a new region");

    qn_rgn_set_service(rgn, QN_SVC_UP, svc);

    rgn2 = qn_rgn_duplicate(rgn);
    qn_rgn_destroy(rgn);
    CU_ASSERT_PTR_NOT_NULL(rgn2);

    svc_ret = qn_rgn_get_service(rgn2, QN_SVC_UP);
    CU_ASSERT_PTR_NOT_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn2, QN_SVC_IO);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn2, QN_SVC_RS);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn2, QN_SVC_RSF);
    CU_ASSERT_PTR_NULL(svc_ret);

    svc_ret = qn_rgn_get_service(rgn2, QN_SVC_API);
    CU_ASSERT_PTR_NULL(svc_ret);

    qn_rgn_destroy(rgn2);
}

CU_TestInfo test_normal_cases[] = {
    {"test_create", test_create},
    {"test_get_init_service", test_get_init_service},
    {"test_set_service", test_set_service},
    {"test_duplicate", test_duplicate},
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

    pSuite = CU_add_suite("Suite_Test_Region", NULL, NULL);
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
