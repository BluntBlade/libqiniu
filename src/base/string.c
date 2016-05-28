#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>
#include <errno.h>

#include "base/string.h"
#include "base/base64.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_STRING
{
    const char * cstr;
    qn_size size;
    char data[0];
} qn_string;

static qn_string qn_str_empty_one = {"", 0};

qn_size qn_str_size(const qn_string_ptr self)
{
    return self->size;
} // qn_str_size

const char * qn_str_cstr(const qn_string_ptr self)
{
    return self->cstr;
} // qn_str_cstr

int qn_str_compare(const qn_string_ptr restrict s1, const qn_string_ptr restrict s2)
{
    if (s1->size < s2->size) {
        // case   | 1      | 2       | 3
        // s1     | AABB   | BBBB    | BBCC
        // s2     | BBBBBB | BBBBBB  | BBBBBB
        // return | -1     | 0 => -1 | 1
        return (memcmp(s1->cstr, s2->cstr, s1->size) <= 0) ? -1 : 1;
    } // if

    if (s1->size > s2->size) {
        // case   | 1      | 2      | 3
        // s1     | AABBBB | BBBBBB | BBBBBB
        // s2     | BBBB   | BBBB   | AABB
        // return | -1     | 0 => 1 | 1
        return (memcmp(s1->cstr, s2->cstr, s2->size) >= 0) ? 1 : -1;
    } // if

    // case   | 1      | 2       | 3
    // s1     | AABB   | BBBB    | BBCC
    // s2     | BBBB   | BBBB    | BBBB
    // return | -1     | 0       | 1
    return memcmp(s1->cstr, s2->cstr, s1->size);
} // qn_str_compare

int qn_str_compare_raw(const qn_string_ptr restrict s1, const char * restrict s2)
{
    return strcmp(s1->cstr, s2);
} // qn_str_compare_raw

qn_string_ptr qn_str_allocate(qn_size size)
{
    return calloc(1, sizeof(qn_string) + size + 1);
} // qn_str_allocate

qn_string_ptr qn_str_create(const char * cstr, qn_size cstr_size)
{
    qn_string src;

    if (cstr_size == 0L) {
        return &qn_str_empty_one;
    }

    src.cstr = cstr;
    src.size = cstr_size;

    return qn_str_duplicate(&src);
} // qn_str_create

qn_string_ptr qn_str_clone_raw(const char * cstr)
{
    return qn_str_create(cstr, strlen(cstr));
} // qn_str_clone_raw

qn_string_ptr qn_str_duplicate(qn_string_ptr src)
{
    qn_string_ptr new_str = NULL;

    if (!src) {
        errno = EINVAL;
        return NULL;
    }

    new_str = qn_str_allocate(src->size);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    new_str->size = src->size;
    new_str->cstr = &new_str->data[0];

    if (new_str->size > 0) {
        memcpy((void*)new_str->cstr, src->cstr, new_str->size);
    }
    return new_str;
} // qn_str_duplicate

void qn_str_destroy(qn_string_ptr self)
{
    if (self && self != &qn_str_empty_one) {
        free(self);
    }
} // qn_str_destroy

void qn_str_fill(qn_string_ptr self, const char * cstr, qn_size cstr_size)
{
    if (self->size + 1 <= cstr_size) {
        memcpy((void*)self->cstr, cstr, self->size);
        ((char*)self->cstr)[self->size] = '\0';
    } else {
        memcpy((void*)self->cstr, cstr, cstr_size);
        ((char*)self->cstr)[cstr_size] = '\0';
    } // if
} // qn_str_fill

