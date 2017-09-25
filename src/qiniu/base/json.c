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
} qn_json_variant_st, *qn_json_variant_ptr;

typedef qn_uint32 qn_json_hash;
typedef unsigned short int qn_json_pos;

static inline void qn_json_destroy_variant(qn_json_type type, qn_json_variant_ptr restrict var)
{
    switch (type) {
        case QN_JSON_OBJECT: qn_json_destroy_object(var->object); break;
        case QN_JSON_ARRAY: qn_json_destroy_array(var->array); break;
        case QN_JSON_STRING: qn_str_destroy(var->string); break;
        default: break;
    } // switch
}

typedef _QN_JSON_TYPE_MAP
{
    struct {
        unsigned char first:4;
        unsigned char second:4;
    } bytes[4]; /* 32 bits can contains 8 JSON types */
} qn_json_type_map_st, *qn_json_type_map_ptr;

#define qn_json_variant_offset(ptr, cap) ((qn_json_variant_ptr) ((char *) ptr + sizeof(qn_string) * cap))
#define qn_json_type_map_offset(ptr, cap) ((qn_json_type_map *) ((char *) qn_json_variant_offset(ptr, cap) + sizeof(qn_json_variant_st) * cap))

#define QN_JSON_TYPE_MAP_CAPACITY (sizeof(qn_json_type_map_st) * 8)
#define qn_json_type_map_count(cnt) (((cnt * 4) - 4 + QN_JSON_TYPE_MAP_CAPACITY) / QN_JSON_TYPE_MAP_CAPACITY)

#define QN_JSON_TYPE_MAP_RATIO ((sizeof(qn_json_type_map_st) * 8) / 4)
#define qn_json_type_map_index(idx) (idx / QN_JSON_TYPE_MAP_RATIO)
#define qn_json_type_map_byte(idx) ((idx % QN_JSON_TYPE_MAP_RATIO) / 2)
#define qn_json_type_map_half(idx) ((idx % QN_JSON_TYPE_MAP_RATIO) % 2)

static qn_json_type qn_json_type_map_get(qn_json_type_map_ptr restrict map, qn_uint32 idx)
{
    qn_uint32 i = qn_json_type_map_index(idx);
    qn_uint32 b = qn_json_type_map_byte(idx);
    qn_uint32 h = qn_json_type_map_half(idx);
    return (h == 0) ? map[i].bytes[b].first : map[i].bytes[b].second;
}

static void qn_json_type_map_set(qn_json_type_map_ptr restrict map, qn_uint32 idx, qn_json_type type)
{
    qn_uint32 i = qn_json_type_map_index(idx);
    qn_uint32 b = qn_json_type_map_byte(idx);
    qn_uint32 h = qn_json_type_map_half(idx);
    if (h == 0) {
        map[i].bytes[b].first = type;
    } else {
        map[i].bytes[b].second = type;
    } // if
}

static void qn_json_type_map_swap(qn_json_type_map_ptr restrict map, qn_uint32 lhs_idx, qn_uint32 rhs_idx)
{
    qn_uint32 lhs_i = qn_json_type_map_index(lhs_idx);
    qn_uint32 lhs_b = qn_json_type_map_byte(lhs_idx);
    qn_uint32 lhs_h = qn_json_type_map_half(lhs_idx);
    qn_json_type lhs_type = (lhs_h == 0) ? map[lhs_i].bytes[lhs_b].first : map[lhs_i].bytes[lhs_b].second;

    qn_uint32 rhs_i = qn_json_type_map_index(rhs_idx);
    qn_uint32 rhs_b = qn_json_type_map_byte(rhs_idx);
    qn_uint32 rhs_h = qn_json_type_map_half(rhs_idx);
    qn_json_type rhs_type = (rhs_h == 0) ? map[rhs_i].bytes[rhs_b].first : map[rhs_i].bytes[rhs_b].second;

    if (lhs_h == 0) {
        map[lhs_i].bytes[lhs_b].first = rhs_type;
    } else {
        map[lhs_i].bytes[lhs_b].second = rhs_type;
    } // if

    if (rhs_h == 0) {
        map[rhs_i].bytes[rhs_b].first = lhs_type;
    } else {
        map[rhs_i].bytes[rhs_b].second = lhs_type;
    } // if
}

/* ==== Definition of JSON Object ==== */

#define QN_JSON_DEFAULT_CAPACITY 8

