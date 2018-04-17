#include <CUnit/Basic.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/json.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/json_formatter.h"

#define CU_ASSERT_LONG_DOUBLE_EQUAL(actual, expected, granularity) \
  { CU_assertImplementation(((fabsl((long double)(actual) - (expected)) <= fabsl((long double)(granularity)))), __LINE__, ("CU_ASSERT_LONG_DOUBLE_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE); }

// ---- test functions ----

void test_manipulate_object(void)
{
    qn_json_integer n = 0;
    qn_json_number num_val = 0.0L;
    qn_json_object_ptr obj_root = NULL;
    qn_string str = NULL;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    // set a string element
    CU_ASSERT_TRUE(qn_json_obj_set_text(obj_root, "_str", buf, buf_len));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 1);

    // set a number element
    CU_ASSERT_TRUE(qn_json_obj_set_number(obj_root, "_num", -9.99L));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 2);

    // set a integer element
    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj_root, "_int", 256));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 3);

    // set a null element
    CU_ASSERT_TRUE(qn_json_obj_set_null(obj_root, "_null"));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 4);

    // set a boolean element
    CU_ASSERT_TRUE(qn_json_obj_set_boolean(obj_root, "_false", qn_false));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 5);

    // check the null element
    qn_json_obj_unset(obj_root, "_null");
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 4);

    // check the integer element
    n = 0;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int", &n));
    CU_ASSERT_EQUAL(n, 256);

    qn_json_obj_unset(obj_root, "_int");
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 3);

    // check the number element
    num_val = 0.0L;
    CU_ASSERT_TRUE(qn_json_obj_get_number(obj_root, "_num", &num_val));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 3);

    // check the string element
    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 3);

    qn_json_obj_unset(obj_root, "_false");
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 2);

    qn_json_obj_unset(obj_root, "_str");
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 1);

    qn_json_obj_unset(obj_root, "_num");
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 0);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_alone_field_1_new_key_greater_than_old_key(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "A", "A line for testing.") ) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "A", "B"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "A", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "B", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_alone_field_2_new_key_less_than_old_key(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "Z", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "Z", "Y"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "Z", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Y", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_alone_field_3_new_key_equals_to_old_key(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "X", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "X", "X"));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "X", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_1(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "A", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "B", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "C", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "C", "D"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "C", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "D", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "B", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "A", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_2(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "A", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "B", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "C", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "B", "D"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "B", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "D", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "C", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "A", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_3(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "A", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "B", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "C", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "A", "D"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "A", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "D", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "C", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "B", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_2_new_key_less_than_old_key_1(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "X", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Y", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Z", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "X", "W"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "X", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "W", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Y", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Z", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_2_new_key_less_than_old_key_2(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "X", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Y", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Z", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "Y", "W"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "Y", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "W", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "X", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Z", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_2_new_key_less_than_old_key_3(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "X", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Y", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Z", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "Z", "W"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "Z", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "W", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "X", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Y", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_3_new_key_equals_to_old_key(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "X", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Y", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Z", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "Y", "Y"));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "X", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Y", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Z", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_rename_accompanied_field_4_new_key_replace_old_key_in_place(void)
{
    qn_string str;
    qn_json_object_ptr obj_root;

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_cstr(obj_root, "X", "A line for testing.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Y", "Both are ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    if (! qn_json_obj_set_cstr(obj_root, "Z", "Call it ready.")) CU_FAIL_FATAL("Cannot set a new string field");

    CU_ASSERT_TRUE(qn_json_obj_rename(obj_root, "Y", "YYYY"));

    str = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "Y", &str));
    CU_ASSERT_TRUE(qn_err_is_no_such_entry());
    CU_ASSERT_PTR_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "X", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "YYYY", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "Z", &str));
    CU_ASSERT_PTR_NOT_NULL(str);

    qn_json_obj_destroy(obj_root);
}

void test_obj_set()
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_object_ptr obj = NULL;
    qn_json_array_ptr arr = NULL;
    qn_json_object_ptr obj_ret = NULL;
    qn_json_array_ptr arr_ret = NULL;
    qn_string str_ret = NULL;
    qn_json_integer int_ret = 0;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    obj_root = qn_json_obj_create();
    CU_ASSERT_PTR_NOT_NULL(obj_root);

    // push a string element
    CU_ASSERT_TRUE(qn_json_obj_set_text(obj_root, "_str", buf, buf_len));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 1);

    // push a integer element
    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj_root, "_int", 123));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_root), 2);

    // ----

    obj = qn_json_obj_create();
    CU_ASSERT_PTR_NOT_NULL(obj);

    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj, "_int", 2));

    arr = qn_json_arr_create();
    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr, 789));

    // ----

    CU_ASSERT_TRUE(qn_json_obj_set_object(obj_root, "_str", obj));

    obj_ret = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_object(obj_root, "_str", &obj_ret));
    CU_ASSERT_PTR_NOT_NULL(obj_ret);
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_ret), 1);

    str_ret = NULL;
    CU_ASSERT_FALSE(qn_json_obj_get_string(obj_root, "_str", &str_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_PTR_NULL(str_ret);

    // ----

    CU_ASSERT_TRUE(qn_json_obj_set_array(obj_root, "_int", arr));

    arr_ret = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_array(obj_root, "_int", &arr_ret));
    CU_ASSERT_PTR_NOT_NULL(arr_ret);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_ret), 1);

    int_ret = -1;
    CU_ASSERT_FALSE(qn_json_obj_get_integer(obj_root, "_int", &int_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_EQUAL(int_ret, -1);

    // ----

    qn_json_obj_destroy(obj_root);
}

void test_manipulate_array(void)
{
    qn_bool bool_val;
    qn_json_integer int_val;
    qn_json_number num_val;
    qn_json_array_ptr arr_root = NULL;
    qn_string str = NULL;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    // unshift a string element
    CU_ASSERT_TRUE(qn_json_arr_unshift_text(arr_root, buf, buf_len));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 1);

    // push a number element
    CU_ASSERT_TRUE(qn_json_arr_push_number(arr_root, -9.99L));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 2);

    // push a integer element
    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr_root, 256));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 3);

    // unshift a null element
    CU_ASSERT_TRUE(qn_json_arr_unshift_null(arr_root));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 4);

    // push a boolean element
    CU_ASSERT_TRUE(qn_json_arr_push_boolean(arr_root, qn_false));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 5);

    // check the first element (null)
    qn_json_arr_shift(arr_root);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 4);

    // check the second element (string)
    str = NULL;
    CU_ASSERT_TRUE(qn_json_arr_get_string(arr_root, 0, &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);

    qn_json_arr_shift(arr_root);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 3);

    // check the last element (boolean == false)
    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_arr_get_boolean(arr_root, 2, &bool_val));
    CU_ASSERT_FALSE(bool_val);
    qn_json_arr_pop(arr_root);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 2);

    // check the last element (int == 256)
    int_val = 0;
    CU_ASSERT_TRUE(qn_json_arr_get_integer(arr_root, 1, &int_val));
    CU_ASSERT_TRUE(int_val != 0);

    int_val = 0;
    CU_ASSERT_TRUE(qn_json_arr_get_integer(arr_root, 1, &int_val));
    CU_ASSERT_TRUE(int_val == 256);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 2);

    // check the first element

    num_val = 0.0L;
    CU_ASSERT_TRUE(qn_json_arr_get_number(arr_root, 0, &num_val));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 2);

    qn_json_arr_destroy(arr_root);
}

