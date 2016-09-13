#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned short int qn_json_pos;

// ---- Inplementation of object of JSON ----

typedef struct _QN_JSON_OBJ_ITEM
{
    qn_json_hash hash;
    qn_string key;
    qn_json_class class;
    qn_json_variant elem;
} qn_json_obj_item;

typedef struct _QN_JSON_OBJECT
{
    qn_json_obj_item * itm;
    qn_json_pos cnt;
    qn_json_pos cap;

    qn_json_obj_item init_itm[2];
} qn_json_object;

static qn_json_hash qn_json_obj_calculate_hash(const char * restrict cstr)
{
    qn_json_hash hash = 5381;
    int c;

    while ((c = *cstr++) != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } // while
    return hash;
}

QN_API qn_json_object_ptr qn_json_create_object(void)
{
    qn_json_object_ptr new_obj = calloc(1, sizeof(qn_json_object));
    if (!new_obj) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_obj->itm = &new_obj->init_itm[0];
    new_obj->cap = sizeof(new_obj->init_itm) / sizeof(new_obj->init_itm[0]);
    return new_obj;
}

QN_API void qn_json_destroy_object(qn_json_object_ptr restrict obj)
{
    qn_json_pos i;

    for (i = 0; i < obj->cnt; i += 1) {
        switch (obj->itm[i].class) {
            case QN_JSON_OBJECT: qn_json_destroy_object(obj->itm[i].elem.object); break;
            case QN_JSON_ARRAY:  qn_json_destroy_array(obj->itm[i].elem.array); break;
            case QN_JSON_STRING: qn_str_destroy(obj->itm[i].elem.string); break;
            default: break;
        } // switch
        free(obj->itm[i].key);
    } // for
    if (obj->itm != &obj->init_itm[0]) {
        free(obj->itm);
    } // if
    free(obj);
}

static qn_json_pos qn_json_obj_bsearch(qn_json_obj_item * restrict itm, qn_json_pos cnt, qn_json_hash hash)
{
    qn_json_pos begin = 0;
    qn_json_pos end = cnt;
    qn_json_pos mid = 0;
    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        if (itm[mid].hash < hash) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    return begin;
}

static qn_json_pos qn_json_obj_find(qn_json_object_ptr restrict obj, qn_json_hash hash, const char * restrict key, int * restrict existent)
{
    qn_json_pos i = qn_json_obj_bsearch(obj->itm, obj->cnt, hash);
    while (i < obj->cnt && obj->itm[i].hash == hash && (*existent = qn_str_compare_raw(obj->itm[i].key, key)) != 0) i += 1;
    return i;
}

static qn_bool qn_json_obj_augment(qn_json_object_ptr restrict obj)
{
    qn_json_pos new_cap = obj->cap * 2;
    qn_json_obj_item * new_itm = calloc(1, sizeof(qn_json_obj_item) * new_cap);
    if (!new_itm) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_itm, obj->itm, sizeof(qn_json_obj_item) * obj->cnt);
    if (obj->itm != &obj->init_itm[0]) free(obj->itm);

    obj->itm = new_itm;
    obj->cap = new_cap;
    return qn_true;
}

static qn_bool qn_json_obj_set(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_class cls, qn_json_variant new_elem)
{
    qn_json_hash hash;
    qn_json_pos pos;
    int existent = -2;

    hash = qn_json_obj_calculate_hash(key);
    pos = qn_json_obj_find(obj, hash, key, &existent);
    if (existent == 0) {
        // There is an element according to the given key.
        if (obj->itm[pos].class == QN_JSON_OBJECT) {
            qn_json_destroy_object(obj->itm[pos].elem.object);
        } else if (obj->itm[pos].class == QN_JSON_ARRAY) {
            qn_json_destroy_array(obj->itm[pos].elem.array);
        } else if (obj->itm[pos].class == QN_JSON_STRING) {
            qn_str_destroy(obj->itm[pos].elem.string);
        } // if
        obj->itm[pos].class = cls;
        obj->itm[pos].elem = new_elem;
        return qn_true;
    } // if

    if ((obj->cap - obj->cnt) <= 0 && !qn_json_obj_augment(obj)) return qn_false;

    if (pos < obj->cnt) memmove(&obj->itm[pos+1], &obj->itm[pos], sizeof(qn_json_obj_item) * (obj->cnt - pos));

    if (!(obj->itm[pos].key = qn_cs_duplicate(key))) return qn_false;
    obj->itm[pos].hash = hash;
    obj->itm[pos].class = cls;
    obj->itm[pos].elem = new_elem;

    obj->cnt += 1;
    return qn_true;
}

