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

typedef union _QN_JSON_VARIANT
{
    qn_json_object_ptr object;
    qn_json_array_ptr array;
    qn_json_string string;
    qn_json_integer integer;
    qn_json_number number;
    qn_json_boolean boolean;
} qn_json_variant_un, *qn_json_variant_ptr;

typedef qn_uint32 qn_json_hash;
typedef unsigned short int qn_uint32;

static inline void qn_json_destroy_variant(qn_json_type type, qn_json_variant_ptr restrict var)
{
    switch (type) {
        case QN_JSON_OBJECT: qn_json_destroy_object(var->object); break;
        case QN_JSON_ARRAY: qn_json_destroy_array(var->array); break;
        case QN_JSON_STRING: qn_str_destroy(var->string); break;
        default: break;
    } // switch
}

typedef _QN_JSON_ATTRIBUTE
{
    unsigned char type:4;
} qn_json_attribute_st, *qn_json_attribute_ptr;

#define qn_json_variant_offset(ptr, cap) ((qn_json_variant_ptr) ((char *) ptr + sizeof(qn_string) * cap))
#define qn_json_attribute_offset(ptr, cap) ((qn_json_attribute_ptr) ((char *) qn_json_variant_offset(ptr, cap) + sizeof(qn_json_variant_un) * cap))

/* ==== Definition of JSON Object ==== */

#define QN_JSON_DEFAULT_CAPACITY 8

typedef struct _QN_JSON_OBJECT
{
    void * data;
    qn_uint16 cnt;
    qn_uint16 cap;

    qn_string keys[QN_JSON_DEFAULT_CAPACITY];
    qn_json_variant_un values[QN_JSON_DEFAULT_CAPACITY];
    qn_json_attribute_st attributes[QN_JSON_DEFAULT_CAPACITY];
} qn_json_object_st;

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
QN_SDK qn_json_object_ptr qn_json_create_object(void)
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
QN_SDK void qn_json_destroy_object(qn_json_object_ptr restrict obj)
{
    qn_uint32 i = 0;
    qn_json_variant_ptr var = NULL;
    qn_json_attribute_ptr attr = NULL;
    
    if (obj) {
        var = qn_json_variant_offset(obj->data, obj->cap);
        attr = qn_json_attribute_offset(obj->data, obj->cap);

        for (i = 0; i < obj->cnt; i += 1) {
            qn_json_destroy_variant(attr[i].type, &var[i]);
            qn_str_destroy((qn_string) obj->data[i]);
        } // for
        if (obj->data != &obj->keys[0]) {
            free(obj->data);
        } // if
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
    qn_uint32 new_cap = obj->cap + (obj->cap >> 1); /* 1.5 times */
    qn_uint32 new_size = (sizeof(qn_string) + sizeof(qn_json_variant_un) + sizeof(qn_json_attribute_st)) * new_cap;
    qn_string * new_data = calloc(1, new_size);
    if (! new_data) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_data, obj->data, sizeof(qn_string) * obj->cnt);
    memcpy(qn_json_variant_offset(new_data, new_cap), qn_json_variant_offset(obj->data, obj->cap), sizeof(qn_json_variant_un) * obj->cnt);
    memcpy(qn_json_attribute_offset(new_data, new_cap), qn_json_attribute_offset(obj->data, obj->cap), sizeof(qn_json_attribute_st) * obj->cnt);

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

    keys = (qn_string *) obj->data;
    vars = qn_json_variant_offset(obj->data, obj->cap);
    attrs = qn_json_attribute_offset(obj->data, obj->cap);

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

/***************************************************************************//**
* @ingroup JSON-Object
*
* Create a new object and then set it as an element into the target object.
*
* @param [in] obj The non-NULL pointer to the target object.
* @param [in] key The key of the new object.
* @retval non-NULL The pointer to the new object.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant_un new_var;

    assert(obj);
    assert(key);

    new_var.object = qn_json_create_object();
    if (new_var.object && ! qn_json_obj_set_variant(obj, key, QN_JSON_OBJECT, new_var)) {
        qn_json_destroy_object(new_var.object);
        return NULL;
    } // if
    return new_var.object;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Create a new array and then set it as an element into the target object.
*
* @param [in] obj The non-NULL pointer to the target object.
* @param [in] key The key of the new array.
* @retval non-NULL The pointer to the new array.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant_un new_var;

    assert(obj);
    assert(key);

    new_var.array = qn_json_create_array();
    if (new_var.array && ! qn_json_obj_set_variant(obj, key, QN_JSON_ARRAY, new_var)) {
        qn_json_destroy_array(new_var.array);
        return NULL;
    } // if
    return new_var.array;
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
QN_SDK qn_uint32 qn_json_object_size(qn_json_object_ptr restrict obj)
{
    assert(obj);
    return obj->cnt;
}

/* == Set & Get methods == */

static qn_json_variant_ptr qn_json_obj_get_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_type type)
{
    int existence = 0;
    qn_uint32 pos = 0;
    if (obj->cnt == 0) return NULL;
    pos = qn_json_bsearch_key((qn_string *) obj->data, obj->cnt, key, &existence);
    return (existence != 0 || obj->itm[pos].type != type) ? NULL : &obj->itm[pos].elem;
}