void test_arr_replace()
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_object_ptr obj = NULL;
    qn_json_array_ptr arr = NULL;
    qn_json_object_ptr obj_ret = NULL;
    qn_json_array_ptr arr_ret = NULL;
    qn_bool bool_val = qn_false;
    qn_string str_ret = NULL;
    qn_json_integer int_ret = 0;
    qn_json_number num_ret = 0.0;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    arr_root = qn_json_arr_create();
    CU_ASSERT_PTR_NOT_NULL(arr_root);

    // push a string element
    CU_ASSERT_TRUE(qn_json_arr_push_text(arr_root, buf, buf_len));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 1);

    // push a integer element
    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr_root, 123));
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_root), 2);

    // ----

    obj = qn_json_obj_create();
    CU_ASSERT_PTR_NOT_NULL(obj);

    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj, "_int", 2));

    arr = qn_json_arr_create();
    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr, 789));

    // ----

    CU_ASSERT_TRUE(qn_json_arr_replace_object(arr_root, 0, obj));

    CU_ASSERT_TRUE(qn_json_arr_get_object(arr_root, 0, &obj_ret));
    CU_ASSERT_PTR_NOT_NULL(obj_ret);
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_ret), 1);

    str_ret = NULL;
    CU_ASSERT_FALSE(qn_json_arr_get_string(arr_root, 0, &str_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_PTR_NULL(str_ret);

    // ----

    CU_ASSERT_TRUE(qn_json_arr_replace_array(arr_root, 1, arr));

    arr_ret = NULL;
    CU_ASSERT_TRUE(qn_json_arr_get_array(arr_root, 1, &arr_ret));
    CU_ASSERT_PTR_NOT_NULL(arr_ret);
    CU_ASSERT_EQUAL(qn_json_arr_size(arr_ret), 1);

    int_ret = -1;
    CU_ASSERT_FALSE(qn_json_arr_get_integer(arr_root, 1, &int_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_EQUAL(int_ret, -1);

    // ----

    CU_ASSERT_TRUE(qn_json_arr_replace_integer(arr_root, 0, 456));

    int_ret = -1;
    CU_ASSERT_TRUE(qn_json_arr_get_integer(arr_root, 0, &int_ret));
    CU_ASSERT_EQUAL(int_ret, 456);

    obj_ret = NULL;
    CU_ASSERT_FALSE(qn_json_arr_get_object(arr_root, 0, &obj_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_PTR_NULL(obj_ret);

    // ----

    CU_ASSERT_TRUE(qn_json_arr_replace_number(arr_root, 1, -666.666L));

    num_ret = -1.0L;
    CU_ASSERT_TRUE(qn_json_arr_get_number(arr_root, 1, &num_ret));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_ret, -666.666L, 0.001L);

    arr_ret = NULL;
    CU_ASSERT_FALSE(qn_json_arr_get_array(arr_root, 1, &arr_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_PTR_NULL(arr_ret);

    // ----

    CU_ASSERT_TRUE(qn_json_arr_replace_boolean(arr_root, 0, qn_true));

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_arr_get_boolean(arr_root, 0, &bool_val));
    CU_ASSERT_TRUE(bool_val);

    int_ret = -1;
    CU_ASSERT_FALSE(qn_json_arr_get_integer(arr_root, 0, &int_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_EQUAL(int_ret, -1);

    // ----

    CU_ASSERT_TRUE(qn_json_arr_replace_null(arr_root, 1));

    num_ret = -1.0L;
    CU_ASSERT_FALSE(qn_json_arr_get_number(arr_root, 1, &num_ret));
    CU_ASSERT_TRUE(qn_err_json_is_not_this_type());
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_ret, -1L, 0.01L);

    // ----

    qn_json_arr_destroy(arr_root);
}

CU_TestInfo test_normal_cases_of_json_manipulating[] = {
    {"test_manipulate_object()", test_manipulate_object},
    {"test_obj_rename_alone_field_1_new_key_greater_than_old_key()", test_obj_rename_alone_field_1_new_key_greater_than_old_key},
    {"test_obj_rename_alone_field_2_new_key_less_than_old_key()", test_obj_rename_alone_field_2_new_key_less_than_old_key},
    {"test_obj_rename_alone_field_3_new_key_equals_to_old_key()", test_obj_rename_alone_field_3_new_key_equals_to_old_key},
    {"test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_1()", test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_1},
    {"test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_2()", test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_2},
    {"test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_3()", test_obj_rename_accompanied_field_1_new_key_greater_than_old_key_3},
    {"test_obj_rename_accompanied_field_2_new_key_less_than_old_key_1()", test_obj_rename_accompanied_field_2_new_key_less_than_old_key_1},
    {"test_obj_rename_accompanied_field_2_new_key_less_than_old_key_2()", test_obj_rename_accompanied_field_2_new_key_less_than_old_key_2},
    {"test_obj_rename_accompanied_field_2_new_key_less_than_old_key_3()", test_obj_rename_accompanied_field_2_new_key_less_than_old_key_3},
    {"test_obj_rename_accompanied_field_3_new_key_equals_to_old_key()", test_obj_rename_accompanied_field_3_new_key_equals_to_old_key},
    {"test_obj_rename_accompanied_field_4_new_key_replace_old_key_in_place()", test_obj_rename_accompanied_field_4_new_key_replace_old_key_in_place},
    {"test_obj_set()", test_obj_set},
    {"test_manipulate_array()", test_manipulate_array},
    {"test_arr_replace()", test_arr_replace},
    CU_TEST_INFO_NULL
};

/* ==== Normal test cases for iterating ==== */

void test_iterating_empty_object(void)
{
    qn_json_iterator_ptr itr = NULL;
    qn_json_object_ptr obj = NULL;

    if (! (obj = qn_json_obj_create())) CU_FAIL_FATAL("Cannot create the object for iterating");

    CU_ASSERT_PTR_NOT_NULL((itr = qn_json_itr_create())); 
    CU_ASSERT_TRUE(qn_json_itr_push_object(itr, obj, 0)); 
    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
    CU_ASSERT_EQUAL(qn_json_itr_get_type(itr), QN_JSON_UNKNOWN);

    qn_json_itr_destroy(itr);
    qn_json_obj_destroy(obj);
}

void test_iterating_object_with_one_element(void)
{
    qn_integer int_val = 0;
    qn_string key = NULL;
    qn_json_iterator_ptr itr = NULL;
    qn_json_object_ptr obj = NULL;

    if (! (obj = qn_json_obj_create())) CU_FAIL_FATAL("Cannot create the object for iterating");
    if (! qn_json_obj_set_integer(obj, "_int", 123)) CU_FAIL_FATAL("Cannot set the integer element for iterating");

    CU_ASSERT_PTR_NOT_NULL((itr = qn_json_itr_create())); 
    CU_ASSERT_TRUE(qn_json_itr_push_object(itr, obj, 0)); 

    CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));
    CU_ASSERT_EQUAL(qn_json_itr_get_type(itr), QN_JSON_INTEGER);

    int_val = 0;
    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, NULL, &int_val));
    CU_ASSERT_EQUAL(int_val, 123);

    key = NULL;
    int_val = 0;
    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_int"), 0);
    CU_ASSERT_EQUAL(int_val, 123);
    qn_json_itr_reclaim_string(itr, key);

    qn_json_itr_advance(itr);
    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
    CU_ASSERT_EQUAL(qn_json_itr_get_type(itr), QN_JSON_UNKNOWN);

    qn_json_itr_destroy(itr);
    qn_json_obj_destroy(obj);
}

void test_iterating_object_with_all_type_elements(void)
{
    qn_json_object_ptr obj_val = NULL;
    qn_json_array_ptr arr_val = NULL;
    qn_integer int_val = 0;
    qn_number num_val = 0;
    qn_bool bool_val = qn_false;
    qn_string str_val = NULL;
    qn_string key = NULL;
    qn_json_iterator_ptr itr = NULL;
    qn_json_object_ptr obj = NULL;

    if (! (obj = qn_json_obj_create())) CU_FAIL_FATAL("Cannot create the object for iterating");

    if (! qn_json_obj_set_null(obj, "_null")) CU_FAIL_FATAL("Cannot set the null element for iterating");
    if (! qn_json_obj_set_new_empty_array(obj, "_arr")) CU_FAIL_FATAL("Cannot set the array element for iterating");
    if (! qn_json_obj_set_number(obj, "_num", 789.789L)) CU_FAIL_FATAL("Cannot set the number element for iterating");
    if (! qn_json_obj_set_boolean(obj, "_bool", qn_true)) CU_FAIL_FATAL("Cannot set the boolean element for iterating");
    if (! qn_json_obj_set_integer(obj, "_int", 123)) CU_FAIL_FATAL("Cannot set the integer element for iterating");
    if (! qn_json_obj_set_cstr(obj, "_str", "test")) CU_FAIL_FATAL("Cannot set the string element for iterating");
    if (! qn_json_obj_set_new_empty_object(obj, "_obj")) CU_FAIL_FATAL("Cannot set the object element for iterating");

    CU_ASSERT_PTR_NOT_NULL((itr = qn_json_itr_create())); 
    CU_ASSERT_TRUE(qn_json_itr_push_object(itr, obj, 0)); 

    arr_val = NULL;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_array(itr, &key, &arr_val));
    CU_ASSERT_PTR_NOT_NULL(arr_val);
    CU_ASSERT_TRUE(qn_json_arr_is_empty(arr_val));
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_arr"), 0);
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_advance(itr);

    bool_val = qn_false;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_boolean(itr, &key, &bool_val));
    CU_ASSERT_TRUE(bool_val);
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_bool"), 0);
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_advance(itr);

    int_val = 0;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
    CU_ASSERT_EQUAL(int_val, 123);
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_int"), 0);
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_advance(itr);

    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_null(itr, &key));
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_null"), 0);
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_advance(itr);

    key = NULL;
    num_val = 0;
    CU_ASSERT_TRUE(qn_json_itr_get_number(itr, &key, &num_val));
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_num"), 0);
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 789.789L, 0.001L);
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_advance(itr);

    key = NULL;
    obj_val = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_object(itr, &key, &obj_val));
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_obj"), 0);
    CU_ASSERT_TRUE(qn_json_obj_is_empty(obj_val));
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_advance(itr);

    key = NULL;
    str_val = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_string(itr, &key, &str_val));
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_str"), 0);
    CU_ASSERT_EQUAL(qn_str_compare_raw(str_val, "test"), 0);
    qn_json_itr_reclaim_string(itr, key);
    qn_json_itr_reclaim_string(itr, str_val);
    qn_json_itr_advance(itr);

    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));

    qn_json_itr_destroy(itr);
    qn_json_obj_destroy(obj);
}