// ---- Inplementation of array of JSON ----

typedef struct _QN_JSON_ARR_ITEM
{
    qn_json_class class;
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

QN_API qn_json_array_ptr qn_json_create_array(void)
{
    qn_json_array_ptr new_arr = calloc(1, sizeof(qn_json_array));
    if (!new_arr) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_arr->itm = &new_arr->init_itm[0];
    new_arr->cap = sizeof(new_arr->init_itm) / sizeof(new_arr->init_itm[0]);
    return new_arr;
}

QN_API void qn_json_destroy_array(qn_json_array_ptr restrict arr)
{
    qn_json_pos i;

    for (i = arr->begin; i < arr->end; i += 1) {
        switch (arr->itm[i].class) {
            case QN_JSON_OBJECT: qn_json_destroy_object(arr->itm[i].elem.object); break;
            case QN_JSON_ARRAY:  qn_json_destroy_array(arr->itm[i].elem.array); break;
            case QN_JSON_STRING: qn_str_destroy(arr->itm[i].elem.string); break;
            default: break;
        } // switch
    } // for
    if (arr->itm != &arr->init_itm[0]) {
        free(arr->itm);
    } // if
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
        qn_err_set_no_enough_memory();
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

static qn_bool qn_json_arr_push(qn_json_array_ptr restrict arr, qn_json_class cls, qn_json_variant new_elem)
{
    if ((arr->end == arr->cap) && !qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;
    arr->itm[arr->end].elem = new_elem;
    arr->itm[arr->end].class = cls;
    arr->end += 1;
    arr->cnt += 1;
    return qn_true;
}

static qn_bool qn_json_arr_unshift(qn_json_array_ptr restrict arr, qn_json_class cls, qn_json_variant new_elem)
{
    if ((arr->begin == 0) && !qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;
    arr->begin -= 1;
    arr->itm[arr->begin].elem = new_elem;
    arr->itm[arr->begin].class = cls;
    arr->cnt += 1;
    return qn_true;
}

// ---- Inplementation of JSON ----

QN_API qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && !qn_json_obj_set(obj, key, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

QN_API qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && !qn_json_obj_set(obj, key, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

QN_API qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && !qn_json_arr_push(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

QN_API qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && !qn_json_arr_push(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

QN_API qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && !qn_json_arr_unshift(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

QN_API qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && !qn_json_arr_unshift(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

QN_API int qn_json_size_object(qn_json_object_ptr restrict obj)
{
    return obj->cnt;
}

QN_API int qn_json_size_array(qn_json_array_ptr restrict arr)
{
    return arr->cnt;
}

QN_API qn_json_object_ptr qn_json_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict default_val)
{
    int existent = -2;
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    return (existent != 0 || obj->itm[pos].class != QN_JSON_OBJECT) ? default_val : obj->itm[pos].elem.object;
}

QN_API qn_json_array_ptr qn_json_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict default_val)
{
    int existent = -2;
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    return (existent != 0 || obj->itm[pos].class != QN_JSON_ARRAY) ? default_val : obj->itm[pos].elem.array;
}

QN_API qn_string qn_json_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict default_val)
{
    int existent = -2;
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    return (existent != 0 || obj->itm[pos].class != QN_JSON_STRING) ? default_val : obj->itm[pos].elem.string;
}

QN_API qn_integer qn_json_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_integer default_val)
{
    int existent = -2;
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    return (existent != 0 || obj->itm[pos].class != QN_JSON_INTEGER) ? default_val : obj->itm[pos].elem.integer;
}

QN_API qn_number qn_json_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_number default_val)
{
    int existent = -2;
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    return (existent != 0 || obj->itm[pos].class != QN_JSON_NUMBER) ? default_val : obj->itm[pos].elem.number;
}

QN_API qn_bool qn_json_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool default_val)
{
    int existent = -2;
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    return (existent != 0 || obj->itm[pos].class != QN_JSON_BOOLEAN) ? default_val : obj->itm[pos].elem.boolean;
}

QN_API qn_json_object_ptr qn_json_pick_object(qn_json_array_ptr restrict arr, int n, qn_json_object_ptr restrict default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_OBJECT) ? default_val : arr->itm[pos].elem.object;
}

QN_API qn_json_array_ptr qn_json_pick_array(qn_json_array_ptr restrict arr, int n, qn_json_array_ptr restrict default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_ARRAY) ? default_val : arr->itm[pos].elem.array;
}

QN_API qn_string qn_json_pick_string(qn_json_array_ptr restrict arr, int n, qn_string restrict default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_STRING) ? default_val : arr->itm[pos].elem.string;
}

QN_API qn_integer qn_json_pick_integer(qn_json_array_ptr restrict arr, int n, qn_integer default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_INTEGER) ? default_val : arr->itm[pos].elem.integer;
}

