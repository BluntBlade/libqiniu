/***************************************************************************//**
* @file qiniu/base/json.h
* @brief The header file declares all JSON-related basic types and functions.
*
* AUTHOR      : liangtao@qiniu.com (QQ: 510857)
* COPYRIGHT   : 2016(c) Shanghai Qiniu Information Technologies Co., Ltd.
* DESCRIPTION :
*
* This header file declares all JSON-related basic functions, like that create
* and manipulate JSON objects or arrays. A set of iterating functions are also
* included for traversing each element in objects or arrays.
*******************************************************************************/

#ifndef __QN_BASE_JSON_H__
#define __QN_BASE_JSON_H__ 1

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ==== Declaration of JSON (Abbreviation: json) ==== */


/***************************************************************************//**
* @defgroup JSON-Object Implementation of JSON Object
*
* The **qn_json_object_ptr** type represents a JSON object. Use this type to
* collect a set of pairs of key and value. The maximum quantity of pairs is
* 65535.
*******************************************************************************/

struct _QN_JSON_OBJECT;
typedef struct _QN_JSON_OBJECT * qn_json_object_ptr;

/***************************************************************************//**
* @defgroup JSON-Array Implementation of JSON Array
*
* The **qn_json_array_ptr** type represents a JSON array. Use this type to
* collect a list of values. The maximum quantity of values is 65535.
* This type can be used as a **bidirectional queue**.
*******************************************************************************/

struct _QN_JSON_ARRAY;
typedef struct _QN_JSON_ARRAY * qn_json_array_ptr;

typedef qn_bool qn_json_boolean;
typedef qn_string qn_json_string;
typedef qn_integer qn_json_integer;
typedef qn_number qn_json_number;

typedef union _QN_JSON_VARIANT
{
    qn_json_object_ptr object;
    qn_json_array_ptr array;
    qn_json_string string;
    qn_json_integer integer;
    qn_json_number number;
    qn_json_boolean boolean;
} qn_json_variant_un, *qn_json_variant_ptr;

typedef enum _QN_JSON_TYPE {
    QN_JSON_UNKNOWN = 0,
    QN_JSON_NULL = 1,
    QN_JSON_BOOLEAN = 2,
    QN_JSON_INTEGER = 3,
    QN_JSON_NUMBER = 4,
    QN_JSON_STRING = 5,
    QN_JSON_ARRAY = 6,
    QN_JSON_OBJECT = 7
} qn_json_type;

/* ==== Declaration of JSON Object ==== */

/* == Constructor & Destructor methods == */

QN_SDK extern qn_json_object_ptr qn_json_create_object(void);
QN_SDK extern qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr restrict obj, const char * restrict key);
QN_SDK extern qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr restrict obj, const char * restrict key);
QN_SDK extern void qn_json_destroy_object(qn_json_object_ptr restrict obj);

/* == Property methods == */

QN_SDK extern qn_uint qn_json_object_size(qn_json_object_ptr restrict obj);

/* == Check methods == */

static inline qn_bool qn_json_is_empty_object(qn_json_object_ptr restrict obj)
{
    return qn_json_object_size(obj) == 0;
}

/* == Set & Get methods == */

QN_SDK extern qn_bool qn_json_obj_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict * val);
QN_SDK extern qn_bool qn_json_obj_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict * val);
QN_SDK extern qn_bool qn_json_obj_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict * val);
QN_SDK extern qn_bool qn_json_obj_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer * val);
QN_SDK extern qn_bool qn_json_obj_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number * val);
QN_SDK extern qn_bool qn_json_obj_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool * val);

/* ==== */

QN_SDK extern qn_bool qn_json_obj_set_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict val);
QN_SDK extern qn_bool qn_json_obj_set_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict val);
QN_SDK extern qn_bool qn_json_obj_set_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict val);
QN_SDK extern qn_bool qn_json_obj_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer val);
QN_SDK extern qn_bool qn_json_obj_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number val);
QN_SDK extern qn_bool qn_json_obj_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val);
QN_SDK extern qn_bool qn_json_obj_set_null(qn_json_object_ptr restrict obj, const char * restrict key);

static inline qn_bool qn_json_obj_set_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val)
{
    qn_string new_str = qn_cs_duplicate(val);
    if (! new_str) return qn_false;
    if (! qn_json_obj_set_string(obj, key, new_str)) {
        qn_str_destroy(new_str);
        return qn_false;
    }
    return qn_true;
}

static inline qn_bool qn_json_obj_set_text(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val, qn_size val_size)
{
    qn_string new_str = qn_cs_clone(val, val_size);
    if (! new_str) return qn_false;
    if (! qn_json_obj_set_string(obj, key, new_str)) {
        qn_str_destroy(new_str);
        return qn_false;
    }
    return qn_true;
}