void test_iterating_empty_array(void)
{
    qn_json_iterator_ptr itr = NULL;
    qn_json_array_ptr arr = NULL;

    if (! (arr = qn_json_arr_create())) CU_FAIL_FATAL("Cannot create the array for iterating");

    CU_ASSERT_PTR_NOT_NULL((itr = qn_json_itr_create())); 
    CU_ASSERT_TRUE(qn_json_itr_push_array(itr, arr, 0)); 
    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
    CU_ASSERT_EQUAL(qn_json_itr_get_type(itr), QN_JSON_UNKNOWN);

    qn_json_itr_destroy(itr);
    qn_json_arr_destroy(arr);
}

void test_iterating_array_with_one_element(void)
{
    qn_integer int_val = 0;
    qn_string key = NULL;
    qn_json_iterator_ptr itr = NULL;
    qn_json_array_ptr arr = NULL;

    if (! (arr = qn_json_arr_create())) CU_FAIL_FATAL("Cannot create the array for iterating");
    if (! qn_json_arr_push_integer(arr, 123)) CU_FAIL_FATAL("Cannot set the integer element for iterating");

    CU_ASSERT_PTR_NOT_NULL((itr = qn_json_itr_create())); 
    CU_ASSERT_TRUE(qn_json_itr_push_array(itr, arr, 0)); 

    CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));
    CU_ASSERT_EQUAL(qn_json_itr_get_type(itr), QN_JSON_INTEGER);

    int_val = 0;
    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, NULL, &int_val));
    CU_ASSERT_EQUAL(int_val, 123);

    key = NULL;
    int_val = 0;
    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
    CU_ASSERT_PTR_NULL(key);
    CU_ASSERT_EQUAL(int_val, 123);

    qn_json_itr_advance(itr);
    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
    CU_ASSERT_EQUAL(qn_json_itr_get_type(itr), QN_JSON_UNKNOWN);

    qn_json_itr_destroy(itr);
    qn_json_arr_destroy(arr);
}

void test_iterating_array_with_all_type_elements(void)
{
    qn_json_object_ptr obj_val = NULL;
    qn_json_array_ptr arr_val = NULL;
    qn_integer int_val = 0;
    qn_number num_val = 0;
    qn_bool bool_val = qn_false;
    qn_string str_val = NULL;
    qn_string key = NULL;
    qn_json_iterator_ptr itr = NULL;
    qn_json_array_ptr arr = NULL;

    if (! (arr = qn_json_arr_create())) CU_FAIL_FATAL("Cannot create the array for iterating");

    if (! qn_json_arr_push_null(arr)) CU_FAIL_FATAL("Cannot push the null element for iterating");
    if (! qn_json_arr_push_new_empty_array(arr)) CU_FAIL_FATAL("Cannot push the array element for iterating");
    if (! qn_json_arr_push_number(arr, 789.789L)) CU_FAIL_FATAL("Cannot push the number element for iterating");
    if (! qn_json_arr_push_boolean(arr, qn_true)) CU_FAIL_FATAL("Cannot push the boolean element for iterating");
    if (! qn_json_arr_unshift_integer(arr, 123)) CU_FAIL_FATAL("Cannot unshift the integer element for iterating");
    if (! qn_json_arr_unshift_cstr(arr, "test")) CU_FAIL_FATAL("Cannot unshift the string element for iterating");
    if (! qn_json_arr_unshift_new_empty_object(arr)) CU_FAIL_FATAL("Cannot unshift the object element for iterating");

    CU_ASSERT_PTR_NOT_NULL((itr = qn_json_itr_create())); 
    CU_ASSERT_TRUE(qn_json_itr_push_array(itr, arr, 0)); 

    key = NULL;
    obj_val = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_object(itr, &key, &obj_val));
    CU_ASSERT_EQUAL(key, NULL);
    CU_ASSERT_TRUE(qn_json_obj_is_empty(obj_val));
    qn_json_itr_advance(itr);

    key = NULL;
    str_val = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_string(itr, &key, &str_val));
    CU_ASSERT_EQUAL(key, NULL);
    CU_ASSERT_EQUAL(qn_str_compare_raw(str_val, "test"), 0);
    qn_json_itr_reclaim_string(itr, str_val);
    qn_json_itr_advance(itr);

    int_val = 0;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
    CU_ASSERT_EQUAL(int_val, 123);
    CU_ASSERT_EQUAL(key, NULL);
    qn_json_itr_advance(itr);

    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_null(itr, &key));
    CU_ASSERT_EQUAL(key, NULL);
    qn_json_itr_advance(itr);

    arr_val = NULL;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_array(itr, &key, &arr_val));
    CU_ASSERT_EQUAL(key, NULL);
    CU_ASSERT_PTR_NOT_NULL(arr_val);
    CU_ASSERT_TRUE(qn_json_arr_is_empty(arr_val));
    qn_json_itr_advance(itr);

    key = NULL;
    num_val = 0;
    CU_ASSERT_TRUE(qn_json_itr_get_number(itr, &key, &num_val));
    CU_ASSERT_EQUAL(key, NULL);
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 789.789L, 0.001L);
    qn_json_itr_advance(itr);

    bool_val = qn_false;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_boolean(itr, &key, &bool_val));
    CU_ASSERT_EQUAL(key, NULL);
    CU_ASSERT_TRUE(bool_val);
    qn_json_itr_advance(itr);

    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));

    qn_json_itr_destroy(itr);
    qn_json_arr_destroy(arr);
}

