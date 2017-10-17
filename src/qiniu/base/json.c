/***************************************************************************//**
* @file qiniu/base/json.c
* @brief The header file declares all JSON-related basic types and functions.
*
* AUTHOR      : liangtao@qiniu.com (QQ: 510857)
* COPYRIGHT   : 2017(c) Shanghai Qiniu Information Technologies Co., Ltd.
* DESCRIPTION :
*
* This source file define all JSON-related basic functions, like that create
* and manipulate JSON objects or arrays. A set of iterating functions are also
* included for traversing each element in objects or arrays.
*******************************************************************************/

#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* == Common functions == */

static inline qn_uint16 qn_json_round_to_multiple_of_8(qn_uint16 n)
{
    qn_uint16 r = ((n - 1 + 8) / 8);
    return r * 8;
}

/* ==== Definition of JSON Variant ==== */

/***************************************************************************//**
* @defgroup JSON-Variant Implementation of JSON Variant
*
* The **qn_json_variant_ptr** type represents a JSON variant used to save a
* JSON value.
*******************************************************************************/

static inline void qn_json_destroy_variant(qn_json_type type, qn_json_variant_ptr restrict var)
{
    switch (type) {
        case QN_JSON_OBJECT: qn_json_obj_destroy(var->object); break;
        case QN_JSON_ARRAY: qn_json_arr_destroy(var->array); break;
        case QN_JSON_STRING: qn_str_destroy(var->string); break;
        default: break;
    } // switch
}

typedef struct _QN_JSON_ATTRIBUTE
{
    unsigned char type:4;
} qn_json_attribute_st, *qn_json_attribute_ptr;

/* ==== Definition of JSON Object ==== */

#define QN_JSON_OBJ_DEFAULT_CAPACITY 8

typedef struct _QN_JSON_OBJECT
{
    void * data;
    qn_uint16 cnt;
    qn_uint16 cap;

    qn_string keys[QN_JSON_OBJ_DEFAULT_CAPACITY];
    qn_json_variant_un values[QN_JSON_OBJ_DEFAULT_CAPACITY];
    qn_json_attribute_st attributes[QN_JSON_OBJ_DEFAULT_CAPACITY];
} qn_json_object_st;

#define qn_json_obj_key_offset(ptr, cap) ((qn_string *) ptr)
#define qn_json_obj_variant_offset(ptr, cap) ((qn_json_variant_ptr) ((char *) ptr + sizeof(qn_string) * cap))
#define qn_json_obj_attribute_offset(ptr, cap) ((qn_json_attribute_ptr) ((char *) qn_json_obj_variant_offset(ptr, cap) + sizeof(qn_json_variant_un) * cap))

// typedef qn_uint32 qn_json_hash;
// static qn_json_hash qn_json_obj_calculate_hash(const char * restrict cstr)
// {
//     qn_json_hash hash = 5381;
//     int c;
// 
//     while ((c = *cstr++) != '\0') {
//         hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//     } // while
//     return hash;
// }

/* == Constructor & Destructor methods == */

/***************************************************************************//**
* @ingroup JSON-Object
*
* Allocate and construct a new JSON object.
*
* @retval non-NULL A pointer to the new JSON object.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_obj_create(void)
{
    qn_json_object_ptr new_obj = calloc(1, sizeof(qn_json_object_st));
    if (! new_obj) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_obj->data = &new_obj->keys[0];
    new_obj->cap = sizeof(new_obj->values) / sizeof(new_obj->values[0]);
    return new_obj;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Destruct and deallocate a JSON object.
*
* @param [in] obj The pointer to the object to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_obj_destroy(qn_json_object_ptr restrict obj)
{
    qn_uint16 i = 0;
    qn_string * keys = NULL;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;
    
    if (obj) {
        keys = qn_json_obj_key_offset(obj->data, obj->cap);
        vars = qn_json_obj_variant_offset(obj->data, obj->cap);
        attrs = qn_json_obj_attribute_offset(obj->data, obj->cap);

        for (i = 0; i < obj->cnt; i += 1) {
            qn_json_destroy_variant(attrs[i].type, &vars[i]);
            qn_str_destroy(keys[i]);
        } // for
        if (obj->data != &obj->keys[0]) free(obj->data);
        free(obj);
    } // if
}

static qn_uint32 qn_json_bsearch_key(qn_string * restrict keys, qn_uint32 cnt, const char * restrict key, int * restrict existence)
{
    qn_uint32 begin = 0;
    qn_uint32 end = cnt;
    qn_uint32 mid = 0;
    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        if (qn_str_compare_raw(keys[mid], key) < 0) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    // -- Finally, the `begin` variable points to the first key that is equal to or larger than the given key,
    //    as an insert point.
    *existence = (begin < cnt) ? qn_str_compare_raw(keys[begin], key) : 1;
    return begin;
}

static qn_bool qn_json_obj_augment(qn_json_object_ptr restrict obj)
{
    qn_uint16 new_cap = qn_json_round_to_multiple_of_8(obj->cap + (obj->cap >> 1));
    size_t new_size = (sizeof(qn_string) + sizeof(qn_json_variant_un) + sizeof(qn_json_attribute_st)) * new_cap;
    void * new_data = calloc(1, new_size);
    if (! new_data) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_data, obj->data, sizeof(qn_string) * obj->cnt);
    memcpy(qn_json_obj_variant_offset(new_data, new_cap), qn_json_obj_variant_offset(obj->data, obj->cap), sizeof(qn_json_variant_un) * obj->cnt);
    memcpy(qn_json_obj_attribute_offset(new_data, new_cap), qn_json_obj_attribute_offset(obj->data, obj->cap), sizeof(qn_json_attribute_st) * obj->cnt);

    if (obj->data != &obj->keys[0]) free(obj->data);

    obj->data = new_data;
    obj->cap = new_cap;
    return qn_true;
}

static void qn_json_obj_move(qn_string * keys, qn_json_variant_ptr vars, qn_json_attribute_ptr attrs, qn_uint32 to, qn_uint32 from, qn_uint32 cnt)
{
    memmove(&keys[to], &keys[from], sizeof(qn_string) * cnt);
    memmove(&vars[to], &vars[from], sizeof(qn_json_variant_un) * cnt);
    memmove(&attrs[to], &attrs[from], sizeof(qn_json_attribute_st) * cnt);
}

static qn_bool qn_json_obj_set_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_type type, qn_json_variant_un new_var)
{
    qn_uint32 pos = 0;
    qn_string new_key = NULL;
    int existence = 0;
    qn_string * keys = NULL;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(obj);
    assert(key);

    keys = qn_json_obj_key_offset(obj->data, obj->cap);
    vars = qn_json_obj_variant_offset(obj->data, obj->cap);
    attrs = qn_json_obj_attribute_offset(obj->data, obj->cap);

    pos = qn_json_bsearch_key(keys, obj->cnt, key, &existence);
    if (existence == 0) {
        /* There is an element according to the given key. */
        qn_json_destroy_variant(attrs[pos].type, &vars[pos]);
        attrs[pos].type = type;
        vars[pos] = new_var;
        return qn_true;
    } // if

    if ((obj->cap - obj->cnt) <= 0 && ! qn_json_obj_augment(obj)) return qn_false;
    if (! (new_key = qn_cs_duplicate(key))) return qn_false;

    if (pos < obj->cnt) {
        memmove(&keys[pos+1], &keys[pos], sizeof(qn_string) * (obj->cnt - pos));
        memmove(&vars[pos+1], &vars[pos], sizeof(qn_json_variant_un) * (obj->cnt - pos));
        memmove(&attrs[pos+1], &attrs[pos], sizeof(qn_json_attribute_st) * (obj->cnt - pos));
    } // if

    keys[pos] = new_key;
    attrs[pos].type = type;
    vars[pos] = new_var;

    obj->cnt += 1;
    return qn_true;
}