QN_API qn_number qn_json_pick_number(qn_json_array_ptr restrict arr, int n, qn_number default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_NUMBER) ? default_val : arr->itm[pos].elem.number;
}

QN_API qn_bool qn_json_pick_boolean(qn_json_array_ptr restrict arr, int n, qn_bool default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_BOOLEAN) ? default_val : arr->itm[pos].elem.boolean;
}

QN_API qn_bool qn_json_set_string(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val)
{
    qn_json_variant elem;
    elem.string = qn_cs_duplicate(val);
    return (!elem.string) ? qn_false : qn_json_obj_set(obj, key, QN_JSON_STRING, elem);
}

QN_API qn_bool qn_json_set_text(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val, size_t size)
{
    qn_json_variant elem;
    if (size < 0) {
        elem.string = qn_cs_duplicate(val);
    } else if (size > 0) {
        elem.string = qn_cs_clone(val, size);
    } else {
        elem.string = qn_str_empty_string;
    } // if
    return (!elem.string) ? qn_false : qn_json_obj_set(obj, key, QN_JSON_STRING, elem);
}

QN_API qn_bool qn_json_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_integer val)
{
    qn_json_variant elem;
    elem.integer = val;
    return qn_json_obj_set(obj, key, QN_JSON_INTEGER, elem);
}

QN_API qn_bool qn_json_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_number val)
{
    qn_json_variant elem;
    elem.number = val;
    return qn_json_obj_set(obj, key, QN_JSON_NUMBER, elem);
}

QN_API qn_bool qn_json_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val)
{
    qn_json_variant elem;
    elem.boolean = val;
    return qn_json_obj_set(obj, key, QN_JSON_BOOLEAN, elem);
}

QN_API qn_bool qn_json_set_null(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant elem;
    elem.integer = 0;
    return qn_json_obj_set(obj, key, QN_JSON_NULL, elem);
}

QN_API void qn_json_unset(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_pos pos;
    int existent = -2;

    if (obj->cnt == 0) return;

    pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key, &existent);
    if (existent != 0) return; // There is no element according to the given key.

    switch (obj->itm[pos].class) {
        case QN_JSON_OBJECT: qn_json_destroy_object(obj->itm[pos].elem.object); break;
        case QN_JSON_ARRAY: qn_json_destroy_array(obj->itm[pos].elem.array); break;
        case QN_JSON_STRING: qn_str_destroy(obj->itm[pos].elem.string); break;
        default: break;
    }
    free((void*)obj->itm[pos].key);
    memmove(&obj->itm[pos], &obj->itm[pos+1], sizeof(qn_json_obj_item) * (obj->cnt - pos - 1));
    obj->cnt -= 1;
}

QN_API qn_bool qn_json_push_string(qn_json_array_ptr restrict arr, const char * restrict val)
{
    qn_json_variant elem;
    elem.string = qn_cs_duplicate(val);
    return (!elem.string) ? qn_false : qn_json_arr_push(arr, QN_JSON_STRING, elem);
}

QN_API qn_bool qn_json_push_text(qn_json_array_ptr restrict arr, const char * restrict val, size_t size)
{
    qn_json_variant elem;
    if (size < 0) {
        elem.string = qn_cs_duplicate(val);
    } else if (size > 0) {
        elem.string = qn_cs_clone(val, size);
    } else {
        elem.string = qn_str_empty_string;
    } // if
    return (!elem.string) ? qn_false : qn_json_arr_push(arr, QN_JSON_STRING, elem);
}