QN_SDK extern void qn_json_obj_unset(qn_json_object_ptr restrict obj, const char * restrict key);

/* ==== */

QN_SDK extern qn_bool qn_json_obj_rename(qn_json_object_ptr restrict obj, const char * restrict old_key, const char * new_key);

/* ==== Declaration of JSON Array ==== */

/* == Constructor & Destructor methods == */

QN_SDK extern qn_json_array_ptr qn_json_create_array(void);
QN_SDK extern qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr restrict arr);
QN_SDK extern qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr restrict arr);
QN_SDK extern qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr restrict arr);
QN_SDK extern qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr restrict arr);
QN_SDK extern void qn_json_destroy_array(qn_json_array_ptr restrict arr);

/* == Property methods == */

QN_SDK extern qn_uint qn_json_array_size(qn_json_array_ptr restrict arr);

static inline qn_bool qn_json_is_empty_array(qn_json_array_ptr restrict obj)
{
    return qn_json_array_size(obj) == 0;
}

/* == Set & Get methods == */

QN_SDK extern qn_bool qn_json_arr_get_object(qn_json_array_ptr restrict arr, qn_uint n, qn_json_object_ptr restrict * val);
QN_SDK extern qn_bool qn_json_arr_get_array(qn_json_array_ptr restrict arr, qn_uint n, qn_json_array_ptr restrict * val);
QN_SDK extern qn_bool qn_json_arr_get_string(qn_json_array_ptr restrict arr, qn_uint n, qn_string restrict * val);
QN_SDK extern qn_bool qn_json_arr_get_integer(qn_json_array_ptr restrict arr, qn_uint n, qn_json_integer * val);
QN_SDK extern qn_bool qn_json_arr_get_number(qn_json_array_ptr restrict arr, qn_uint n, qn_json_number * val);
QN_SDK extern qn_bool qn_json_arr_get_boolean(qn_json_array_ptr restrict arr, qn_uint n, qn_bool * val);

/* ==== */

QN_SDK extern qn_bool qn_json_arr_push_object(qn_json_array_ptr restrict arr, qn_json_object_ptr restrict val);
QN_SDK extern qn_bool qn_json_arr_push_array(qn_json_array_ptr restrict arr, qn_json_array_ptr restrict val);
QN_SDK extern qn_bool qn_json_arr_push_string(qn_json_array_ptr restrict arr, qn_string restrict val);
QN_SDK extern qn_bool qn_json_arr_push_integer(qn_json_array_ptr restrict arr, qn_json_integer val);
QN_SDK extern qn_bool qn_json_arr_push_number(qn_json_array_ptr restrict arr, qn_json_number val);
QN_SDK extern qn_bool qn_json_arr_push_boolean(qn_json_array_ptr restrict arr, qn_bool val);
QN_SDK extern qn_bool qn_json_arr_push_null(qn_json_array_ptr restrict arr);

static inline qn_bool qn_json_arr_push_cstr(qn_json_array_ptr restrict arr, const char * restrict val)
{
    qn_string new_str = qn_cs_duplicate(val);
    if (! new_str) return qn_false;
    if (! qn_json_arr_push_string(arr, new_str)) {
        qn_str_destroy(new_str);
        return qn_false;
    }
    return qn_true;
}

static inline qn_bool qn_json_arr_push_text(qn_json_array_ptr restrict arr, const char * restrict val, qn_size val_size)
{
    qn_string new_str = qn_cs_clone(val, val_size);
    if (! new_str) return qn_false;
    if (! qn_json_arr_push_string(arr, new_str)) {
        qn_str_destroy(new_str);
        return qn_false;
    }
    return qn_true;
}

QN_SDK extern void qn_json_arr_pop(qn_json_array_ptr restrict arr);

/* ==== */

QN_SDK extern qn_bool qn_json_arr_unshift_object(qn_json_array_ptr restrict arr, qn_json_object_ptr restrict val);
QN_SDK extern qn_bool qn_json_arr_unshift_array(qn_json_array_ptr restrict arr, qn_json_array_ptr restrict val);
QN_SDK extern qn_bool qn_json_arr_unshift_string(qn_json_array_ptr restrict arr, qn_string restrict val);
QN_SDK extern qn_bool qn_json_arr_unshift_integer(qn_json_array_ptr restrict arr, qn_json_integer val);
QN_SDK extern qn_bool qn_json_arr_unshift_number(qn_json_array_ptr restrict arr, qn_json_number val);
QN_SDK extern qn_bool qn_json_arr_unshift_boolean(qn_json_array_ptr restrict arr, qn_bool val);
QN_SDK extern qn_bool qn_json_arr_unshift_null(qn_json_array_ptr restrict arr);