void test_iterating_five_levels(void)
{
    qn_json_object_ptr obj2 = NULL;
    qn_json_array_ptr arr3 = NULL;
    qn_json_array_ptr arr4 = NULL;
    qn_json_object_ptr obj5 = NULL;
    qn_json_object_ptr obj_val = NULL;
    qn_json_array_ptr arr_val = NULL;
    qn_integer int_val = 0;
    qn_string key = NULL;
    qn_json_iterator_ptr itr = NULL;
    qn_json_object_ptr obj = NULL;

    /* ==== Prepare iterating target ==== */
    if (! (obj = qn_json_obj_create())) CU_FAIL_FATAL("Cannot create the object for iterating");
    if (! (itr = qn_json_itr_create())) CU_FAIL_FATAL("Cannot create the iterator itself");

    if (! (obj2 = qn_json_obj_set_new_empty_object(obj, "_obj2"))) CU_FAIL_FATAL("Cannot set the object element in the second level");
    if (! qn_json_obj_set_integer(obj2, "_int", 2)) CU_FAIL_FATAL("Cannot set the integer element of the object in the second level");

    if (! (arr3 = qn_json_obj_set_new_empty_array(obj2, "_arr3"))) CU_FAIL_FATAL("Cannot set the array in the third level");
    if (! qn_json_arr_push_integer(arr3, 3)) CU_FAIL_FATAL("Cannot push the integer element of the array in the third level");

    if (! (arr4 = qn_json_arr_push_new_empty_array(arr3))) CU_FAIL_FATAL("Cannot push the array in the fourth level");
    if (! qn_json_arr_unshift_integer(arr4, 4)) CU_FAIL_FATAL("Cannot unshift the integer element of the array in the fourth level");

    if (! (obj5 = qn_json_arr_unshift_new_empty_object(arr4))) CU_FAIL_FATAL("Cannot unshift the object in the fifth level");
    if (! qn_json_obj_set_integer(obj5, "_int", 5)) CU_FAIL_FATAL("Cannot set the integer element of the object in the fifth level");

    if (! qn_json_itr_push_object(itr, obj, 0)) CU_FAIL_FATAL("Cannot push the object element in the first level");

    /* ==== Test iterating ==== */
    /* == First level == */
    CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));

    obj_val = NULL;
    key = NULL;
    CU_ASSERT_TRUE(qn_json_itr_get_object(itr, &key, &obj_val));
    CU_ASSERT_EQUAL(qn_json_obj_size(obj_val), 2);
    CU_ASSERT_PTR_NOT_NULL(key);
    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_obj2"), 0);
    qn_json_itr_reclaim_string(itr, key);

    CU_ASSERT_TRUE(qn_json_itr_push_object(itr, obj2, 0));
    {
        /* == Second level == */
        CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));
        
        arr_val = NULL;
        key = NULL;
        CU_ASSERT_TRUE(qn_json_itr_get_array(itr, &key, &arr_val));
        CU_ASSERT_EQUAL(qn_json_arr_size(arr_val), 2);
        CU_ASSERT_PTR_NOT_NULL(key);
        CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_arr3"), 0);
        qn_json_itr_reclaim_string(itr, key);

        CU_ASSERT_TRUE(qn_json_itr_push_array(itr, arr_val, 0));
        {
            /* == Third level == */
            CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));
            
            int_val = 0;
            key = NULL;
            CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
            CU_ASSERT_EQUAL(int_val, 3);
            CU_ASSERT_PTR_NULL(key);

            qn_json_itr_advance(itr);
            CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));
            
            arr_val = NULL;
            key = NULL;
            CU_ASSERT_TRUE(qn_json_itr_get_array(itr, &key, &arr_val));
            CU_ASSERT_EQUAL(qn_json_arr_size(arr_val), 2);
            CU_ASSERT_PTR_NULL(key);

            CU_ASSERT_TRUE(qn_json_itr_push_array(itr, arr_val, 0));
            {
                /* == Fourth level == */
                CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));
                
                obj_val = NULL;
                key = NULL;
                CU_ASSERT_TRUE(qn_json_itr_get_object(itr, &key, &obj_val));
                CU_ASSERT_EQUAL(qn_json_obj_size(obj_val), 1);
                CU_ASSERT_PTR_NULL(key);

                qn_json_itr_advance(itr);
                CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));

                CU_ASSERT_TRUE(qn_json_itr_push_object(itr, obj_val, 0));
                {
                    /* == Fifth level == */
                    CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));

                    int_val = 0;
                    key = NULL;
                    CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
                    CU_ASSERT_EQUAL(int_val, 5);
                    CU_ASSERT_PTR_NOT_NULL(key);
                    CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_int"), 0);
                    qn_json_itr_reclaim_string(itr, key);

                    qn_json_itr_advance(itr);
                    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
                }
                qn_json_itr_pop(itr);

                int_val = 0;
                key = NULL;
                CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
                CU_ASSERT_EQUAL(int_val, 4);
                CU_ASSERT_PTR_NULL(key);

                qn_json_itr_advance(itr);
                CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
            }
            qn_json_itr_pop(itr);

            qn_json_itr_advance(itr);
            CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
        }
        qn_json_itr_pop(itr);

        qn_json_itr_advance(itr);
        CU_ASSERT_TRUE(qn_json_itr_has_next_one(itr));

        int_val = 0;
        key = NULL;
        CU_ASSERT_TRUE(qn_json_itr_get_integer(itr, &key, &int_val));
        CU_ASSERT_EQUAL(int_val, 2);
        CU_ASSERT_PTR_NOT_NULL(key);
        CU_ASSERT_EQUAL(qn_str_compare_raw(key, "_int"), 0);
        qn_json_itr_reclaim_string(itr, key);

        qn_json_itr_advance(itr);
        CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));
    }
    qn_json_itr_pop(itr);

    qn_json_itr_advance(itr);
    CU_ASSERT_FALSE(qn_json_itr_has_next_one(itr));

    qn_json_itr_destroy(itr);
    qn_json_obj_destroy(obj);
}

CU_TestInfo test_normal_cases_of_json_iterating[] = {
    {"test_iterating_empty_object()", test_iterating_empty_object},
    {"test_iterating_object_with_one_element()", test_iterating_object_with_one_element},
    {"test_iterating_object_with_all_type_elements()", test_iterating_object_with_all_type_elements},
    {"test_iterating_empty_array()", test_iterating_empty_array},
    {"test_iterating_array_with_one_element()", test_iterating_array_with_one_element},
    {"test_iterating_array_with_all_type_elements()", test_iterating_array_with_all_type_elements},
    {"test_iterating_five_levels()", test_iterating_five_levels},
    CU_TEST_INFO_NULL
};

// ---- normal test case of parsing ----