/* == Property methods == */

/***************************************************************************//**
* @ingroup JSON-Object
*
* Return the current quantity of pairs of the object.
*
* @param [in] obj The non-NULL pointer to the object.
* @retval Integer-Value The current quantity of pairs of the object.
*******************************************************************************/
QN_SDK qn_uint16 qn_json_obj_size(qn_json_object_ptr restrict obj)
{
    assert(obj);
    return obj->cnt;
}

/* == Set & Get methods == */

static qn_json_variant_ptr qn_json_obj_get_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_type type)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;
    int existence = 0;
    qn_uint32 pos = 0;

    assert(obj);
    assert(key);

    if (obj->cnt == 0) {
        qn_err_set_no_such_entry();
        return NULL;
    }

    pos = qn_json_bsearch_key(qn_json_obj_key_offset(obj->data, obj->cap), obj->cnt, key, &existence);
    if (existence != 0) {
        qn_err_set_no_such_entry();
        return NULL;
    }

    attrs = qn_json_obj_attribute_offset(obj->data, obj->cap);
    if (attrs[pos].type != type) {
        qn_err_json_set_not_this_type();
        return NULL;
    }

    vars = qn_json_obj_variant_offset(obj->data, obj->cap);
    return &vars[pos];
}

QN_SDK qn_bool qn_json_obj_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict * val)
{
    qn_json_variant_ptr var = qn_json_obj_get_variant(obj, key, QN_JSON_OBJECT);
    if (! var) return qn_false;
    *val = var->object;
    return qn_true;
}

QN_SDK qn_bool qn_json_obj_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict * val)
{
    qn_json_variant_ptr var = qn_json_obj_get_variant(obj, key, QN_JSON_ARRAY);
    if (! var) return qn_false;
    *val = var->array;
    return qn_true;
}

QN_SDK qn_bool qn_json_obj_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict * val)
{
    qn_json_variant_ptr var = qn_json_obj_get_variant(obj, key, QN_JSON_STRING);
    if (! var) return qn_false;
#if defined(QN_CFG_SUPPORT_MULTITHREAD)
    *val = qn_str_duplicate(var->string);
#else
    *val = var->string;
#endif
    return qn_true;
}

QN_SDK qn_bool qn_json_obj_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer * val)
{
    qn_json_variant_ptr var = qn_json_obj_get_variant(obj, key, QN_JSON_INTEGER);
    if (! var) return qn_false;
    *val = var->integer;
    return qn_true;
}

QN_SDK qn_bool qn_json_obj_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number * val)
{
    qn_json_variant_ptr var = qn_json_obj_get_variant(obj, key, QN_JSON_NUMBER);
    if (! var) return qn_false;
    *val = var->number;
    return qn_true;
}

QN_SDK qn_bool qn_json_obj_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool * val)
{
    qn_json_variant_ptr var = qn_json_obj_get_variant(obj, key, QN_JSON_BOOLEAN);
    if (! var) return qn_false;
    *val = var->boolean;
    return qn_true;
}

/* ==== */

QN_SDK qn_bool qn_json_obj_set_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.object = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_OBJECT, new_var);
}

QN_SDK qn_bool qn_json_obj_set_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.array = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_ARRAY, new_var);
}

QN_SDK qn_bool qn_json_obj_set_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.string = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_STRING, new_var);
}

QN_SDK qn_bool qn_json_obj_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer val)
{
    qn_json_variant_un new_var;
    new_var.integer = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_INTEGER, new_var);
}

QN_SDK qn_bool qn_json_obj_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number val)
{
    qn_json_variant_un new_var;
    new_var.number = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_NUMBER, new_var);
}

QN_SDK qn_bool qn_json_obj_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val)
{
    qn_json_variant_un new_var;
    new_var.boolean = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_BOOLEAN, new_var);
}