typedef struct _QN_JSON_OBJECT
{
    void * data;
    qn_uint16 cnt;
    qn_uint16 cap;

    qn_uint16 indexes[QN_JSON_DEFAULT_CAPACITY];
    qn_string keys[QN_JSON_DEFAULT_CAPACITY];
    qn_json_variant_st values[QN_JSON_DEFAULT_CAPACITY];
    qn_json_type_map_st type_maps[qn_json_type_capacity(QN_JSON_DEFAULT_CAPACITY)];
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
    new_obj->cap = sizeof(new_obj->values) / sizeof(new_obj->elements[0]);
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
    qn_json_type_map_ptr map = NULL;
    
    if (obj) {
        var = qn_json_variant_offset(obj->data, obj->cap);
        map = qn_json_type_map_offset(obj->data, obj->cap);

        for (i = 0; i < obj->cnt; i += 1) {
            qn_json_destroy_variant(qn_json_type_map_get(map, i), &var[i]);
            qn_str_destroy(obj->data[i]);
        } // for
        if (obj->data != &obj->keys[0]) {
            free(obj->data);
        } // if
        free(obj);
    } // if
}

static qn_bool qn_json_set_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_type type, qn_json_variant new_elem)
{
    qn_uint32 pos = 0;
    qn_string new_key = NULL;
    int existence = 0;
    qn_json_variant_ptr var = NULL;
    qn_json_type_map_ptr map = NULL;

    assert(obj);
    assert(key);

    var = qn_json_variant_offset(obj->data, obj->cap);
    map = qn_json_type_map_offset(obj->data, obj->cap);

    qn_json_variant_ptr var = NULL;
    qn_json_type_map_ptr map = NULL;

    pos = qn_json_obj_bsearch(obj->data, obj->cnt, key, &existence);
    if (existence == 0) {
        // There is an element according to the given key.
        qn_json_destroy_element(qn_json_type_map_get(map, pos), &var[pos]);
        qn_json_type_map_set(map, pos, type);
        var[pos] = new_elem;
        return qn_true;
    } // if

    if ((obj->cap - obj->cnt) <= 0 && ! qn_json_obj_augment(obj)) return qn_false;
    if (! (new_key = qn_cs_duplicate(key))) return qn_false;

    if (pos < obj->cnt) {
        memmove(&obj->itm[pos+1], &obj->itm[pos], sizeof(qn_json_obj_item) * (obj->cnt - pos));
    } // if

    obj->itm[pos].type = type;
    obj->itm[pos].key = new_key;
    obj->itm[pos].elem = new_elem;

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
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && ! qn_json_set_variant(obj, key, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
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
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && ! qn_json_set_variant(obj, key, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

static qn_uint32 qn_json_obj_bsearch(qn_string * restrict keys, qn_uint32 cnt, const char * restrict key, int * restrict existence)
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
    qn_uint32 new_size = (sizeof(qn_string) + sizeof(qn_json_variant_st)) * new_cap + sizeof(qn_json_type_map_st) * qn_json_type_map_count(new_cap);
    qn_string * new_data = calloc(1, new_size);
    if (! new_data) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_data, obj->data, sizeof(qn_string) * obj->cnt);
    memcpy(qn_json_variant_offset(new_data, new_cap), qn_json_variant_offset(qn_obj->data, qn_obj->cap), sizeof(qn_json_variant_st) * obj->cnt);
    memcpy(qn_json_type_map_offset(new_data, new_cap), qn_json_type_map_offset(qn_obj->data, qn_obj->cap), sizeof(qn_json_type_map_st) * qn_json_type_map_count(obj->cnt));

    if (obj->data != &obj->keys[0]) free(obj->data);

    obj->data = new_data;
    obj->cap = new_cap;
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
QN_SDK qn_uint32 qn_json_object_size(qn_json_object_ptr restrict obj)
{
    return obj->cnt;
}

/* == Set & Get methods == */

static qn_json_variant_ptr qn_json_get_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_type type)
{
    int existence = 0;
    qn_uint32 pos = 0;
    if (obj->cnt == 0) return NULL;
    pos = qn_json_obj_bsearch(obj->data, obj->cnt, key, &existence);
    return (existence != 0 || obj->itm[pos].type != type) ? NULL : &obj->itm[pos].elem;
}

QN_SDK qn_json_object_ptr qn_json_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_OBJECT);
    return (elem) ? elem->object : default_val;
}