void test_parse_empty_object(void)
{
    qn_bool ok = qn_false;
    const char buf[] = {"{}"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (! ok) {
        CU_FAIL("Cannot parse the empty object.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_obj_is_empty(obj_root));

    qn_json_obj_destroy(obj_root);
}

void test_parse_object_holding_one_element(void)
{
    qn_bool ok = qn_false;
    const char buf[] = {"{\"trivial\":\"This is a trivial element.\"}"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_string str = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (! ok) {
        CU_FAIL("Cannot parse the object holding one element.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_obj_is_empty(obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "trivial", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), "This is a trivial element.") == 0);

    qn_json_obj_destroy(obj_root);
}

void test_parse_object_holding_two_elements(void)
{
    qn_bool ok = qn_false;
    qn_json_integer n = 0;
    const char buf[] = {"{\"trivial\":\"This is a trivial element.\",\"int\":-123}"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (! ok) {
        CU_FAIL("Cannot parse the object holding two elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_obj_is_empty(obj_root));

    n = 0;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "int", &n));
    CU_ASSERT_TRUE(n == -123);

    qn_json_obj_destroy(obj_root);
}

void test_parse_object_holding_ordinary_elements(void)
{
    qn_bool ok = qn_false;
    qn_json_number num_val = 0.0L;
    qn_bool bool_val = qn_false;
    const char buf[] = {"{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null}"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the object holding ordinary elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_obj_is_empty(obj_root));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_false", &bool_val));
    CU_ASSERT_FALSE(bool_val);

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_true", &bool_val));
    CU_ASSERT_TRUE(bool_val);

    num_val = 0.0L;
    CU_ASSERT_TRUE(qn_json_obj_get_number(obj_root, "_num", &num_val));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    qn_json_obj_destroy(obj_root);
}

void test_parse_object_holding_empty_complex_elements(void)
{
    qn_bool ok = qn_false;
    const char buf[] = {"{\"_arr\":[],\"_obj\":{}}"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the object holding empty complex elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_obj_is_empty(obj_root));

    arr_elem = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_array(obj_root, "_arr", &arr_elem));
    CU_ASSERT_PTR_NOT_NULL(arr_elem);
    CU_ASSERT_TRUE(qn_json_arr_is_empty(arr_elem));

    obj_elem = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_object(obj_root, "_obj", &obj_elem));
    CU_ASSERT_PTR_NOT_NULL(obj_elem);
    CU_ASSERT_TRUE(qn_json_obj_is_empty(obj_elem));

    qn_json_obj_destroy(obj_root);
}

void test_parse_object_holding_embedded_objects(void)
{
    qn_bool ok = qn_false;
    qn_json_number num_val = 0.0L;
    qn_bool bool_val = qn_false;
    const char buf[] = {"{\"_obj\":{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null},\"_obj2\":{}}"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the object holding embedded objects.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_obj_is_empty(obj_root));

    obj_elem = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_object(obj_root, "_obj", &obj_elem));
    CU_ASSERT_PTR_NOT_NULL(obj_elem);
    CU_ASSERT_TRUE(!qn_json_obj_is_empty(obj_elem));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_elem, "_false", &bool_val));
    CU_ASSERT_FALSE(bool_val);

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_elem, "_true", &bool_val));
    CU_ASSERT_TRUE(bool_val);

    num_val = 0.0L;
    CU_ASSERT_TRUE(qn_json_obj_get_number(obj_elem, "_num", &num_val));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    obj_elem = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_object(obj_root, "_obj2", &obj_elem));
    CU_ASSERT_PTR_NOT_NULL(obj_elem);
    CU_ASSERT_TRUE(qn_json_obj_is_empty(obj_elem));

    qn_json_obj_destroy(obj_root);
}

void test_parse_object_holding_utf8_string(void)
{
    const char buf[] = {"{\"_str\":\"工人\"}"};
    qn_size buf_len = strlen(buf);
    qn_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "工人");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_holding_string_contains_double_quotes(void)
{
    const char buf[] = {"{\"_str\":\"ab\\\"cd\\\"ef\"}"};
    qn_size buf_len = strlen(buf);
    qn_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "ab\"cd\"ef");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_holding_string_contains_backslash(void)
{
    const char buf[] = {"{\"_str\":\"\\\\t\"}"};
    qn_size buf_len = strlen(buf);
    qn_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "\\t");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_integer_value_in_next_chunk_followed_by_others(void)
{
    const char buf[] = {"{\"_int\":"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"12345, \"_str\":\"ABC\"}"};
    qn_size buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    val = -1;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int", &val));
    CU_ASSERT_EQUAL(val, 12345);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_key_input_in_two_chunks_1(void)
{
    const char buf[] = {"{\""};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"_int\":345}"};
    qn_size buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    val = -1;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int", &val));
    CU_ASSERT_EQUAL(val, 345);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_key_input_in_two_chunks_2(void)
{
    const char buf[] = {"{\"_i"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"nt\":678}"};
    qn_size buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    val = -1;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int", &val));
    CU_ASSERT_EQUAL(val, 678);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_key_input_in_two_chunks_3(void)
{
    const char buf[] = {"{\"_int"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"\":90}"};
    qn_size buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    val = -1;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int", &val));
    CU_ASSERT_EQUAL(val, 90);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_key_input_in_two_chunks_4(void)
{
    const char buf[] = {"{\"_int\\"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"\"\":555}"};
    qn_size buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    val = -1;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int\"", &val));
    CU_ASSERT_EQUAL(val, 555);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_string_input_in_two_chunks_1(void)
{
    const char buf[] = {"{\"_str\":\""};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"农民\"}"};
    qn_size buf2_len = strlen(buf2);
    qn_string str = NULL;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "农民");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_string_input_in_two_chunks_2(void)
{
    const char buf[] = {"{\"_str\":\"学"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"生\"}"};
    qn_size buf2_len = strlen(buf2);
    qn_string str = NULL;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "学生");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_string_input_in_two_chunks_3(void)
{
    const char buf[] = {"{\"_str\":\"医生"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"\"}"};
    qn_size buf2_len = strlen(buf2);
    qn_string str = NULL;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "医生");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_string_input_in_two_chunks_4(void)
{
    const char buf[] = {"{\"_str\":\"工程师\\"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"\"\"}"};
    qn_size buf2_len = strlen(buf2);
    qn_string str = NULL;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_obj_get_string(obj_root, "_str", &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "工程师\"");

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_integer_value_input_in_two_chunks(void)
{
    const char buf[] = {"{\"_int\":12"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"345}"};
    qn_size buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    val = -1;
    CU_ASSERT_TRUE(qn_json_obj_get_integer(obj_root, "_int", &val));
    CU_ASSERT_EQUAL(val, 12345);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_true_value_input_in_two_chunks_1(void)
{
    const char buf[] = {"{\"_true\":t"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"rue}"};
    qn_size buf2_len = strlen(buf2);
    qn_bool bool_val = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_true", &bool_val));
    CU_ASSERT_TRUE(bool_val);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_true_value_input_in_two_chunks_2(void)
{
    const char buf[] = {"{\"_true\":tr"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"ue}"};
    qn_size buf2_len = strlen(buf2);
    qn_bool bool_val = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_true", &bool_val));
    CU_ASSERT_TRUE(bool_val);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_true_value_input_in_two_chunks_3(void)
{
    const char buf[] = {"{\"_true\":tru"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"e}"};
    qn_size buf2_len = strlen(buf2);
    qn_bool bool_val = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_true", &bool_val));
    CU_ASSERT_TRUE(bool_val);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_false_value_input_in_two_chunks_1(void)
{
    const char buf[] = {"{\"_false\":f"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"alse}"};
    qn_size buf2_len = strlen(buf2);
    qn_bool bool_val = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_false", &bool_val));
    CU_ASSERT_FALSE(bool_val);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_false_value_input_in_two_chunks_2(void)
{
    const char buf[] = {"{\"_false\":fal"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"se}"};
    qn_size buf2_len = strlen(buf2);
    qn_bool bool_val = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_false", &bool_val));
    CU_ASSERT_FALSE(bool_val);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

void test_parse_object_false_value_input_in_two_chunks_3(void)
{
    const char buf[] = {"{\"_false\":fals"};
    qn_size buf_len = strlen(buf);
    const char buf2[] = {"e}"};
    qn_size buf2_len = strlen(buf2);
    qn_bool bool_val = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    CU_ASSERT_TRUE(qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_obj_get_boolean(obj_root, "_false", &bool_val));
    CU_ASSERT_FALSE(bool_val);

    qn_json_obj_destroy(obj_root);
    qn_json_prs_destroy(prs);
}

// ----

void test_parse_empty_array(void)
{
    qn_bool ok = qn_false;
    const char buf[] = {"[]"};
    qn_size buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the empty array.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_arr_is_empty(arr_root));

    qn_json_arr_destroy(arr_root);
}

void test_parse_array_holding_one_element(void)
{
    qn_bool ok = qn_false;
    const char buf[] = {"[\"This is a trivial element.\"]"};
    qn_size buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_string str = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the array holding one element.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_arr_is_empty(arr_root));

    str = NULL;
    CU_ASSERT_TRUE(qn_json_arr_get_string(arr_root, 0, &str));
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), "This is a trivial element.") == 0);

    qn_json_arr_destroy(arr_root);
}

void test_parse_array_holding_two_elements(void)
{
    qn_bool ok = qn_false;
    qn_json_integer int_val;
    const char buf[] = {"[\"This is a trivial element.\",-123]"};
    qn_size buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the array holding two elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_arr_is_empty(arr_root));
    int_val = 0;
    CU_ASSERT_TRUE(qn_json_arr_get_integer(arr_root, 1, &int_val));
    CU_ASSERT_TRUE(int_val == -123);

    qn_json_arr_destroy(arr_root);
}

void test_parse_array_holding_ordinary_elements(void)
{
    qn_bool ok = qn_false;
    qn_bool bool_val;
    qn_json_number num_val;
    const char buf[] = {"[+123.456,true,false,null]"};
    qn_size buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the array holding ordinary elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_arr_is_empty(arr_root));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_arr_get_boolean(arr_root, 2, &bool_val));
    CU_ASSERT_TRUE(bool_val == qn_false);

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_arr_get_boolean(arr_root, 1, &bool_val));
    CU_ASSERT_TRUE(bool_val == qn_true);

    num_val = 0.0L;
    CU_ASSERT_TRUE(qn_json_arr_get_number(arr_root, 0, &num_val));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    qn_json_arr_destroy(arr_root);
}