QN_API qn_bool qn_json_push_integer(qn_json_array_ptr restrict arr, qn_integer val)
{
    qn_json_variant elem;
    elem.integer = val;
    return qn_json_arr_push(arr, QN_JSON_INTEGER, elem);
}

QN_API qn_bool qn_json_push_number(qn_json_array_ptr restrict arr, qn_number val)
{
    qn_json_variant elem;
    elem.number = val;
    return qn_json_arr_push(arr, QN_JSON_NUMBER, elem);
}

QN_API qn_bool qn_json_push_boolean(qn_json_array_ptr restrict arr, qn_bool val)
{
    qn_json_variant elem;
    elem.boolean = val;
    return qn_json_arr_push(arr, QN_JSON_BOOLEAN, elem);
}

QN_API qn_bool qn_json_push_null(qn_json_array_ptr restrict arr)
{
    qn_json_variant elem;
    elem.integer = 0;
    return qn_json_arr_push(arr, QN_JSON_NULL, elem);
}

QN_API void qn_json_pop(qn_json_array_ptr restrict arr)
{
    if (arr->cnt > 0) {
        arr->end -= 1;
        if (arr->itm[arr->end].class == QN_JSON_OBJECT) {
            qn_json_destroy_object(arr->itm[arr->end].elem.object);
        } else if (arr->itm[arr->end].class == QN_JSON_ARRAY) {
            qn_json_destroy_array(arr->itm[arr->end].elem.array);
        }
        arr->cnt -= 1;
    } // if
}

QN_API qn_bool qn_json_unshift_string(qn_json_array_ptr restrict arr, const char * restrict val)
{
    qn_json_variant elem;
    elem.string = qn_cs_duplicate(val);
    return (!elem.string) ? qn_false : qn_json_arr_unshift(arr, QN_JSON_STRING, elem);
}

QN_API qn_bool qn_json_unshift_text(qn_json_array_ptr restrict arr, const char * restrict val, size_t size)
{
    qn_json_variant elem;
    if (size < 0) {
        elem.string = qn_cs_duplicate(val);
    } else if (size > 0) {
        elem.string = qn_cs_clone(val, size);
    } else {
        elem.string = qn_str_empty_string;
    } // if
    return (!elem.string) ? qn_false : qn_json_arr_unshift(arr, QN_JSON_STRING, elem);
}

QN_API qn_bool qn_json_unshift_integer(qn_json_array_ptr restrict arr, qn_integer val)
{
    qn_json_variant elem;
    elem.integer = val;
    return qn_json_arr_unshift(arr, QN_JSON_INTEGER, elem);
}

QN_API qn_bool qn_json_unshift_number(qn_json_array_ptr restrict arr, qn_number val)
{
    qn_json_variant elem;
    elem.number = val;
    return qn_json_arr_unshift(arr, QN_JSON_NUMBER, elem);
}

QN_API qn_bool qn_json_unshift_boolean(qn_json_array_ptr restrict arr, qn_bool val)
{
    qn_json_variant elem;
    elem.boolean = val;
    return qn_json_arr_unshift(arr, QN_JSON_BOOLEAN, elem);
}

QN_API qn_bool qn_json_unshift_null(qn_json_array_ptr restrict arr)
{
    qn_json_variant elem;
    elem.integer = 0;
    return qn_json_arr_unshift(arr, QN_JSON_NULL, elem);
}

QN_API void qn_json_shift(qn_json_array_ptr restrict arr)
{
    if (arr->cnt > 0) {
        if (arr->itm[arr->begin].class == QN_JSON_OBJECT) {
            qn_json_destroy_object(arr->itm[arr->begin].elem.object);
        } else if (arr->itm[arr->begin].class == QN_JSON_ARRAY) {
            qn_json_destroy_array(arr->itm[arr->begin].elem.array);
        }
        arr->begin += 1;
        arr->cnt -= 1;
    } // if
}

// ---- Inplementation of iterator of JSON ----