QN_SDK qn_bool qn_json_obj_set_null(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant_un new_var;
    new_var.integer = 0;
    return qn_json_obj_set_variant(obj, key, QN_JSON_NULL, new_var);
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Unset the element which corresponds to the key.
*
* @param [in] obj The non-NULL pointer to the target object.
* @param [in] key The key of the unsetting element.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_obj_unset(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_uint32 pos = 0;
    int existence = 0;
    qn_string * keys = NULL;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(obj);
    assert(key);

    if (obj->cnt == 0) return;

    keys = qn_json_obj_key_offset(obj->data, obj->cap);

    pos = qn_json_bsearch_key(keys, obj->cnt, key, &existence);
    if (existence != 0) return; // There is no element corresponds to the key.

    vars = qn_json_obj_variant_offset(obj->data, obj->cap);
    attrs = qn_json_obj_attribute_offset(obj->data, obj->cap);

    qn_json_destroy_variant(attrs[pos].type, &vars[pos]);
    qn_str_destroy(keys[pos]);
    if (pos < obj->cnt - 1) qn_json_obj_move(keys, vars, attrs, pos, pos + 1, (obj->cnt - pos - 1));
    obj->cnt -= 1;
}

/* ==== */

QN_SDK qn_bool qn_json_obj_rename(qn_json_object_ptr restrict obj, const char * restrict old_key, const char * new_key)
{
    qn_uint32 old_pos = 0;
    qn_uint32 new_pos = 0;
    qn_string new_key_str = NULL;
    int existence = 0;
    qn_string * keys = NULL;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;
    qn_json_variant_un old_var;
    qn_json_attribute_st old_attr;

    assert(obj);
    assert(old_key);
    assert(new_key);

    if (obj->cnt == 0) {
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    if (strcmp(old_key, new_key) == 0) return qn_true; /* The old key is exactly the same to the new key. */

    keys = qn_json_obj_key_offset(obj->data, obj->cap);
    vars = qn_json_obj_variant_offset(obj->data, obj->cap);
    attrs = qn_json_obj_attribute_offset(obj->data, obj->cap);

    old_pos = qn_json_bsearch_key(keys, obj->cnt, old_key, &existence);
    if (existence != 0) {
        /* ---- There is no element corresponds to the old key. ---- */
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    new_pos = qn_json_bsearch_key(keys, obj->cnt, new_key, &existence);
    if (existence == 0) {
        /* ---- There is a variant corresponds to the new key. ---- */
        /* Destroy the variant to be replaced. */
        qn_json_destroy_variant(attrs[new_pos].type, &vars[new_pos]);

        /* Replace the variant. */
        attrs[new_pos].type = attrs[old_pos].type;
        vars[new_pos] = vars[old_pos];

        /* Destroy the old key. */
        qn_str_destroy(keys[old_pos]);

        if (old_pos < obj->cnt - 1) qn_json_obj_move(keys, vars, attrs, old_pos, old_pos + 1, (obj->cnt - old_pos - 1));
        obj->cnt -= 1;
        return qn_true;
    } // if

    /* ---- There is no variant corresponds to the new key. ---- */
    if (! (new_key_str = qn_cs_duplicate(new_key))) return qn_false;

    /* Destroy the old key. */
    qn_str_destroy(keys[old_pos]);

    if (old_pos == new_pos) {
        /* The two keys reside in the same position. */
        keys[old_pos] = new_key_str;
        return qn_true;
    } // if

    old_var = vars[old_pos];
    old_attr = attrs[old_pos];

    if (old_pos < new_pos) {
        /* The old key resides in a position before the new key. */
        qn_json_obj_move(keys, vars, attrs, old_pos, old_pos + 1, (new_pos - old_pos - 1));
        new_pos -= 1;
    } else {
        /* The old key resides in a position after the new key. */
        qn_json_obj_move(keys, vars, attrs, new_pos + 1, new_pos, (old_pos - new_pos));
    } // if
    keys[new_pos] = new_key_str;
    vars[new_pos] = old_var;
    attrs[new_pos] = old_attr;
    return qn_true;
}

/* == Check methods == */

QN_SDK qn_json_type qn_json_obj_get_type(qn_json_object_ptr restrict obj, const char * restrict key)
{
    int existence = 0;
    qn_uint32 pos = 0;

    assert(obj);
    assert(key);

    if (obj->cnt == 0) {
        qn_err_set_no_such_entry();
        return QN_JSON_UNKNOWN;
    }

    pos = qn_json_bsearch_key(qn_json_obj_key_offset(obj->data, obj->cap), obj->cnt, key, &existence);
    if (existence != 0) {
        qn_err_set_no_such_entry();
        return QN_JSON_UNKNOWN;
    }

    return (qn_json_obj_attribute_offset(obj->data, obj->cap))[pos].type;
}

/* ==== Definition of array of JSON ==== */

#define QN_JSON_ARR_DEFAULT_CAPACITY 8

typedef struct _QN_JSON_ARRAY
{
    void * data;
    qn_uint16 cnt;
    qn_uint16 cap;
    qn_uint16 begin;
    qn_uint16 end;

    qn_json_variant_un values[QN_JSON_ARR_DEFAULT_CAPACITY];
    qn_json_attribute_st attributes[QN_JSON_ARR_DEFAULT_CAPACITY];
} qn_json_array_st;

#define qn_json_arr_variant_offset(ptr, cap) ((qn_json_variant_ptr) ptr)
#define qn_json_arr_attribute_offset(ptr, cap) ((qn_json_attribute_ptr) ((char *) qn_json_arr_variant_offset(ptr, cap) + sizeof(qn_json_variant_un) * cap))

/* == Constructor & Destructor methods == */

/***************************************************************************//**
* @ingroup JSON-Array
*
* Allocate and construct a new JSON array.
*
* @retval non-NULL A pointer to the new JSON array.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_arr_create(void)
{
    qn_json_array_ptr new_arr = calloc(1, sizeof(qn_json_array_st));
    if (! new_arr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_arr->data = &new_arr->values[0];
    new_arr->cap = sizeof(new_arr->values) / sizeof(new_arr->values[0]);
    return new_arr;
}

enum
{
    QN_JSON_ARR_PUSHING = 0,
    QN_JSON_ARR_UNSHIFTING = 1
};

static qn_bool qn_json_arr_augment(qn_json_array_ptr restrict arr, int direct)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;
    qn_json_variant_ptr new_vars = NULL;
    qn_json_attribute_ptr new_attrs = NULL;
    qn_uint16 offset = 0;
    qn_uint16 new_cap = qn_json_round_to_multiple_of_8(arr->cap + (arr->cap >> 1));
    size_t new_size = (sizeof(qn_json_variant_un) + sizeof(qn_json_attribute_st)) * new_cap;
    void * new_data = calloc(1,  new_size);
    if (! new_data) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    vars = qn_json_arr_variant_offset(arr->data, arr->cap);
    attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);

    new_vars = qn_json_arr_variant_offset(new_data, new_cap);
    new_attrs = qn_json_arr_attribute_offset(new_data, new_cap);

    if (direct == QN_JSON_ARR_PUSHING) {
        memcpy(&new_vars[arr->begin], &vars[arr->begin], sizeof(qn_json_variant_un) * arr->cnt);
        memcpy(&new_attrs[arr->begin], &attrs[arr->begin], sizeof(qn_json_attribute_st) * arr->cnt);
    } else {
        offset = new_cap - arr->cap;
        memcpy(&new_vars[arr->begin + offset], &vars[arr->begin], sizeof(qn_json_variant_un) * arr->cnt);
        memcpy(&new_attrs[arr->begin + offset], &attrs[arr->begin], sizeof(qn_json_attribute_st) * arr->cnt);
        arr->begin += offset;
        arr->end += offset;
    } // if

    if (arr->data != &arr->values[0]) free(arr->data);

    arr->data = new_data;
    arr->cap = new_cap;
    return qn_true;
}

static qn_bool qn_json_arr_push_variant(qn_json_array_ptr restrict arr, qn_json_type type, qn_json_variant_un new_var)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(arr);

    if ((arr->end == arr->cap) && ! qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;

    vars = qn_json_arr_variant_offset(arr->data, arr->cap);
    attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);

    vars[arr->end] = new_var;
    attrs[arr->end].type = type;
    arr->end += 1;
    arr->cnt += 1;
    return qn_true;
}

static qn_bool qn_json_arr_unshift_variant(qn_json_array_ptr restrict arr, qn_json_type type, qn_json_variant_un new_var)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    if ((arr->begin == 0) && ! qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;

    vars = qn_json_arr_variant_offset(arr->data, arr->cap);
    attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);

    arr->begin -= 1;
    vars[arr->begin] = new_var;
    attrs[arr->begin].type = type;
    arr->cnt += 1;
    return qn_true;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Destruct and deallocate a JSON array.
*
* @param [in] arr The pointer to the array to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_arr_destroy(qn_json_array_ptr restrict arr)
{
    qn_uint16 i = 0;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    if (arr) {
        vars = qn_json_arr_variant_offset(arr->data, arr->cnt);
        attrs = qn_json_arr_attribute_offset(arr->data, arr->cnt);

        for (i = arr->begin; i < arr->end; i += 1) {
            qn_json_destroy_variant(attrs[i].type, &vars[i]);
        } // for
        if (arr->data != &arr->values[0]) free(arr->data);
        free(arr);
    } // if
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Return the current quantity of values of the array.
*
* @param [in] obj The non-NULL pointer to the array.
* @retval Integer-Value The current quantity of values of the array.
*******************************************************************************/
QN_SDK qn_uint16 qn_json_arr_size(qn_json_array_ptr restrict arr)
{
    assert(arr);
    return arr->cnt;
}

/* == Set & Get methods == */

static qn_json_variant_ptr qn_json_arr_get_variant(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_type type)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(arr);

    if (arr->cnt == 0 || arr->cnt <= n) {
        qn_err_json_set_out_of_index();
        return NULL;
    }
    if ((qn_uint)arr->cnt + n > 0xFFFF) {
        qn_err_json_set_out_of_index();
        return NULL;
    }

    attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);
    if (attrs[arr->begin + n].type != type) {
        qn_err_json_set_not_this_type();
        return NULL;
    }
    vars = qn_json_arr_variant_offset(arr->data, arr->cap);
    return &vars[arr->begin + n];
}

QN_SDK qn_bool qn_json_arr_get_object(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_object_ptr restrict * val)
{
    qn_json_variant_ptr var = qn_json_arr_get_variant(arr, n, QN_JSON_OBJECT);
    if (! var) return qn_false;
    *val = var->object;
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_get_array(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_array_ptr restrict * val)
{
    qn_json_variant_ptr var = qn_json_arr_get_variant(arr, n, QN_JSON_ARRAY);
    if (! var) return qn_false;
    *val = var->array;
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_get_string(qn_json_array_ptr restrict arr, qn_uint16 n, qn_string restrict * val)
{
    qn_json_variant_ptr var = qn_json_arr_get_variant(arr, n, QN_JSON_STRING);
    if (! var) return qn_false;
#if defined(QN_CFG_SUPPORT_MULTITHREAD)
    *val = qn_str_duplicate(var->string);
#else
    *val = var->string;
#endif
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_get_integer(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_integer * val)
{
    qn_json_variant_ptr var = qn_json_arr_get_variant(arr, n, QN_JSON_INTEGER);
    if (! var) return qn_false;
    *val = var->integer;
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_get_number(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_number * val)
{
    qn_json_variant_ptr var = qn_json_arr_get_variant(arr, n, QN_JSON_NUMBER);
    if (! var) return qn_false;
    *val = var->number;
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_get_boolean(qn_json_array_ptr restrict arr, qn_uint16 n, qn_bool * val)
{
    qn_json_variant_ptr var = qn_json_arr_get_variant(arr, n, QN_JSON_BOOLEAN);
    if (! var) return qn_false;
    *val = var->boolean;
    return qn_true;
}

/* ==== */

QN_SDK qn_bool qn_json_arr_push_object(qn_json_array_ptr restrict arr, qn_json_object_ptr restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.object = val;
    return qn_json_arr_push_variant(arr, QN_JSON_OBJECT, new_var);
}

QN_SDK qn_bool qn_json_arr_push_array(qn_json_array_ptr restrict arr, qn_json_array_ptr restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.array = val;
    return qn_json_arr_push_variant(arr, QN_JSON_ARRAY, new_var);
}

QN_SDK qn_bool qn_json_arr_push_string(qn_json_array_ptr restrict arr, qn_string restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.string = val;
    return qn_json_arr_push_variant(arr, QN_JSON_STRING, new_var);
}

QN_SDK qn_bool qn_json_arr_push_integer(qn_json_array_ptr restrict arr, qn_json_integer val)
{
    qn_json_variant_un new_var;
    new_var.integer = val;
    return qn_json_arr_push_variant(arr, QN_JSON_INTEGER, new_var);
}

QN_SDK qn_bool qn_json_arr_push_number(qn_json_array_ptr restrict arr, qn_json_number val)
{
    qn_json_variant_un new_var;
    new_var.number = val;
    return qn_json_arr_push_variant(arr, QN_JSON_NUMBER, new_var);
}

QN_SDK qn_bool qn_json_arr_push_boolean(qn_json_array_ptr restrict arr, qn_bool val)
{
    qn_json_variant_un new_var;
    new_var.boolean = val;
    return qn_json_arr_push_variant(arr, QN_JSON_BOOLEAN, new_var);
}

QN_SDK qn_bool qn_json_arr_push_null(qn_json_array_ptr restrict arr)
{
    qn_json_variant_un new_var;
    new_var.integer = 0;
    return qn_json_arr_push_variant(arr, QN_JSON_NULL, new_var);
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Pop and destroy the tail element of the array.
*
* @param [in] arr The non-NULL pointer to the target array.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_arr_pop(qn_json_array_ptr restrict arr)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(arr);

    if (arr->cnt > 0) {
        vars = qn_json_arr_variant_offset(arr->data, arr->cap);
        attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);

        arr->end -= 1;
        qn_json_destroy_variant(attrs[arr->end].type, &vars[arr->end]);
        arr->cnt -= 1;
    } // if
}

/* ==== */

QN_SDK qn_bool qn_json_arr_unshift_object(qn_json_array_ptr restrict arr, qn_json_object_ptr restrict val)
{
    qn_json_variant_un new_var;

    assert(arr);
    assert(val);

    new_var.object = val;
    return qn_json_arr_unshift_variant(arr, QN_JSON_OBJECT, new_var);
}

QN_SDK qn_bool qn_json_arr_unshift_array(qn_json_array_ptr restrict arr, qn_json_array_ptr restrict val)
{
    qn_json_variant_un new_var;

    assert(arr);
    assert(val);

    new_var.array = val;
    return qn_json_arr_unshift_variant(arr, QN_JSON_ARRAY, new_var);
}

QN_SDK qn_bool qn_json_arr_unshift_string(qn_json_array_ptr restrict arr, qn_string restrict val)
{
    qn_json_variant_un new_var;

    assert(arr);
    assert(val);

    new_var.string = val;
    return qn_json_arr_unshift_variant(arr, QN_JSON_STRING, new_var);
}

QN_SDK qn_bool qn_json_arr_unshift_integer(qn_json_array_ptr restrict arr, qn_json_integer val)
{
    qn_json_variant_un new_var;

    assert(arr);

    new_var.integer = val;
    return qn_json_arr_unshift_variant(arr, QN_JSON_INTEGER, new_var);
}

QN_SDK qn_bool qn_json_arr_unshift_number(qn_json_array_ptr restrict arr, qn_json_number val)
{
    qn_json_variant_un new_var;

    assert(arr);

    new_var.number = val;
    return qn_json_arr_unshift_variant(arr, QN_JSON_NUMBER, new_var);
}

QN_SDK qn_bool qn_json_arr_unshift_boolean(qn_json_array_ptr restrict arr, qn_bool val)
{
    qn_json_variant_un new_var;

    assert(arr);

    new_var.boolean = val;
    return qn_json_arr_unshift_variant(arr, QN_JSON_BOOLEAN, new_var);
}

QN_SDK qn_bool qn_json_arr_unshift_null(qn_json_array_ptr restrict arr)
{
    qn_json_variant_un new_var;

    assert(arr);

    new_var.integer = 0;
    return qn_json_arr_unshift_variant(arr, QN_JSON_NULL, new_var);
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Shift and destroy the head element of the array.
*
* @param [in] arr The non-NULL pointer to the target array.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_arr_shift(qn_json_array_ptr restrict arr)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(arr);

    if (arr->cnt > 0) {
        vars = qn_json_arr_variant_offset(arr->data, arr->cap);
        attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);

        qn_json_destroy_variant(attrs[arr->begin].type, &vars[arr->begin]);
        arr->begin += 1;
        arr->cnt -= 1;
    } // if
}

/* ==== */

static qn_bool qn_json_arr_replace_variant(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_type type, qn_json_variant_un new_var)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(arr);

    if (arr->cnt == 0 || arr->cnt <= n) {
        qn_err_json_set_out_of_index();
        return qn_false;
    }
    if ((qn_uint)arr->cnt + n > 0xFFFF) {
        qn_err_json_set_out_of_index();
        return qn_false;
    }

    vars = qn_json_arr_variant_offset(arr->data, arr->cap);
    attrs = qn_json_arr_attribute_offset(arr->data, arr->cap);

    qn_json_destroy_variant(attrs[arr->begin + n].type, &vars[arr->begin + n]);
    vars[arr->begin + n] = new_var;
    attrs[arr->begin + n].type = type;
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_replace_object(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_object_ptr restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.object = val;
    return qn_json_arr_replace_variant(arr, n, QN_JSON_OBJECT, new_var);
}

QN_SDK qn_bool qn_json_arr_replace_array(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_array_ptr restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    new_var.array = val;
    return qn_json_arr_replace_variant(arr, n, QN_JSON_ARRAY, new_var);
}

QN_SDK qn_bool qn_json_arr_replace_string(qn_json_array_ptr restrict arr, qn_uint16 n, qn_string restrict val)
{
    qn_json_variant_un new_var;
    assert(val);
    if (! (new_var.string = qn_str_duplicate(val))) return qn_false;
    if (! qn_json_arr_replace_variant(arr, n, QN_JSON_STRING, new_var)) {
        qn_str_destroy(new_var.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_arr_replace_integer(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_integer val)
{
    qn_json_variant_un new_var;
    new_var.integer = val;
    return qn_json_arr_replace_variant(arr, n, QN_JSON_INTEGER, new_var);
}

QN_SDK qn_bool qn_json_arr_replace_number(qn_json_array_ptr restrict arr, qn_uint16 n, qn_json_number val)
{
    qn_json_variant_un new_var;
    new_var.number = val;
    return qn_json_arr_replace_variant(arr, n, QN_JSON_NUMBER, new_var);
}

QN_SDK qn_bool qn_json_arr_replace_boolean(qn_json_array_ptr restrict arr, qn_uint16 n, qn_bool val)
{
    qn_json_variant_un new_var;
    new_var.boolean = val;
    return qn_json_arr_replace_variant(arr, n, QN_JSON_BOOLEAN, new_var);
}

QN_SDK qn_bool qn_json_arr_replace_null(qn_json_array_ptr restrict arr, qn_uint16 n)
{
    qn_json_variant_un new_var;
    new_var.integer = 0;
    return qn_json_arr_replace_variant(arr, n, QN_JSON_NULL, new_var);
}

/* == Check methods == */

QN_SDK qn_json_type qn_json_arr_get_type(qn_json_array_ptr restrict arr, qn_uint16 n)
{
    assert(arr);

    if (arr->cnt == 0 || arr->cnt <= n) {
        qn_err_json_set_out_of_index();
        return QN_JSON_UNKNOWN;
    }
    if ((qn_uint)arr->cnt + n > 0xFFFF) {
        qn_err_json_set_out_of_index();
        return QN_JSON_UNKNOWN;
    }

    return (qn_json_arr_attribute_offset(arr->data, arr->cap))[arr->begin + n].type;
}

// ---- Inplementation of iterator of JSON ----

typedef struct _QN_JSON_ITR_LEVEL
{
    qn_uint32 pos;
    qn_json_type type;
    qn_json_variant_un parent;
    qn_uint32 status;
} qn_json_itr_level, *qn_json_itr_level_ptr;

typedef struct _QN_JSON_ITERATOR
{
    int cnt;
    int cap;
    qn_json_itr_level * lvl;
    qn_json_itr_level init_lvl[3];
} qn_json_iterator;

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Allocate and construct a new JSON iterator.
*
* @retval non-NULL A pointer to the new iterator.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_iterator_ptr qn_json_itr_create(void)
{
    qn_json_iterator_ptr new_itr = calloc(1, sizeof(qn_json_iterator));
    if (!new_itr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_itr->cap = sizeof(new_itr->init_lvl) / sizeof(new_itr->init_lvl[0]);
    new_itr->lvl = &new_itr->init_lvl[0];
    return new_itr;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Destruct and deallocate a JSON iterator.
*
* @param [in] itr The pointer to the iterator to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_itr_destroy(qn_json_iterator_ptr restrict itr)
{
    if (itr) {
        if (itr->lvl != &itr->init_lvl[0]) {
            free(itr->lvl);
        } // if
        free(itr);
    } // if
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Reset the given JSON iterator for next iteration.
*
* @param [in] itr The pointer to the iterator to reset.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_itr_reset(qn_json_iterator_ptr restrict itr)
{
    itr->cnt = 0;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Rewind the current level for a new iteration.
*
* @param [in] itr The pointer to the iterator to rewind.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_itr_rewind(qn_json_iterator_ptr restrict itr)
{
    if (itr->cnt <= 0) return;
    itr->lvl[itr->cnt - 1].pos = 0;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Test the given iterator whether it is in use.
*
* @param [in] itr The pointer to the iterator to test.
* @retval true The iterator is not in use.
* @retval false The iterator is in use.
*******************************************************************************/
QN_SDK qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr restrict itr)
{
    return itr->cnt == 0;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Get the count that how many pairs or values of the current level has been
* iterated.
*
* @param [in] itr The pointer to the iterator.
* @retval ANY The count of iterated pairs or values of the current level.
*******************************************************************************/
QN_SDK int qn_json_itr_done_steps(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0) ? 0 : itr->lvl[itr->cnt - 1].pos;
}

QN_SDK qn_string qn_json_itr_get_key(qn_json_iterator_ptr restrict itr)
{
    qn_string * keys = NULL;
    qn_json_itr_level_ptr lvl = NULL;

    if (itr->cnt == 0) return NULL;
    lvl = &itr->lvl[itr->cnt - 1];

    if (lvl->type != QN_JSON_OBJECT) return NULL;

    keys = qn_json_obj_key_offset(lvl->parent.object->data, lvl->parent.object->cap);
    return keys[lvl->pos - 1];
}

QN_SDK void qn_json_itr_set_status(qn_json_iterator_ptr restrict itr, qn_uint32 sts)
{
    if (itr->cnt) itr->lvl[itr->cnt - 1].status = sts;
}

QN_SDK qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt == 0) ? 0 : itr->lvl[itr->cnt - 1].status;
}

static qn_bool qn_json_itr_augment_levels(qn_json_iterator_ptr restrict itr)
{
    int new_capacity = itr->cap + (itr->cap >> 1); // 1.5 times of the last stack capacity.
    qn_json_itr_level_ptr new_lvl = calloc(1, sizeof(qn_json_itr_level) * new_capacity);
    if (!new_lvl) {
        qn_err_set_out_of_memory();
        return qn_false;
    }  // if

    memcpy(new_lvl, itr->lvl, sizeof(qn_json_itr_level) * itr->cnt);
    if (itr->lvl != &itr->init_lvl[0]) free(itr->lvl);
    itr->lvl = new_lvl;
    itr->cap = new_capacity;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr_push_object(qn_json_iterator_ptr restrict itr, qn_json_object_ptr restrict obj)
{
    if ((itr->cnt + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->cnt].type = QN_JSON_OBJECT;
    itr->lvl[itr->cnt].parent.object = obj;
    itr->cnt += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

QN_SDK qn_bool qn_json_itr_push_array(qn_json_iterator_ptr restrict itr, qn_json_array_ptr restrict arr)
{
    if ((itr->cnt + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->cnt].type = QN_JSON_ARRAY;
    itr->lvl[itr->cnt].parent.array = arr;
    itr->cnt += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

QN_SDK void qn_json_itr_pop(qn_json_iterator_ptr restrict itr)
{
    if (itr->cnt > 0) itr->cnt -= 1;
}

QN_SDK qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0 || itr->lvl[itr->cnt - 1].type != QN_JSON_OBJECT) ? NULL : itr->lvl[itr->cnt - 1].parent.object;
}

QN_SDK qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0 || itr->lvl[itr->cnt - 1].type != QN_JSON_ARRAY) ? NULL : itr->lvl[itr->cnt - 1].parent.array;
}

QN_SDK int qn_json_itr_advance(qn_json_iterator_ptr restrict itr, void * data, qn_json_itr_callback_fn cb)
{
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;
    qn_json_type type = QN_JSON_UNKNOWN;
    qn_json_itr_level_ptr lvl = NULL;
    qn_json_variant_un var;

    if (itr->cnt <= 0) return QN_JSON_ITR_NO_MORE;

    lvl = &itr->lvl[itr->cnt - 1];
    if (lvl->type == QN_JSON_OBJECT) {
        if (lvl->pos < lvl->parent.object->cnt) {
            vars = qn_json_obj_variant_offset(lvl->parent.object->data, lvl->parent.object->cap);
            attrs = qn_json_obj_attribute_offset(lvl->parent.object->data, lvl->parent.object->cap);

            type = attrs[lvl->pos].type;
            var = vars[lvl->pos];
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        } // if
    } else {
        if (lvl->pos < lvl->parent.array->cnt) {
            vars = qn_json_arr_variant_offset(lvl->parent.array->data, lvl->parent.array->cap);
            attrs = qn_json_arr_attribute_offset(lvl->parent.array->data, lvl->parent.array->cap);

            type = attrs[lvl->pos + lvl->parent.array->begin].type;
            var = vars[lvl->pos + lvl->parent.array->begin];
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        } // if
    } // if
    return cb(data, type, &var);
}

/* == Redesign == */

#define QN_JSON_ITR_DEFAULT_CAPACITY (4)

typedef struct _QN_JSON_ITERATOR2
{
    qn_uint16 cnt;
    qn_uint16 cap;
    void * data;
} qn_json_iterator2_st;

typedef union _QN_JSON_ITR2_VARIANT
{
    qn_json_object_ptr object;
    qn_json_array_ptr array;
} qn_json_itr2_variant_st, *qn_json_itr2_variant_ptr;

typedef qn_uint8 qn_json_itr2_type;

#define qn_json_itr2_variant_offset(ptr, cap) ((qn_json_itr2_variant_ptr) ptr)
#define qn_json_itr2_position_offset(ptr, cap) ((qn_uint16 *) ((char *) ptr + sizeof(qn_json_itr2_variant_st) * cap))
#define qn_json_itr2_type_offset(ptr, cap) ((qn_json_itr2_type *) ((char *) ptr + sizeof(qn_json_itr2_variant_st) * cap + sizeof(qn_uint16) * cap))

static inline size_t qn_json_itr2_calculate_data_size(qn_uint16 cap)
{
    return (sizeof(qn_json_itr2_variant_st) + sizeof(qn_uint16) + sizeof(qn_json_itr2_type)) * cap;
}

QN_SDK qn_json_iterator2_ptr qn_json_itr2_create(void)
{
    qn_json_iterator2_ptr new_itr = calloc(1, sizeof(qn_json_iterator2_st) + qn_json_itr2_calculate_data_size(QN_JSON_ITR_DEFAULT_CAPACITY));
    if (! new_itr) {
        qn_err_set_out_of_memory();
        return NULL;
    }

    new_itr->cap = QN_JSON_ITR_DEFAULT_CAPACITY;
    new_itr->data = new_itr + sizeof(qn_json_iterator2_st);
    return new_itr;
}

QN_SDK void qn_json_itr2_destroy(qn_json_iterator2_ptr restrict itr)
{
    if (itr) {
        if (itr->data != (itr + sizeof(qn_json_iterator2_st))) free(itr->data);
        free(itr);
    }
}

static inline qn_bool qn_json_itr2_augment(qn_json_iterator2_ptr restrict itr)
{
    qn_uint16 new_cap = qn_json_round_to_multiple_of_8(itr->cap + (itr->cap >> 1));
    void * new_data = calloc(1, qn_json_itr2_calculate_data_size(new_cap));
    if (! new_data) {
        qn_err_set_out_of_memory();
        return qn_false;
    }

    memcpy(qn_json_itr2_variant_offset(new_data, new_cap), qn_json_itr2_variant_offset(itr->data, itr->cap), sizeof(qn_json_itr2_variant_st) * itr->cnt);
    memcpy(qn_json_itr2_position_offset(new_data, new_cap), qn_json_itr2_position_offset(itr->data, itr->cap), sizeof(qn_uint16) * itr->cnt);
    memcpy(qn_json_itr2_type_offset(new_data, new_cap), qn_json_itr2_type_offset(itr->data, itr->cap), sizeof(qn_json_itr2_type) * itr->cnt);

    if (itr->data != (itr + sizeof(qn_json_iterator2_st))) free(itr->data);
    itr->data = new_data;
    itr->cap = new_cap;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_push_object(qn_json_iterator2_ptr restrict itr, qn_json_object_ptr restrict obj)
{
    assert(itr);
    assert(obj);

    if (itr->cnt == itr->cap && ! qn_json_itr2_augment(itr)) return qn_false;
    (qn_json_itr2_variant_offset(itr->data, itr->cap))[itr->cnt].object = obj;
    (qn_json_itr2_position_offset(itr->data, itr->cap))[itr->cnt] = 0;
    (qn_json_itr2_type_offset(itr->data, itr->cap))[itr->cnt] = QN_JSON_OBJECT;
    itr->cnt += 1;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_push_array(qn_json_iterator2_ptr restrict itr, qn_json_array_ptr restrict arr)
{
    assert(itr);
    assert(arr);

    if (itr->cnt == itr->cap && ! qn_json_itr2_augment(itr)) return qn_false;
    (qn_json_itr2_variant_offset(itr->data, itr->cap))[itr->cnt].array = arr;
    (qn_json_itr2_position_offset(itr->data, itr->cap))[itr->cnt] = 0;
    (qn_json_itr2_type_offset(itr->data, itr->cap))[itr->cnt] = QN_JSON_ARRAY;
    itr->cnt += 1;
    return qn_true;
}

QN_SDK void qn_json_itr2_pop(qn_json_iterator2_ptr restrict itr)
{
    assert(itr);
    if (itr->cnt > 0) itr->cnt -= 1;
}

QN_SDK void qn_json_itr2_pop_all(qn_json_iterator2_ptr restrict itr)
{
    assert(itr);
    itr->cnt = 0;
}

QN_SDK qn_bool qn_json_itr2_has_next_entry(qn_json_iterator2_ptr restrict itr)
{
    assert(itr);
    if ((qn_json_itr2_type_offset(itr->data, itr->cap))[itr->cnt] == QN_JSON_OBJECT) {
        return (qn_json_itr2_position_offset(itr->data, itr->cap))[itr->cnt] < qn_json_obj_size((qn_json_itr2_variant_offset(itr->data, itr->cap))[itr->cnt].object);
    }
    return (qn_json_itr2_position_offset(itr->data, itr->cap))[itr->cnt] < qn_json_arr_size((qn_json_itr2_variant_offset(itr->data, itr->cap))[itr->cnt].array);
}

QN_SDK void qn_json_itr2_advance(qn_json_iterator2_ptr restrict itr)
{
    assert(itr);
    itr->cnt += 1;
}

static inline qn_bool qn_json_itr2_get_variant(qn_json_iterator2_ptr restrict itr, qn_json_type type, qn_json_variant_ptr restrict jvar, qn_string * restrict key)
{
    qn_json_itr2_variant_st var = (qn_json_itr2_variant_offset(itr->data, itr->cap))[itr->cnt]; 
    qn_uint16 pos = qn_json_itr2_position_offset(itr->data, itr->cap)[itr->cnt]; 

    if ((qn_json_itr2_type_offset(itr->data, itr->cap))[itr->cnt] == QN_JSON_OBJECT) {
        if ((qn_json_obj_attribute_offset(var.object->data, var.object->cap))[pos].type != type) {
            qn_err_json_set_not_this_type();
            return qn_false;
        }
        *jvar = (qn_json_obj_variant_offset(var.object->data, var.object->cap))[pos];

        if (key) {
#if defined(QN_CFG_SUPPORT_MULTITHREAD)
            *key = qn_str_duplicate((qn_json_obj_key_offset(var.object->data, var.object->cap))[pos]);
            if (! *key) return qn_false;
#else
            *key = (qn_json_obj_key_offset(var.object->data, var.object->cap))[pos];
#endif
        }
    } else {
        if ((qn_json_arr_attribute_offset(var.array->data, var.array->cap))[pos].type != type) {
            qn_err_json_set_not_this_type();
            return qn_false;
        }
        *jvar = (qn_json_arr_variant_offset(var.array->data, var.array->cap))[pos];

        if (key) *key = NULL;
    } /* if */
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_get_object(qn_json_iterator2_ptr restrict itr, qn_string * restrict key, qn_json_object_ptr * restrict val)
{
    qn_json_variant_un var;

    assert(itr);
    assert(val);

    if (! qn_json_itr2_get_variant(itr, QN_JSON_OBJECT, &var, key)) return qn_false;
    *val = var.object;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_get_array(qn_json_iterator2_ptr restrict itr, qn_string * restrict key, qn_json_array_ptr * restrict val)
{
    qn_json_variant_un var;

    assert(itr);
    assert(val);

    if (! qn_json_itr2_get_variant(itr, QN_JSON_ARRAY, &var, key)) return qn_false;
    *val = var.array;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_get_string(qn_json_iterator2_ptr restrict itr, qn_string * restrict key, qn_string * restrict val)
{
    qn_json_variant_un var;

    assert(itr);
    assert(val);

    if (! qn_json_itr2_get_variant(itr, QN_JSON_STRING, &var, key)) return qn_false;
#if defined(QN_CFG_SUPPORT_MULTITHREAD)
    *val = qn_str_duplicate(var.string);
    if (! *val) return qn_false;
#else
    *val = var.string;
#endif
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_get_integer(qn_json_iterator2_ptr restrict itr, qn_string * restrict key, qn_json_integer * restrict val)
{
    qn_json_variant_un var;

    assert(itr);
    assert(val);

    if (! qn_json_itr2_get_variant(itr, QN_JSON_INTEGER, &var, key)) return qn_false;
    *val = var.integer;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_get_number(qn_json_iterator2_ptr restrict itr, qn_string * restrict key, qn_json_number * restrict val)
{
    qn_json_variant_un var;

    assert(itr);
    assert(val);

    if (! qn_json_itr2_get_variant(itr, QN_JSON_NUMBER, &var, key)) return qn_false;
    *val = var.number;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr2_get_null(qn_json_iterator2_ptr restrict itr, qn_string * restrict key)
{
    qn_json_variant_un var;

    assert(itr);

    if (! qn_json_itr2_get_variant(itr, QN_JSON_NULL, &var, key)) return qn_false;
    return qn_true;
}

QN_SDK qn_json_type qn_json_itr2_get_type(qn_json_iterator2_ptr restrict itr)
{
    qn_json_itr2_variant_st var = (qn_json_itr2_variant_offset(itr->data, itr->cap))[itr->cnt]; 
    qn_uint16 pos = qn_json_itr2_position_offset(itr->data, itr->cap)[itr->cnt]; 

    if ((qn_json_itr2_type_offset(itr->data, itr->cap))[itr->cnt] == QN_JSON_OBJECT) {
        return (qn_json_obj_attribute_offset(var.object->data, var.object->cap))[pos].type;
    }
    return (qn_json_arr_attribute_offset(var.array->data, var.array->cap))[pos].type;
}

#ifdef __cplusplus
}
#endif