qn_string_ptr qn_str_join_raw(
    const char * restrict delimiter,
    const char * restrict s1,
    qn_size s1_size,
    const char * restrict s2,
    qn_size s2_size,
    ...)
{
    va_list ap;
    qn_string_ptr new_str = NULL;
    const char * src_str = NULL;
    char * dst_pos = NULL;
    qn_size src_size = 0L;
    qn_size remainder_capacity = QN_STR_MAX_SIZE;
    qn_size delimiter_size = 0L;
    int i = 0;
    int arg_pair_count = 0;

    //== Check fixed arguments
    if (!delimiter) {
        // Not a valid string passed as delimiter.
        errno = EINVAL;
        return NULL;
    }

    delimiter_size = strlen(delimiter);

    // Check the first and second strings
    if (!s1 || !s2) {
        errno = EINVAL;
        return NULL;
    }

    if (remainder_capacity < (s1_size + delimiter_size + s2_size)) {
        errno = EOVERFLOW;
        return NULL;
    }
    remainder_capacity -= (s1_size + delimiter_size + s2_size);

    // Check other <str,size> pairs
    va_start(ap, s2_size);
    while ((src_str = va_arg(ap, const char *))) {
        src_size = va_arg(ap, qn_size);
        if (remainder_capacity < delimiter_size + src_size) {
            va_end(ap);
            errno = EOVERFLOW;
            return NULL;
        }
        remainder_capacity -= (delimiter_size + src_size);
        arg_pair_count += 1;
    } // while
    va_end(ap);

    //== Prepare a new string object
    new_str = calloc(1, sizeof(*new_str) + (QN_STR_MAX_SIZE - remainder_capacity) + 1);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    //== Copy all source strings and delimiters between them.
    dst_pos = &new_str->data[0];

    // Copy the first and second strings, and the delimiter between them.
    memcpy(dst_pos, s1, s1_size);
    dst_pos += s1_size;
    if (delimiter_size > 0L) {
        memcpy(dst_pos, delimiter, delimiter_size);
        dst_pos += delimiter_size;
    }
    memcpy(dst_pos, s2, s2_size);
    dst_pos += s2_size;

    // Copy other strings and delimiters between them.
    if (arg_pair_count > 0) {
        va_start(ap, s2_size);
        for (i = 0; i < arg_pair_count; i += 1) {
            if (delimiter_size > 0L) {
                memcpy(dst_pos, delimiter, delimiter_size);
                dst_pos += delimiter_size;
            }

            src_str = va_arg(ap, const char *);
            src_size = va_arg(ap, qn_size);
            if (src_size > 0L) {
                memcpy(dst_pos, src_str, src_size);
                dst_pos += src_size;
            }
        } // for
        va_end(ap);
    } // if

    new_str->size = QN_STR_MAX_SIZE - remainder_capacity;
    new_str->cstr = &new_str->data[0];
    return new_str;
} // qn_str_join_raw

qn_string_ptr qn_str_join(const char * restrict delimiter, qn_string_ptr strs[], int n)
{
    qn_string_ptr new_str = NULL;
    char * dst_pos = NULL;
    qn_size remainder_capacity = QN_STR_MAX_SIZE;
    qn_size delimiter_size = 0L;
    int i = 0;

    //== Check fixed arguments
    if (!delimiter || !strs || n == 0) {
        // Not a valid string as delimiter, or no strings to join.
        errno = EINVAL;
        return NULL;
    }

    if (n == 1) {
        return qn_str_duplicate(strs[0]);
    }

    delimiter_size = strlen(delimiter);

    // Check string objects
    remainder_capacity -= strs[0]->size;
    for (i = 1; i < n; i += 1) {
        if (remainder_capacity < delimiter_size + strs[i]->size) {
            errno = EOVERFLOW;
            return NULL;
        }
        remainder_capacity -= (delimiter_size + strs[i]->size);
    } // for

    //== Prepare a new string object
    new_str = calloc(1, sizeof(*new_str) + (QN_STR_MAX_SIZE - remainder_capacity) + 1);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    //== Copy all source strings and delimiters between them.
    dst_pos = &new_str->data[0];

    // Copy all strings and delimiters between them.
    if (strs[0]->size > 0L) {
        memcpy(dst_pos, strs[0]->cstr, strs[0]->size);
        dst_pos += strs[0]->size;
    } // if

    for (i = 1; i < n; i += 1) {
        if (delimiter_size > 0L) {
            memcpy(dst_pos, delimiter, delimiter_size);
            dst_pos += delimiter_size;
        }
        if (strs[i]->size > 0L) {
            memcpy(dst_pos, strs[i]->cstr, strs[i]->size);
            dst_pos += strs[i]->size;
        }
    } // for

    new_str->size = QN_STR_MAX_SIZE - remainder_capacity;
    new_str->cstr = &new_str->data[0];
    return new_str;
} // qn_str_join