typedef struct _QN_JSON_ITR_LEVEL
{
    qn_json_pos pos;
    qn_json_class class;
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

QN_API qn_json_iterator_ptr qn_json_itr_create(void)
{
    qn_json_iterator_ptr new_itr = calloc(1, sizeof(qn_json_iterator));
    if (!new_itr) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_itr->cap = sizeof(new_itr->init_lvl) / sizeof(new_itr->init_lvl[0]);
    new_itr->lvl = &new_itr->init_lvl[0];
    return new_itr;
}

QN_API void qn_json_itr_destroy(qn_json_iterator_ptr restrict itr)
{
    if (itr) {
        if (itr->lvl != &itr->init_lvl[0]) {
            free(itr->lvl);
        } // if
        free(itr);
    } // if
}

QN_API void qn_json_itr_reset(qn_json_iterator_ptr restrict itr)
{
    itr->cnt = 0;
}

QN_API void qn_json_itr_rewind(qn_json_iterator_ptr restrict itr)
{
    if (itr->cnt <= 0) return;
    itr->lvl[itr->cnt - 1].pos = 0;
}

QN_API qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr restrict itr)
{
    return itr->cnt == 0;
}

QN_API int qn_json_itr_steps(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0) ? 0 : itr->lvl[itr->cnt - 1].pos;
}

QN_API qn_string qn_json_itr_get_key(qn_json_iterator_ptr restrict itr)
{
    qn_json_itr_level_ptr lvl = NULL;

    if (itr->cnt == 0) return NULL;
    lvl = &itr->lvl[itr->cnt - 1];

    if (lvl->class != QN_JSON_OBJECT) return NULL;
    return lvl->parent.object->itm[lvl->pos - 1].key;
}

QN_API void qn_json_itr_set_status(qn_json_iterator_ptr restrict itr, qn_uint32 sts)
{
    if (itr->cnt) itr->lvl[itr->cnt - 1].status = sts;
}

QN_API qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt == 0) ? 0 : itr->lvl[itr->cnt - 1].status;
}

static qn_bool qn_json_itr_augment_levels(qn_json_iterator_ptr restrict itr)
{
    int new_capacity = itr->cap + (itr->cap >> 1); // 1.5 times of the last stack capacity.
    qn_json_itr_level_ptr new_lvl = calloc(1, sizeof(qn_json_itr_level) * new_capacity);
    if (!new_lvl) {
        qn_err_set_no_enough_memory();
        return qn_false;
    }  // if

    memcpy(new_lvl, itr->lvl, sizeof(qn_json_itr_level) * itr->cnt);
    if (itr->lvl != &itr->init_lvl[0]) free(itr->lvl);
    itr->lvl = new_lvl;
    itr->cap = new_capacity;
    return qn_true;
}

QN_API qn_bool qn_json_itr_push_object(qn_json_iterator_ptr restrict itr, qn_json_object_ptr obj)
{
    if ((itr->cnt + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->cnt].class = QN_JSON_OBJECT;
    itr->lvl[itr->cnt].parent.object = obj;
    itr->cnt += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

QN_API qn_bool qn_json_itr_push_array(qn_json_iterator_ptr restrict itr, qn_json_array_ptr arr)
{
    if ((itr->cnt + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->cnt].class = QN_JSON_ARRAY;
    itr->lvl[itr->cnt].parent.array = arr;
    itr->cnt += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

QN_API void qn_json_itr_pop(qn_json_iterator_ptr restrict itr)
{
    if (itr->cnt > 0) itr->cnt -= 1;
}

QN_API qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0 || itr->lvl[itr->cnt - 1].class != QN_JSON_OBJECT) ? NULL : itr->lvl[itr->cnt - 1].parent.object;
}

QN_API qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0 || itr->lvl[itr->cnt - 1].class != QN_JSON_ARRAY) ? NULL : itr->lvl[itr->cnt - 1].parent.array;
}

QN_API int qn_json_itr_advance(qn_json_iterator_ptr restrict itr, void * data, qn_json_itr_callback cb)
{
    qn_json_class class;
    qn_json_variant elem;
    qn_json_itr_level_ptr lvl;

    if (itr->cnt <= 0) return QN_JSON_ITR_NO_MORE;

    lvl = &itr->lvl[itr->cnt - 1];
    if (lvl->class == QN_JSON_OBJECT) {
        if (lvl->pos < lvl->parent.object->cnt) {
            class = lvl->parent.object->itm[lvl->pos].class;
            elem = lvl->parent.object->itm[lvl->pos].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        }
    } else {
        if (lvl->pos < lvl->parent.array->cnt) {
            class = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].class;
            elem = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        }
    }
    return cb(data, class, &elem);
}

#ifdef __cplusplus
}
#endif