static inline qn_bool qn_json_arr_unshift_cstr(qn_json_array_ptr restrict arr, const char * restrict val)
{
    qn_string new_str = qn_cs_duplicate(val);
    if (! new_str) return qn_false;
    if (! qn_json_arr_unshift_string(arr, new_str)) {
        qn_str_destroy(new_str);
        return qn_false;
    }
    return qn_true;
}

static inline qn_bool qn_json_arr_unshift_text(qn_json_array_ptr restrict arr, const char * restrict val, qn_size val_size)
{
    qn_string new_str = qn_cs_clone(val, val_size);
    if (! new_str) return qn_false;
    if (! qn_json_arr_unshift_string(arr, new_str)) {
        qn_str_destroy(new_str);
        return qn_false;
    }
    return qn_true;
}

QN_SDK extern void qn_json_arr_shift(qn_json_array_ptr restrict arr);

/* ==== */

QN_SDK extern qn_bool qn_json_arr_replace_object(qn_json_array_ptr restrict arr, qn_uint n, qn_json_object_ptr restrict val);
QN_SDK extern qn_bool qn_json_arr_replace_array(qn_json_array_ptr restrict arr, qn_uint n, qn_json_array_ptr restrict val);
QN_SDK extern qn_bool qn_json_arr_replace_string(qn_json_array_ptr restrict arr, qn_uint n, qn_string restrict val);
QN_SDK extern qn_bool qn_json_arr_replace_integer(qn_json_array_ptr restrict arr, qn_uint n, qn_json_integer val);
QN_SDK extern qn_bool qn_json_arr_replace_number(qn_json_array_ptr restrict arr, qn_uint n, qn_json_number val);
QN_SDK extern qn_bool qn_json_arr_replace_boolean(qn_json_array_ptr restrict arr, qn_uint n, qn_bool val);
QN_SDK extern qn_bool qn_json_arr_replace_null(qn_json_array_ptr restrict arr, qn_uint n);

static inline qn_bool qn_json_arr_replace_cstr(qn_json_array_ptr restrict arr, qn_uint n, const char * restrict val)
{
    qn_string new_str = qn_cs_duplicate(val);
    if (! new_str) return qn_false;
    if (! qn_json_arr_replace_string(arr, n, new_str)) {
        qn_str_destroy(new_str);
        return qn_true;
    }
    return qn_false;
}

static inline qn_bool qn_json_arr_replace_text(qn_json_array_ptr restrict arr, qn_uint n, const char * restrict val, qn_size val_size)
{
    qn_string new_str = qn_cs_clone(val, val_size);
    if (! new_str) return qn_false;
    if (! qn_json_arr_replace_string(arr, n, new_str)) {
        qn_str_destroy(new_str);
        return qn_true;
    }
    return qn_false;
}

/***************************************************************************//**
* @defgroup JSON-Iterator Implementation of JSON Object and Array Iterator
*
* The **qn_json_iterator_ptr** represents a JSON iterator.  Use this
* iterator type to iterate all elements reside in a JSON object or array.
* Moreover, the iterator can hold one status value for each level of the
* hierarchy, so the caller can implement some useful features based on this, 
* like the JSON formatter and parser provided by this SDK.
*******************************************************************************/

struct _QN_JSON_ITERATOR;
typedef struct _QN_JSON_ITERATOR * qn_json_iterator_ptr;

QN_SDK extern qn_json_iterator_ptr qn_json_itr_create(void);
QN_SDK extern void qn_json_itr_destroy(qn_json_iterator_ptr restrict itr);
QN_SDK extern void qn_json_itr_reset(qn_json_iterator_ptr restrict itr);
QN_SDK extern void qn_json_itr_rewind(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr restrict itr);
QN_SDK extern int qn_json_itr_done_steps(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_string qn_json_itr_get_key(qn_json_iterator_ptr restrict itr);

QN_SDK extern void qn_json_itr_set_status(qn_json_iterator_ptr restrict itr, qn_uint32 sts);
QN_SDK extern qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_bool qn_json_itr_push_object(qn_json_iterator_ptr restrict itr, qn_json_object_ptr restrict obj);
QN_SDK extern qn_bool qn_json_itr_push_array(qn_json_iterator_ptr restrict itr, qn_json_array_ptr restrict arr);
QN_SDK extern void qn_json_itr_pop(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr restrict itr);
QN_SDK extern qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr restrict itr);

enum
{
    QN_JSON_ITR_OK = 0,
    QN_JSON_ITR_NO_MORE = 1
};

typedef int (*qn_json_itr_callback_fn)(void * data, qn_json_type cls, qn_json_variant_ptr restrict val);
QN_SDK extern int qn_json_itr_advance(qn_json_iterator_ptr restrict itr, void * data, qn_json_itr_callback_fn cb);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE_JSON_H__