qn_string_ptr qn_str_vprintf(const char * restrict format, va_list ap)
{
    va_list cp;
    qn_string_ptr new_str = NULL;
    int printed_count = 0;

    va_copy(cp, ap);
#if defined(_MSC_VER)
    printed_count = vsnprintf(0x1, 1, format, ap);
#else
    printed_count = vsnprintf(NULL, 0, format, ap);
#endif
    va_end(cp);

    if (printed_count < 0) {
        // Keep the errno set by vsnprintf.
        return NULL;
    }
    if (printed_count == 0) {
        return &qn_str_empty_one;
    }

    new_str = qn_str_allocate(printed_count);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    va_copy(cp, ap);
    printed_count = vsnprintf(&new_str->data[0], printed_count + 1, format, ap);
    va_end(cp);

    if (printed_count < 0) {
        // Keep the errno set by vsnprintf.
        qn_str_destroy(new_str);
        return NULL;
    }
    return new_str;
} // qn_str_vprintf

qn_string_ptr qn_str_sprintf(const char * restrict format, ...)
{
    va_list ap;
    qn_string_ptr new_str = NULL;

    va_start(ap, format);
    new_str = qn_str_vprintf(format, ap);
    va_end(ap);
    return new_str;
} // qn_str_printf

#if defined(_MSC_VER)

#if (_MSC_VER < 1400)
#error The MSVC compiler's version is lower then VC++ 2005.
#endif

int qn_str_snprintf(char * restrict str, qn_size size,  const char * restrict format, ...)
{
    va_list ap;
    int printed_count = 0;
    char * buf = str;
    qn_size buf_cap = size;

    if (str == NULL || size == 0) {
        buf = 0x1;
        buf_cap = 1;
    } // if

    va_start(ap, format);
    printed_count = vsnprintf(buf, buf_cap, format, ap);
    va_end(ap);
    return printed_count;
} // qn_str_snprintf

#else

int qn_str_snprintf(char * restrict str, qn_size size,  const char * restrict format, ...)
{
    va_list ap;
    int printed_count = 0;

    va_start(ap, format);
    printed_count = vsnprintf(str, size, format, ap);
    va_end(ap);
    return printed_count;
} // qn_str_snprintf

#endif

qn_string_ptr qn_str_encode_base64_urlsafe(const char * restrict bin, qn_size bin_size)
{
    qn_string_ptr new_str = NULL;
    qn_size encoding_size = qn_b64_encode_urlsafe(NULL, 0, bin, bin_size, QN_B64_APPEND_PADDING);
    
    if (encoding_size == 0) {
        return &qn_str_empty_one;
    }

    new_str = qn_str_allocate(encoding_size);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    qn_b64_encode_urlsafe(&new_str->data[0], encoding_size, bin, bin_size, QN_B64_APPEND_PADDING);
    return new_str;
} // qn_str_encode_base64_urlsafe

qn_string_ptr qn_str_decode_base64_urlsafe(const char * restrict str, qn_size str_size)
{
    qn_string_ptr new_str = NULL;
    qn_size decoding_size = qn_b64_decode_urlsafe(NULL, 0, str, str_size, 0);
    
    if (decoding_size == 0) {
        return &qn_str_empty_one;
    }

    new_str = qn_str_allocate(decoding_size);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    qn_b64_decode_urlsafe(&new_str->data[0], decoding_size, str, str_size, 0);
    return new_str;
} // qn_str_decode_base64_urlsafe

#ifdef __cplusplus
}
#endif