void test_parse_array_holding_empty_complex_elements(void)
{
    qn_bool ok = qn_false;
    const char buf[] = {"[{},[]]"};
    qn_size buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the array holding empty complex elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_arr_is_empty(arr_root));

    CU_ASSERT_TRUE(qn_json_arr_get_object(arr_root, 0, &obj_elem));
    CU_ASSERT_PTR_NOT_NULL(obj_elem);
    CU_ASSERT_TRUE(qn_json_obj_is_empty(obj_elem));

    arr_elem = NULL;
    CU_ASSERT_TRUE(qn_json_arr_get_array(arr_root, 1, &arr_elem));
    CU_ASSERT_PTR_NOT_NULL(arr_elem);
    CU_ASSERT_TRUE(qn_json_arr_is_empty(arr_elem));

    qn_json_arr_destroy(arr_root);
}

void test_parse_array_holding_embedded_arrays(void)
{
    qn_bool ok = qn_false;
    qn_bool bool_val;
    qn_json_number num_val;
    const char buf[] = {"[[+123.456,true,false,null],[]]"};
    qn_size buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ok = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ok) {
        CU_FAIL("Cannot parse the array holding embedded arrays.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_arr_is_empty(arr_root));

    arr_elem = NULL;
    CU_ASSERT_TRUE(qn_json_arr_get_array(arr_root, 0, &arr_elem));
    CU_ASSERT_PTR_NOT_NULL(arr_elem);
    CU_ASSERT_TRUE(!qn_json_arr_is_empty(arr_elem));

    bool_val = qn_true;
    CU_ASSERT_TRUE(qn_json_arr_get_boolean(arr_elem, 2, &bool_val));
    CU_ASSERT_FALSE(bool_val);

    bool_val = qn_false;
    CU_ASSERT_TRUE(qn_json_arr_get_boolean(arr_elem, 1, &bool_val));
    CU_ASSERT_TRUE(bool_val);

    num_val = 0.0L;
    CU_ASSERT_TRUE(qn_json_arr_get_number(arr_elem, 0, &num_val));
    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    arr_elem = NULL;
    CU_ASSERT_TRUE(qn_json_arr_get_array(arr_root, 1, &arr_elem));
    CU_ASSERT_PTR_NOT_NULL(arr_elem);
    CU_ASSERT_TRUE(qn_json_arr_is_empty(arr_elem));

    qn_json_arr_destroy(arr_root);
}

CU_TestInfo test_normal_cases_of_json_parsing[] = {
    {"test_parse_empty_object()", test_parse_empty_object},
    {"test_parse_object_holding_one_element()", test_parse_object_holding_one_element},
    {"test_parse_object_holding_two_elements()", test_parse_object_holding_two_elements},
    {"test_parse_object_holding_ordinary_elements()", test_parse_object_holding_ordinary_elements},
    {"test_parse_object_holding_empty_complex_elements()", test_parse_object_holding_empty_complex_elements},
    {"test_parse_object_holding_embedded_objects()", test_parse_object_holding_embedded_objects},
    {"test_parse_object_holding_utf8_string()", test_parse_object_holding_utf8_string}, 
    {"test_parse_object_holding_string_contains_double_quotes()", test_parse_object_holding_string_contains_double_quotes}, 
    {"test_parse_object_holding_string_contains_backslash()", test_parse_object_holding_string_contains_backslash},
    {"test_parse_object_integer_value_in_next_chunk_followed_by_others()", test_parse_object_integer_value_in_next_chunk_followed_by_others}, 
    {"test_parse_object_key_input_in_two_chunks_1()", test_parse_object_key_input_in_two_chunks_1},
    {"test_parse_object_key_input_in_two_chunks_2()", test_parse_object_key_input_in_two_chunks_2},
    {"test_parse_object_key_input_in_two_chunks_3()", test_parse_object_key_input_in_two_chunks_3},
    {"test_parse_object_key_input_in_two_chunks_4()", test_parse_object_key_input_in_two_chunks_4},
    {"test_parse_object_string_input_in_two_chunks_1()", test_parse_object_string_input_in_two_chunks_1},
    {"test_parse_object_string_input_in_two_chunks_2()", test_parse_object_string_input_in_two_chunks_2},
    {"test_parse_object_string_input_in_two_chunks_3()", test_parse_object_string_input_in_two_chunks_3},
    {"test_parse_object_string_input_in_two_chunks_4()", test_parse_object_string_input_in_two_chunks_4},
    {"test_parse_object_integer_value_input_in_two_chunks()", test_parse_object_integer_value_input_in_two_chunks},
    {"test_parse_object_true_value_input_in_two_chunks_1()", test_parse_object_true_value_input_in_two_chunks_1},
    {"test_parse_object_true_value_input_in_two_chunks_2()", test_parse_object_true_value_input_in_two_chunks_2},
    {"test_parse_object_true_value_input_in_two_chunks_3()", test_parse_object_true_value_input_in_two_chunks_3},
    {"test_parse_object_false_value_input_in_two_chunks_1()", test_parse_object_false_value_input_in_two_chunks_1},
    {"test_parse_object_false_value_input_in_two_chunks_2()", test_parse_object_false_value_input_in_two_chunks_2},
    {"test_parse_object_false_value_input_in_two_chunks_3()", test_parse_object_false_value_input_in_two_chunks_3},
    {"test_parse_empty_array()", test_parse_empty_array},
    {"test_parse_array_holding_one_element()", test_parse_array_holding_one_element},
    {"test_parse_array_holding_two_elements()", test_parse_array_holding_two_elements},
    {"test_parse_array_holding_ordinary_elements()", test_parse_array_holding_ordinary_elements},
    {"test_parse_array_holding_empty_complex_elements()", test_parse_array_holding_empty_complex_elements},
    {"test_parse_array_holding_embedded_arrays()", test_parse_array_holding_embedded_arrays},
    CU_TEST_INFO_NULL
};

// ---- abnormal test case of parsing ----

void test_parse_object_without_enough_input_of_key_string(void)
{
    const char buf[] = {"{\"_key\":123456,"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    qn_json_prs_destroy(prs);

    if (!qn_err_json_is_need_more_text_input()) {
        CU_FAIL("The error is not `need more text input`.");
        return;
    } // if
}

void test_parse_object_without_enough_input_of_integer(void)
{
    const char buf[] = {"{\"_key\":1"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    qn_json_prs_destroy(prs);

    if (!qn_err_json_is_need_more_text_input()) {
        CU_FAIL("The error is not `need more text input`.");
        return;
    } // if
}

void test_parse_object_without_enough_input_of_value(void)
{
    const char buf[] = {"{\"_key\":"};
    qn_size buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    CU_ASSERT_FALSE(qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root));
    qn_json_prs_destroy(prs);

    if (!qn_err_json_is_need_more_text_input()) {
        CU_FAIL("The error is not `need more text input`.");
        return;
    } // if
}

CU_TestInfo test_abnormal_cases_of_json_parsing[] = {
    {"test_parse_object_without_enough_input_of_key_string()", test_parse_object_without_enough_input_of_key_string},
    {"test_parse_object_without_enough_input_of_integer()", test_parse_object_without_enough_input_of_integer},
    {"test_parse_object_without_enough_input_of_value()", test_parse_object_without_enough_input_of_value},
    CU_TEST_INFO_NULL
};

// ---- test formatter ----

void test_format_empty_object(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{}", 2), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_string_element(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_cstr(obj_root, "_str", "Normal string"));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_str\":\"Normal string\"}", 24), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_integer_element(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj_root, "_int1", -123));

    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj_root, "_int2", 987));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 26);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_int1\":-123,\"_int2\":987}", 26), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_number_element(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_number(obj_root, "_num1", -123.123456));

    CU_ASSERT_TRUE(qn_json_obj_set_number(obj_root, "_num2", 987.987));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 40);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_num1\":-123.123456,\"_num2\":987.987000}", 40), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_boolean_element(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_boolean(obj_root, "_bool1", qn_false));

    CU_ASSERT_TRUE(qn_json_obj_set_boolean(obj_root, "_bool2", qn_true));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 30);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_bool1\":false,\"_bool2\":true}", 30), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_null_element(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_null(obj_root, "_null"));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 14);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_null\":null}", 14), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_string_contains_double_quotes(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_cstr(obj_root, "_str", "ab\"cd\"ef"));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 21);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_str\":\"ab\\\"cd\\\"ef\"}", 21), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_string_contains_backslash(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_cstr(obj_root, "_str", "\\t"));

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 14);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_str\":\"\\\\t\"}", 14), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_empty_array(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[]", 2), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_string_element(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    CU_ASSERT_TRUE(qn_json_arr_push_string(arr_root, "Normal string"));

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 17);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[\"Normal string\"]", 17), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_integer_element(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr_root, -123));

    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr_root, 987));

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 10);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[-123,987]", 10), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_number_element(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    CU_ASSERT_TRUE(qn_json_arr_unshift_number(arr_root, -123.123456));

    CU_ASSERT_TRUE(qn_json_arr_unshift_number(arr_root, 987.987));

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[987.987000,-123.123456]", 24), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_boolean_element(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    CU_ASSERT_TRUE(qn_json_arr_push_boolean(arr_root, qn_false));

    CU_ASSERT_TRUE(qn_json_arr_push_boolean(arr_root, qn_true));

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 12);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[false,true]", 12), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_null_element(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    CU_ASSERT_TRUE(qn_json_arr_unshift_null(arr_root));

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 6);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[null]", 6), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_complex_element(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    /*-- First array, then object. --*/

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    arr_elem = qn_json_obj_set_new_empty_array(obj_root, "_arr");
    CU_ASSERT_FATAL(arr_elem != NULL);

    CU_ASSERT_TRUE(qn_json_arr_unshift_null(arr_elem));

    obj_elem = qn_json_obj_set_new_empty_object(obj_root, "_obj");
    CU_ASSERT_FATAL(obj_elem != NULL);

    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 25);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_arr\":[null],\"_obj\":{}}", buf_size), 0);

    qn_json_obj_destroy(obj_root);

    /*-- First object, then array. --*/

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    obj_elem = qn_json_obj_set_new_empty_object(obj_root, "_fst");
    CU_ASSERT_FATAL(obj_elem != NULL);

    arr_elem = qn_json_obj_set_new_empty_array(obj_root, "_snd");
    CU_ASSERT_FATAL(arr_elem != NULL);

    CU_ASSERT_TRUE(qn_json_arr_push_integer(arr_elem, 999));

    buf_size = sizeof(buf);
    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_fst\":{},\"_snd\":[999]}", buf_size), 0);

    qn_json_obj_destroy(obj_root);

    /*-- Only empty object --*/

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    obj_elem = qn_json_obj_set_new_empty_object(obj_root, "_obj");
    CU_ASSERT_FATAL(obj_elem != NULL);

    buf_size = sizeof(buf);
    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 11);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_obj\":{}}", buf_size), 0);

    qn_json_obj_destroy(obj_root);

    /*-- Only empty array --*/

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    arr_elem = qn_json_obj_set_new_empty_array(obj_root, "_arr");
    CU_ASSERT_FATAL(arr_elem != NULL);

    buf_size = sizeof(buf);
    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 11);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_arr\":[]}", buf_size), 0);

    qn_json_obj_destroy(obj_root);

    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_complex_element(void)
{
    qn_json_array_ptr arr_root = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    /*-- First object, then array. --*/

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    obj_elem = qn_json_arr_push_new_empty_object(arr_root);
    CU_ASSERT_TRUE(obj_elem != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_null(obj_elem, "_null"));

    arr_elem = qn_json_arr_push_new_empty_array(arr_root);
    CU_ASSERT_FATAL(arr_elem != NULL);

    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 19);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[{\"_null\":null},[]]", buf_size), 0);

    qn_json_arr_destroy(arr_root);

    /*-- First array, then object. --*/

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    arr_elem = qn_json_arr_push_new_empty_array(arr_root);
    CU_ASSERT_FATAL(arr_elem != NULL);

    obj_elem = qn_json_arr_push_new_empty_object(arr_root);
    CU_ASSERT_TRUE(obj_elem != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_string(obj_elem, "_str", "Trivial"));

    buf_size = sizeof(buf);
    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 23);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[[],{\"_str\":\"Trivial\"}]", buf_size), 0);

    qn_json_arr_destroy(arr_root);

    /*-- Only empty object --*/

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    obj_elem = qn_json_arr_push_new_empty_object(arr_root);
    CU_ASSERT_TRUE(obj_elem != NULL);

    buf_size = sizeof(buf);
    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 4);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[{}]", buf_size), 0);

    qn_json_arr_destroy(arr_root);

    /*-- Only empty array --*/

    arr_root = qn_json_arr_create();
    CU_ASSERT_FATAL(arr_root != NULL);

    arr_elem = qn_json_arr_push_new_empty_array(arr_root);
    CU_ASSERT_TRUE(arr_elem != NULL);

    buf_size = sizeof(buf);
    CU_ASSERT_TRUE(qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size));
    CU_ASSERT_EQUAL_FATAL(buf_size, 4);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[[]]", buf_size), 0);

    qn_json_arr_destroy(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_without_enough_buffers(void)
{
    qn_string str = NULL;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[256];
    qn_size buf_size = sizeof(buf);
    qn_size pos = 0;

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    memset(buf, 0, sizeof(buf));

    /*-- Formst result: {"_false":false,"_null":null,"_num":456.456000,"_true":true,"key":"pair","ret":123456} --*/

    obj_root = qn_json_obj_create();
    CU_ASSERT_FATAL(obj_root != NULL);

    str = qn_cs_duplicate("pair");
    CU_ASSERT_FATAL(str != NULL);

    CU_ASSERT_TRUE(qn_json_obj_set_string(obj_root, "key", str));
    CU_ASSERT_TRUE(qn_json_obj_set_integer(obj_root, "ret", 123456));
    CU_ASSERT_TRUE(qn_json_obj_set_number(obj_root, "_num", 456.456L));
    CU_ASSERT_TRUE(qn_json_obj_set_boolean(obj_root, "_true", qn_true));
    CU_ASSERT_TRUE(qn_json_obj_set_boolean(obj_root, "_false", qn_false));
    CU_ASSERT_TRUE(qn_json_obj_set_null(obj_root, "_null"));

    /*-- {" --*/
    buf_size = 2;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- _fals --*/
    pos += buf_size;
    buf_size = 5;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- e" --*/
    pos += buf_size;
    buf_size = 2;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- : --*/
    pos += buf_size;
    buf_size = 1;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- fals --*/
    pos += buf_size;
    buf_size = 4;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- false,"_null": --*/
    pos += buf_size;
    buf_size = 15;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- null --*/
    pos += buf_size;
    buf_size = 4;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- , --*/
    pos += buf_size;
    buf_size = 1;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- "_num":456.45 --*/
    pos += buf_size;
    buf_size = 13;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- 456.456000,"_true": --*/
    pos += buf_size;
    buf_size = 19;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- true,"key":"pa --*/
    pos += buf_size;
    buf_size = 14;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- ir","ret":123 --*/
    pos += buf_size;
    buf_size = 13;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    /*-- 123456} --*/
    pos += buf_size;
    buf_size = 7;
    CU_ASSERT_TRUE(qn_json_fmt_format_object(fmt, obj_root, buf + pos, &buf_size));

    pos += buf_size;
    CU_ASSERT_EQUAL_FATAL(pos, 86);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_false\":false,\"_null\":null,\"_num\":456.456000,\"_true\":true,\"key\":\"pair\",\"ret\":123456}", pos), 0);

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