QN_SDK qn_json_array_ptr qn_json_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_ARRAY);
    return (elem) ? elem->array : default_val;
}

QN_SDK qn_string qn_json_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_STRING);
    return (elem) ? elem->string : default_val;
}

QN_SDK const char * qn_json_get_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_STRING);
    return (elem) ? qn_str_cstr(elem->string) : default_val;
}

QN_SDK qn_json_integer qn_json_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_INTEGER);
    return (elem) ? elem->integer : default_val;
}

QN_SDK qn_json_number qn_json_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_NUMBER);
    return (elem) ? elem->number : default_val;
}

QN_SDK qn_bool qn_json_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool default_val)
{
    qn_json_variant_ptr elem = qn_json_get_variant(obj, key, QN_JSON_BOOLEAN);
    return (elem) ? elem->boolean : default_val;
}

QN_SDK qn_bool qn_json_set_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict val)
{
    qn_json_variant new_elem;
    new_elem.object = val;
    return qn_json_set_variant(obj, key, QN_JSON_OBJECT, new_elem);
}

QN_SDK qn_bool qn_json_set_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict val)
{
    qn_json_variant new_elem;
    new_elem.array = val;
    return qn_json_set_variant(obj, key, QN_JSON_ARRAY, new_elem);
}

QN_SDK qn_bool qn_json_set_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict val)
{
    qn_json_variant new_elem;
    if (! (new_elem.string = qn_str_duplicate(val))) return qn_false;
    if (! qn_json_set_variant(obj, key, QN_JSON_STRING, new_elem)) {
        qn_str_destroy(new_elem.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_set_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val)
{
    qn_json_variant new_elem;
    if (! (new_elem.string = qn_cs_duplicate(val))) return qn_false;
    if (! qn_json_set_variant(obj, key, QN_JSON_STRING, new_elem)) {
        qn_str_destroy(new_elem.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_set_text(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val, qn_size size)
{
    qn_json_variant new_elem;
    if (! (new_elem.string = qn_cs_clone(val, size))) return qn_false;
    if (! qn_json_set_variant(obj, key, QN_JSON_STRING, new_elem)) {
        qn_str_destroy(new_elem.string);
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_json_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer val)
{
    qn_json_variant new_elem;
    new_elem.integer = val;
    return qn_json_set_variant(obj, key, QN_JSON_INTEGER, new_elem);
}

QN_SDK qn_bool qn_json_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number val)
{
    qn_json_variant new_elem;
    new_elem.number = val;
    return qn_json_set_variant(obj, key, QN_JSON_NUMBER, new_elem);
}

QN_SDK qn_bool qn_json_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val)
{
    qn_json_variant new_elem;
    new_elem.boolean = val;
    return qn_json_set_variant(obj, key, QN_JSON_BOOLEAN, new_elem);
}

QN_SDK qn_bool qn_json_set_null(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant new_elem;
    new_elem.integer = 0;
    return qn_json_set_variant(obj, key, QN_JSON_NULL, new_elem);
}

// ---- Inplementation of array of JSON ----

typedef struct _QN_JSON_ARR_ITEM
{
    qn_json_type type;
    qn_json_variant elem;
} qn_json_arr_item;

typedef struct _QN_JSON_ARRAY
{
    qn_json_arr_item * itm;
    qn_json_pos begin;
    qn_json_pos end;
    qn_json_pos cnt;
    qn_json_pos cap;

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
    qn_json_pos i;

    for (i = arr->begin; i < arr->end; i += 1) {
        qn_json_destroy_element(arr->itm[i].type, &arr->itm[i].elem);
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
    qn_json_pos new_cap = arr->cap * 2;
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

static inline qn_json_pos qn_json_arr_find(qn_json_array_ptr restrict arr, int n)
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
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && ! qn_json_push_variant(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
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
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && ! qn_json_push_variant(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
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
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && ! qn_json_unshift_variant(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
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
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && ! qn_json_unshift_variant(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
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
    qn_json_pos pos;
    if (arr->cnt == 0) return NULL;
    pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].type != type) ? NULL : &arr->itm[pos].elem;
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
    qn_json_pos pos;
    int existence;

    if (obj->cnt == 0) return;

    pos = qn_json_obj_bsearch(obj->itm, obj->cnt, key, &existence);
    if (existence != 0) return; // There is no element corresponds to the key.

    qn_json_destroy_element(obj->itm[pos].type, &obj->itm[pos].elem);
    qn_str_destroy(obj->itm[pos].key);
    if (pos < obj->cnt - 1) memmove(&obj->itm[pos], &obj->itm[pos+1], sizeof(qn_json_obj_item) * (obj->cnt - pos - 1));
    obj->cnt -= 1;
}

QN_SDK qn_bool qn_json_rename(qn_json_object_ptr restrict obj, const char * restrict old_key, const char * new_key)
{
    qn_json_pos old_pos;
    qn_json_pos new_pos;
    qn_json_obj_item tmp_item;
    qn_string new_key_str;
    int existence;

    if (obj->cnt == 0) {
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    if (strcmp(old_key, new_key) == 0) return qn_true; // The old key is exactly the same to the new key.

    old_pos = qn_json_obj_bsearch(obj->itm, obj->cnt, old_key, &existence);
    if (existence != 0) {
        // ---- There is no element corresponds to the old key.
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    new_pos = qn_json_obj_bsearch(obj->itm, obj->cnt, new_key, &existence);
    if (existence == 0) {
        // ---- There is an element corresponds to the new key.
        // -- Destroy the element to be replaced.
        qn_json_destroy_element(obj->itm[new_pos].type, &obj->itm[new_pos].elem);

        // -- Replace the element.
        obj->itm[new_pos].type = obj->itm[old_pos].type;
        obj->itm[new_pos].elem = obj->itm[old_pos].elem;

        // -- Destroy the old key.
        qn_str_destroy(obj->itm[old_pos].key);

        if (old_pos < obj->cnt - 1) memmove(&obj->itm[old_pos], &obj->itm[old_pos+1], sizeof(qn_json_obj_item) * (obj->cnt - old_pos - 1));
        obj->cnt -= 1;

        return qn_true;
    } // if

    // ---- There is no element corresponds to the new key.
    // -- Replace the old key.
    new_key_str = qn_cs_duplicate(new_key);
    if (!new_key_str) return qn_false;

    qn_str_destroy(obj->itm[old_pos].key);
    obj->itm[old_pos].key = new_key_str;

    if (old_pos == new_pos) return qn_true; // -- The two keys reside in the same position.

    tmp_item = obj->itm[old_pos];
    if (old_pos < new_pos) {
        // -- The old key resides in a position before the new key.
        new_pos -= 1;
        if (old_pos < new_pos) memmove(&obj->itm[old_pos], &obj->itm[old_pos+1], sizeof(qn_json_obj_item) * (new_pos - old_pos));
    } else {
        // -- The old key resides in a position after the new key.
        memmove(&obj->itm[new_pos+1], &obj->itm[new_pos], sizeof(qn_json_obj_item) * (old_pos - new_pos));
    } // if
    obj->itm[new_pos] = tmp_item;
    return qn_true;
}

QN_SDK qn_bool qn_json_push_variant(qn_json_array_ptr restrict arr, qn_json_type type, qn_json_variant new_elem)
{
    assert(arr);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if ((arr->end == arr->cap) && !qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;

    arr->itm[arr->end].elem = new_elem;
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
        qn_json_destroy_element(arr->itm[arr->end].type, &arr->itm[arr->end].elem);
        arr->cnt -= 1;
    } // if
}

QN_SDK qn_bool qn_json_unshift_variant(qn_json_array_ptr restrict arr, qn_json_type type, qn_json_variant new_elem)
{
    assert(arr);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if ((arr->begin == 0) && !qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;

    arr->begin -= 1;
    arr->itm[arr->begin].elem = new_elem;
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
        qn_json_destroy_element(arr->itm[arr->begin].type, &arr->itm[arr->begin].elem);
        arr->begin += 1;
        arr->cnt -= 1;
    } // if
}

QN_SDK qn_bool qn_json_replace_variant(qn_json_array_ptr restrict arr, int n, qn_json_type type, qn_json_variant new_elem)
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

    qn_json_destroy_element(arr->itm[arr->begin + n].type, &arr->itm[arr->begin + n].elem);
    arr->itm[arr->begin + n].elem = new_elem;
    arr->itm[arr->begin + n].type = type;
    return qn_true;
}

// ---- Inplementation of iterator of JSON ----

typedef struct _QN_JSON_ITR_LEVEL
{
    qn_json_pos pos;
    qn_json_type type;
    qn_json_variant parent;
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
    qn_json_variant elem;
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
