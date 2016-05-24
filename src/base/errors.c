#include <stdlib.h>

#include "base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
    QN_ERR_SUCCEED              = 0,
    QN_ERR_NO_ENOUGH_MEMORY     = 1001,
    QN_ERR_TRY_AGAIN            = 1002,
    QN_ERR_INVALID_ARGUMENT     = 1003,
    QN_ERR_OVERFLOW_UPPER_BOUND = 1004,
    QN_ERR_OVERFLOW_LOWER_BOUND = 1005,
    QN_ERR_BAD_TEXT_INPUT       = 2001,
};

typedef qn_uint32 qn_err_enum;

static qn_err_enum qn_err_code;

typedef struct _QN_ERROR
{
    qn_uint32 code;
    const char * message;
} qn_error, *qn_error_ptr;

static qn_error qn_errors[] = {
    {QN_ERR_SUCCEED, "Operation succeed"},
    {QN_ERR_NO_ENOUGH_MEMORY, "No enough memory"},
    {QN_ERR_TRY_AGAIN, "Operation would be blocked, try again"},
    {QN_ERR_INVALID_ARGUMENT, "Invalid argument is passed"},
    {QN_ERR_OVERFLOW_UPPER_BOUND, "Integer value is overflow the upper bound"},
    {QN_ERR_OVERFLOW_LOWER_BOUND, "Integer value is overflow the lower bound"},
    {QN_ERR_BAD_TEXT_INPUT, "Bad text input is read"},
};

static
int qn_err_compare(const void * key, const void * item)
{
    if (*((qn_err_enum *)key) < ((qn_error_ptr)item)->code) {
        return -1;
    } // if
    if (*((qn_err_enum *)key) > ((qn_error_ptr)item)->code) {
        return 1;
    } // if
    return 0;
} // qn_err_compare

const char * qn_err_get_message(void)
{
    qn_error_ptr error = (qn_error_ptr) bsearch(&qn_err_code, &qn_errors, sizeof(qn_errors) / sizeof(qn_errors[0]), sizeof(qn_errors[0]), &qn_err_compare);
    return error->message;
} // qn_err_get_message

void qn_err_set_succeed(void)
{
    qn_err_code = QN_ERR_SUCCEED;
} // qn_err_set_succeed

void qn_err_set_no_enough_memory(void)
{
    qn_err_code = QN_ERR_NO_ENOUGH_MEMORY;
} // qn_err_set_no_enough_memory

void qn_err_set_try_again(void)
{
    qn_err_code = QN_ERR_TRY_AGAIN;
} // qn_err_set_try_again

void qn_err_set_invalid_argument(void)
{
    qn_err_code = QN_ERR_INVALID_ARGUMENT;
} // qn_err_set_invalid_argument

void qn_err_set_overflow_upper_bound(void)
{
    qn_err_code = QN_ERR_OVERFLOW_UPPER_BOUND;
} // qn_err_set_overflow_upper_bound

void qn_err_set_overflow_lower_bound(void)
{
    qn_err_code = QN_ERR_OVERFLOW_LOWER_BOUND;
} // qn_err_set_overflow_lower_bound

void qn_err_set_bad_text_input(void)
{
    qn_err_code = QN_ERR_BAD_TEXT_INPUT;
} // qn_err_set_bad_text_input

qn_bool qn_err_is_succeed(void)
{
    return (qn_err_code == QN_ERR_SUCCEED);
} // qn_err_is_succeed

qn_bool qn_err_is_no_enough_memory(void)
{
    return (qn_err_code == QN_ERR_NO_ENOUGH_MEMORY);
} // qn_err_is_no_enough_memory

qn_bool qn_err_is_try_again(void)
{
    return (qn_err_code == QN_ERR_TRY_AGAIN);
} // qn_err_is_try_again

qn_bool qn_err_is_invalid_argument(void)
{
    return (qn_err_code == QN_ERR_TRY_AGAIN);
} // qn_err_is_invalid_argument

qn_bool qn_err_is_overflow_upper_bound(void)
{
    return (qn_err_code == QN_ERR_OVERFLOW_UPPER_BOUND);
} // qn_err_is_overflow_upper_bound

qn_bool qn_err_is_overflow_lower_bound(void)
{
    return (qn_err_code == QN_ERR_OVERFLOW_LOWER_BOUND);
} // qn_err_is_overflow_lower_bound

qn_bool qn_err_is_bad_text_input(void)
{
    return (qn_err_code == QN_ERR_BAD_TEXT_INPUT);
} // qn_err_is_bad_text_input

#ifdef __cplusplus
}
#endif