CU_TestInfo test_normal_cases_of_json_formatting[] = {
    {"test_format_empty_object()", test_format_empty_object},
    {"test_format_object_holding_string_element()", test_format_object_holding_string_element},
    {"test_format_object_holding_integer_element()", test_format_object_holding_integer_element},
    {"test_format_object_holding_number_element()", test_format_object_holding_number_element},
    {"test_format_object_holding_boolean_element()", test_format_object_holding_boolean_element},
    {"test_format_object_holding_null_element()", test_format_object_holding_null_element},
    {"test_format_object_holding_string_contains_double_quotes()", test_format_object_holding_string_contains_double_quotes},
    {"test_format_object_holding_string_contains_backslash()", test_format_object_holding_string_contains_backslash},
    {"test_format_empty_array()", test_format_empty_array},
    {"test_format_array_holding_string_element()", test_format_array_holding_string_element},
    {"test_format_array_holding_integer_element()", test_format_array_holding_integer_element},
    {"test_format_array_holding_number_element()", test_format_array_holding_number_element},
    {"test_format_array_holding_boolean_element()", test_format_array_holding_boolean_element},
    {"test_format_array_holding_null_element()", test_format_array_holding_null_element},
    {"test_format_object_holding_complex_element()", test_format_object_holding_complex_element},
    {"test_format_array_holding_complex_element()", test_format_array_holding_complex_element},
    {"test_format_without_enough_buffers()", test_format_without_enough_buffers},
    CU_TEST_INFO_NULL
};