QN_SDK qn_json_object_ptr qn_json_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_OBJECT))) ? var->object : default_val;
}

QN_SDK qn_json_array_ptr qn_json_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_ARRAY))) ? var->array : default_val;
}

QN_SDK qn_string qn_json_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_STRING))) ? var->string : default_val;
}

QN_SDK const char * qn_json_get_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_STRING))) ? qn_str_cstr(var->string) : default_val;
}

QN_SDK qn_json_integer qn_json_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_INTEGER))) ? var->integer : default_val;
}

QN_SDK qn_json_number qn_json_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_NUMBER))) ? var->number : default_val;
}

QN_SDK qn_bool qn_json_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool default_val)
{
    qn_json_variant_ptr var = NULL;
    assert(obj);
    assert(key);
    return ((var = qn_json_obj_get_variant(obj, key, QN_JSON_BOOLEAN))) ? var->boolean : default_val;
}

QN_SDK qn_bool qn_json_set_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict val)
{
    qn_json_variant_un new_var;
    new_var.object = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_OBJECT, new_var);
}

/* ---- */

QN_SDK qn_bool qn_json_set_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict val)
{
    qn_json_variant_un new_var;
    new_var.array = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_ARRAY, new_var);
}

QN_SDK qn_bool qn_json_set_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict val)
{
    qn_json_variant_un new_var;
    if (! (new_var.string = qn_str_duplicate(val))) return qn_false;
    if (! qn_json_obj_set_variant(obj, key, QN_JSON_STRING, new_var)) {
        qn_str_destroy(new_var.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_set_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val)
{
    qn_json_variant_un new_var;
    if (! (new_var.string = qn_cs_duplicate(val))) return qn_false;
    if (! qn_json_obj_set_variant(obj, key, QN_JSON_STRING, new_var)) {
        qn_str_destroy(new_var.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_set_text(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val, qn_size size)
{
    qn_json_variant_un new_var;
    if (! (new_var.string = qn_cs_clone(val, size))) return qn_false;
    if (! qn_json_obj_set_variant(obj, key, QN_JSON_STRING, new_var)) {
        qn_str_destroy(new_var.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer val)
{
    qn_json_variant_un new_var;
    new_var.integer = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_INTEGER, new_var);
}

QN_SDK qn_bool qn_json_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number val)
{
    qn_json_variant_un new_var;
    new_var.number = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_NUMBER, new_var);
}

QN_SDK qn_bool qn_json_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val)
{
    qn_json_variant_un new_var;
    new_var.boolean = val;
    return qn_json_obj_set_variant(obj, key, QN_JSON_BOOLEAN, new_var);
}

QN_SDK qn_bool qn_json_set_null(qn_json_object_ptr restrict obj, const char * restrict key)
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
QN_SDK void qn_json_unset(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_uint32 pos = 0;
    int existence = 0;
    qn_string * keys = NULL;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(obj);
    assert(key);

    if (obj->cnt == 0) return;

    keys = (qn_string *) obj->data;

    pos = qn_json_bsearch_key(keys, obj->cnt, key, &existence);
    if (existence != 0) return; // There is no element corresponds to the key.

    vars = qn_json_variant_offset(obj->data, obj->cap);
    attrs = qn_json_attribute_offset(obj->data, obj->cap);

    qn_json_destroy_variant(attrs[pos].type, &vars[pos]);
    qn_str_destroy(keys[pos]);
    if (pos < obj->cnt - 1) qn_json_obj_move(keys, vars, attrs, pos, pos + 1, (obj->cnt - pos - 1));
    obj->cnt -= 1;
}

QN_SDK qn_bool qn_json_rename(qn_json_object_ptr restrict obj, const char * restrict old_key, const char * new_key)
{
    qn_uint32 old_pos = 0;
    qn_uint32 new_pos = 0;
    qn_string new_key_str = NULL;
    int existence = 0;
    qn_string * keys = NULL;
    qn_json_variant_ptr vars = NULL;
    qn_json_attribute_ptr attrs = NULL;

    assert(obj);
    assert(old_key);
    assert(new_key);

    if (obj->cnt == 0) {
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    if (strcmp(old_key, new_key) == 0) return qn_true; /* The old key is exactly the same to the new key. */

    keys = (qn_string *) obj->data;
    vars = qn_json_variant_offset(obj->data, obj->cap);
    attrs = qn_json_attribute_offset(obj->data, obj->cap);

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

// ---- Inplementation of array of JSON ----

typedef struct _QN_JSON_ARR_ITEM
{
    qn_json_type type;
    qn_json_variant_un elem;
} qn_json_arr_item;

typedef struct _QN_JSON_ARRAY
{
    qn_json_arr_item * itm;
    qn_uint32 begin;
    qn_uint32 end;
    qn_uint32 cnt;
    qn_uint32 cap;

    qn_json_arr_item init_itm[4];
} qn_json_array, *qn_json_array_ptr;

/***************************************************************************//**
* @ingroup JSON-Array
*
* Return a static and immutable empty array.
*
* @retval non-NULL A pointer to the that static and immutable empty array.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_immutable_empty_array(void)
{
    static qn_json_array arr;
    return &arr;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Allocate and construct a new JSON array.
*
* @retval non-NULL A pointer to the new JSON array.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_array(void)
{
    qn_json_array_ptr new_arr = calloc(1, sizeof(qn_json_array));
    if (!new_arr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_arr->itm = &new_arr->init_itm[0];
    new_arr->cap = sizeof(new_arr->init_itm) / sizeof(new_arr->init_itm[0]);
    return new_arr;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Destruct and deallocate a JSON array.
*
* @param [in] arr The pointer to the array to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_destroy_array(qn_json_array_ptr restrict arr)
{
    qn_uint32 i;

    for (i = arr->begin; i < arr->end; i += 1) {
        qn_json_destroy_variant(arr->itm[i].type, &arr->itm[i].elem);
    } // for
    if (arr->itm != &arr->init_itm[0]) free(arr->itm);
    free(arr);
}

enum
{
    QN_JSON_ARR_PUSHING = 0,
    QN_JSON_ARR_UNSHIFTING = 1
};

static qn_bool qn_json_arr_augment(qn_json_array_ptr restrict arr, int direct)
{
    qn_uint32 new_cap = arr->cap * 2;
    qn_json_arr_item * new_itm = calloc(1, sizeof(qn_json_arr_item) * new_cap);
    if (!new_itm) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    if (direct == QN_JSON_ARR_PUSHING) {
        memcpy(new_itm + arr->begin, arr->itm + arr->begin, sizeof(qn_json_arr_item) * arr->cnt);
    } else {
        memcpy(new_itm + arr->begin + arr->cap, arr->itm + arr->begin, sizeof(qn_json_arr_item) * arr->cnt);
        arr->begin += arr->cap;
        arr->end += arr->cap;
    } // if
    if (arr->itm != &arr->init_itm[0]) free(arr->itm);

    arr->itm = new_itm;
    arr->cap = new_cap;
    return qn_true;
}

static inline qn_uint32 qn_json_arr_find(qn_json_array_ptr restrict arr, int n)
{
    return (n < 0 || n >= arr->cnt) ? arr->cnt : (arr->begin + n);
}

// ---- Inplementation of JSON ----

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new object and then push it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new object.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr restrict arr)
{
    qn_json_variant_un new_var;
    new_var.object = qn_json_create_object();
    if (new_var.object && ! qn_json_push_variant(arr, QN_JSON_OBJECT, new_var)) {
        qn_json_destroy_object(new_var.object);
        return NULL;
    } // if
    return new_var.object;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new array and then push it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new array.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr restrict arr)
{
    qn_json_variant_un new_var;
    new_var.array = qn_json_create_array();
    if (new_var.array && ! qn_json_push_variant(arr, QN_JSON_ARRAY, new_var)) {
        qn_json_destroy_array(new_var.array);
        return NULL;
    } // if
    return new_var.array;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new object and then unshift it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new object.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr restrict arr)
{
    qn_json_variant_un new_var;
    new_var.object = qn_json_create_object();
    if (new_var.object && ! qn_json_unshift_variant(arr, QN_JSON_OBJECT, new_var)) {
        qn_json_destroy_object(new_var.object);
        return NULL;
    } // if
    return new_var.object;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new array and then unshift it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new array.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr restrict arr)
{
    qn_json_variant_un new_var;
    new_var.array = qn_json_create_array();
    if (new_var.array && ! qn_json_unshift_variant(arr, QN_JSON_ARRAY, new_var)) {
        qn_json_destroy_array(new_var.array);
        return NULL;
    } // if
    return new_var.array;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Return the current quantity of values of the array.
*
* @param [in] obj The non-NULL pointer to the array.
* @retval Integer-Value The current quantity of values of the array.
*******************************************************************************/
QN_SDK int qn_json_size_array(qn_json_array_ptr restrict arr)
{
    return arr->cnt;
}

QN_SDK qn_json_variant_ptr qn_json_pick_variant(qn_json_array_ptr restrict arr, int n, qn_json_type type)
{
    qn_uint32 pos;
    if (arr->cnt == 0) return NULL;
    pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].type != type) ? NULL : &arr->itm[pos].elem;
}

QN_SDK qn_bool qn_json_push_variant(qn_json_array_ptr restrict arr, qn_json_type type, qn_json_variant_un new_var)
{
    assert(arr);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if ((arr->end == arr->cap) && !qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;

    arr->itm[arr->end].elem = new_var;
    arr->itm[arr->end].type = type;
    arr->end += 1;
    arr->cnt += 1;
    return qn_true;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Pop and destroy the tail element of the array.
*
* @param [in] arr The non-NULL pointer to the target array.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_pop(qn_json_array_ptr restrict arr)
{
    if (arr->cnt > 0) {
        arr->end -= 1;
        qn_json_destroy_variant(arr->itm[arr->end].type, &arr->itm[arr->end].elem);
        arr->cnt -= 1;
    } // if
}

QN_SDK qn_bool qn_json_unshift_variant(qn_json_array_ptr restrict arr, qn_json_type type, qn_json_variant_un new_var)
{
    assert(arr);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if ((arr->begin == 0) && !qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;

    arr->begin -= 1;
    arr->itm[arr->begin].elem = new_var;
    arr->itm[arr->begin].type = type;
    arr->cnt += 1;
    return qn_true;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Shift and destroy the head element of the array.
*
* @param [in] arr The non-NULL pointer to the target array.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_shift(qn_json_array_ptr restrict arr)
{
    if (arr->cnt > 0) {
        qn_json_destroy_variant(arr->itm[arr->begin].type, &arr->itm[arr->begin].elem);
        arr->begin += 1;
        arr->cnt -= 1;
    } // if
}

QN_SDK qn_bool qn_json_replace_variant(qn_json_array_ptr restrict arr, int n, qn_json_type type, qn_json_variant_un new_var)
{
    assert(arr);
    assert(0 <= n);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if (n < 0 || (arr->end - arr->begin) <= n) {
        qn_err_json_set_out_of_index();
        return qn_false;
    } // if

    qn_json_destroy_variant(arr->itm[arr->begin + n].type, &arr->itm[arr->begin + n].elem);
    arr->itm[arr->begin + n].elem = new_var;
    arr->itm[arr->begin + n].type = type;
    return qn_true;
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
    qn_json_itr_level_ptr lvl = NULL;

    if (itr->cnt == 0) return NULL;
    lvl = &itr->lvl[itr->cnt - 1];

    if (lvl->type != QN_JSON_OBJECT) return NULL;
    return lvl->parent.object->itm[lvl->pos - 1].key;
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
    qn_json_type type;
    qn_json_variant_un elem;
    qn_json_itr_level_ptr lvl;

    if (itr->cnt <= 0) return QN_JSON_ITR_NO_MORE;

    lvl = &itr->lvl[itr->cnt - 1];
    if (lvl->type == QN_JSON_OBJECT) {
        if (lvl->pos < lvl->parent.object->cnt) {
            type = lvl->parent.object->itm[lvl->pos].type;
            elem = lvl->parent.object->itm[lvl->pos].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        } // if
    } else {
        if (lvl->pos < lvl->parent.array->cnt) {
            type = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].type;
            elem = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        } // if
    } // if
    return cb(data, type, &elem);
}

#ifdef __cplusplus
}
#endif