void test_format_integer_value_without_enough_buffer(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    if (!fmt) CU_FAIL_FATAL("Cannot create a new formatter");

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_integer(obj_root, "_int", 123)) CU_FAIL_FATAL("Cannot create a new integer value");

    buf_size = 8;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 9;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 10;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_number_value_without_enough_buffer(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    if (!fmt) CU_FAIL_FATAL("Cannot create a new formatter");

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_number(obj_root, "_num", 123.4)) CU_FAIL_FATAL("Cannot create a new number value");

    buf_size = 8;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 9;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 10;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 11;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 12;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_true_value_without_enough_buffer(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    if (!fmt) CU_FAIL_FATAL("Cannot create a new formatter");

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_boolean(obj_root, "_true", qn_true)) CU_FAIL_FATAL("Cannot create a new true value");

    buf_size = 9;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 10;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 11;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 12;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_false_value_without_enough_buffer(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    if (!fmt) CU_FAIL_FATAL("Cannot create a new formatter");

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_boolean(obj_root, "_false", qn_false)) CU_FAIL_FATAL("Cannot create a new false value");

    buf_size = 10;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 11;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 12;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 13;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 14;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_null_value_without_enough_buffer(void)
{
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    if (!fmt) CU_FAIL_FATAL("Cannot create a new formatter");

    obj_root = qn_json_obj_create();
    if (!obj_root) CU_FAIL_FATAL("Cannot create a new object");

    if (! qn_json_obj_set_null(obj_root, "_null")) CU_FAIL_FATAL("Cannot create a new null value");

    buf_size = 9;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 10;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 11;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_fmt_reset(fmt);
    buf_size = 12;
    CU_ASSERT_FALSE(qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size));
    CU_ASSERT_TRUE(qn_err_is_out_of_buffer());

    qn_json_obj_destroy(obj_root);
    qn_json_fmt_destroy(fmt);
}

CU_TestInfo test_abnormal_cases_of_json_formatting[] = {
    {"test_format_integer_value_without_enough_buffer()", test_format_integer_value_without_enough_buffer},
    {"test_format_number_value_without_enough_buffer()", test_format_number_value_without_enough_buffer},
    {"test_format_true_value_without_enough_buffer()", test_format_true_value_without_enough_buffer},
    {"test_format_false_value_without_enough_buffer()", test_format_false_value_without_enough_buffer},
    {"test_format_null_value_without_enough_buffer()", test_format_null_value_without_enough_buffer},
    CU_TEST_INFO_NULL
};

// ---- test suites ----

CU_SuiteInfo suites[] = {
    {"test_normal_cases_of_json_manipulating", NULL, NULL, test_normal_cases_of_json_manipulating},
    {"test_normal_cases_of_json_iterating", NULL, NULL, test_normal_cases_of_json_iterating},
    {"test_normal_cases_of_json_parsing", NULL, NULL, test_normal_cases_of_json_parsing},
    {"test_abnormal_cases_of_json_parsing", NULL, NULL, test_abnormal_cases_of_json_parsing},
    {"test_normal_cases_of_json_formatting", NULL, NULL, test_normal_cases_of_json_formatting},
    {"test_abnormal_cases_of_json_formatting", NULL, NULL, test_abnormal_cases_of_json_formatting},
    CU_SUITE_INFO_NULL
};

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    } // if

    pSuite = CU_add_suite("Suite_Test_JSON", NULL, NULL);
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
}
