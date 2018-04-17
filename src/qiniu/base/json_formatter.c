#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"

#ifdef __cplusplus
extern "C"
{
#endif

//-- Implementation of qn_json_formatter

enum 
{
    QN_JSON_FMT_START = 0,
    QN_JSON_FMT_OPENING_BRACE = 1,
    QN_JSON_FMT_OPENING_BRACKET = 2,
    QN_JSON_FMT_CLOSING = 3,
    QN_JSON_FMT_COLON = 4,
    QN_JSON_FMT_COMMA = 5,
    QN_JSON_FMT_OBJECT = 6,
    QN_JSON_FMT_ARRAY = 7,
    QN_JSON_FMT_STRING = 8,
    QN_JSON_FMT_INTEGER = 9,
    QN_JSON_FMT_NUMBER = 10,
    QN_JSON_FMT_BOOLEAN = 11,
    QN_JSON_FMT_NULL = 12,
    QN_JSON_FMT_END = 13,
    QN_JSON_FMT_ERROR = 14
};

enum
{
    QN_JSON_FMT_ESCAPE_UTF8_STRING = 0x1
};

typedef struct _QN_JSON_FORMATTER {
    int flags;
    qn_uint32 with_key:1;
    qn_uint32 tkn:4;

    char * buf;
    qn_size buf_size;
    qn_size buf_capacity;

    qn_string string;
    qn_size string_pos;

    qn_json_iterator_ptr itr;
} qn_json_formatter;

#define QN_JSON_FMT_PAGE_SIZE 4096

QN_SDK qn_json_formatter_ptr qn_json_fmt_create(void)
{
    qn_json_formatter_ptr new_fmt = NULL;

    new_fmt = calloc(1, sizeof(qn_json_formatter));
    if (!new_fmt) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_fmt->itr = qn_json_itr_create();
    if (!new_fmt->itr) {
        free(new_fmt->buf);
        free(new_fmt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_fmt->flags = 0;
    return new_fmt;
}

QN_SDK void qn_json_fmt_destroy(qn_json_formatter_ptr restrict fmt)
{
    if (fmt) {
        qn_json_itr_destroy(fmt->itr);
        free(fmt);
    } // for
}

QN_SDK void qn_json_fmt_reset(qn_json_formatter_ptr restrict fmt)
{
    qn_json_itr_pop_all(fmt->itr);
    fmt->tkn = QN_JSON_FMT_START;
    fmt->flags = 0;
}

QN_SDK void qn_json_fmt_enable_escape_utf8_string(qn_json_formatter_ptr restrict fmt)
{
    fmt->flags |= QN_JSON_FMT_ESCAPE_UTF8_STRING;
}

QN_SDK void qn_json_fmt_disable_escape_utf8_string(qn_json_formatter_ptr restrict fmt)
{
    fmt->flags &= ~QN_JSON_FMT_ESCAPE_UTF8_STRING;
}

static inline qn_bool qn_json_fmt_putc(qn_json_formatter_ptr fmt, char ch)
{
    if (fmt->buf_size == fmt->buf_capacity) {
        qn_err_set_out_of_buffer();
        return qn_false;
    } // if

    fmt->buf[fmt->buf_size++] = ch;
    return qn_true;
}

static qn_bool qn_json_fmt_serialize_string(qn_json_formatter_ptr restrict fmt)
{
#define c1 (str[pos-1])
#define c2 (str[pos])
#define c3 (str[pos+1])
#define c4 (str[pos+2])
#define head_code (0xD800 + (((wch - 0x10000) & 0xFFC00) >> 10))
#define tail_code (0xDC00 + ((wch - 0x10000) & 0x003FF))

    qn_size free_size = 0;
    qn_size pos = fmt->string_pos;
    qn_size end = qn_str_size(fmt->string) + 1;
    const char * str = qn_str_cstr(fmt->string);
    int chars = 0;
    int ret = 0;
    qn_uint32 wch = 0;

    if (pos == 0) {
        if (! qn_json_fmt_putc(fmt, '"')) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
        ++pos;
    }

    while (pos < end) {
        free_size = fmt->buf_capacity - fmt->buf_size;

        if ((c1 & 0x80) == 0 || ((fmt->flags & QN_JSON_FMT_ESCAPE_UTF8_STRING) == 0)) {
            // ASCII range: 0zzzzzzz（00-7F）
            if (c1 == '&') {
                if ((fmt->buf_size + 6) >= fmt->buf_capacity) {
                    qn_err_set_out_of_buffer();
                    goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
                } // if

                ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "\\u%4X", c1);
                if (ret < 0) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
                if (ret >= free_size) {
                    qn_err_set_out_of_buffer();
                    goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
                } // if

                pos += 1;
                fmt->buf_size += ret;
            } else if (c1 == '"') {
                if (! qn_json_fmt_putc(fmt, '\\') || ! qn_json_fmt_putc(fmt, '"')) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
                pos += 1;
            } else if (c1 == '\\') {
                if (! qn_json_fmt_putc(fmt, '\\') || ! qn_json_fmt_putc(fmt, '\\')) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
                pos += 1;
            } else {
                if (! qn_json_fmt_putc(fmt, c1)) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
                pos += 1;
            } // if

            continue;
        } // if

        if ((c1 & 0xE0) == 0xC0) {
            // Check if the c2 is valid.
            if ((end - pos < 2) || (c2 & 0xC0) != 0x80) {
                qn_err_set_bad_utf8_sequence();
                goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
            } // if

            // From : 110yyyyy（C0-DF) 10zzzzzz(80-BF）
            // To   : 00000000 00000yyy yyzzzzzz
            wch = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
            chars = 2;
        } else if ((c1 & 0xF0) == 0xE0) {
            // Check if the c2 and c3 are valid.
            if ((end - pos < 3) || ((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80)) {
                qn_err_set_bad_utf8_sequence();
                goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
            } // if

            // From : 1110xxxx(E0-EF) 10yyyyyy 10zzzzzz
            // To   : 00000000 xxxxyyyy yyzzzzzz
            wch = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            chars = 3;
        } else if ((c1 & 0xF8) == 0xF0) {
            // Check if the c2 and c3 and c4 are valid.
            if ((end - pos < 4) || ((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80)) {
                qn_err_set_bad_utf8_sequence();
                goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
            } // if

            // From : 11110www(F0-F7) 10xxxxxx 10yyyyyy 10zzzzzz
            // To   : 000wwwxx xxxxyyyy yyzzzzzz
            wch = ((c1 & 0x1F) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
            chars = 4;
        } // if

        if (0xD800 <= wch && wch <= 0xDFFF) {
            qn_err_set_bad_utf8_sequence();
            goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
        } // if

        if ((fmt->buf_size + 12) >= fmt->buf_capacity) {
            qn_err_set_out_of_buffer();
            goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
        } // if

        ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "\\u%4X\\u%4X", head_code, tail_code);
        if (ret < 0) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
        if (ret >= free_size) {
            qn_err_set_out_of_buffer();
            goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
        } // if

        pos += chars;
        fmt->buf_size += ret;
    } // while

    if (pos == end) {
        if (! qn_json_fmt_putc(fmt, '"')) goto QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING;
        ++pos;

        qn_json_itr_reclaim_string(fmt->itr, fmt->string);
        fmt->string = NULL;
        fmt->string_pos = 0;
    }

    return qn_true;

QN_JSON_FMT_SERIALIZE_STRING_ERROR_HANDLING:
    fmt->string_pos = pos;
    return qn_false;

#undef c1
#undef c2
#undef c3
#undef c4
#undef head_code
#undef tail_code
}

static qn_bool qn_json_fmt_output_key(qn_json_formatter_ptr restrict fmt, qn_string restrict key)
{
    assert(key);

    fmt->with_key = 0;
    fmt->tkn = QN_JSON_FMT_COLON;

    fmt->string = key;
    fmt->string_pos = 0;
    if (! qn_json_fmt_serialize_string(fmt)) return qn_false;

    fmt->string = NULL;
    fmt->string_pos = 0;
    return qn_true;
}

static qn_bool qn_json_fmt_push_object(qn_json_formatter_ptr restrict fmt)
{
    qn_string key = NULL;
    qn_json_object_ptr val = NULL;

    assert(fmt->tkn == QN_JSON_FMT_OBJECT); 

    if (! qn_json_itr_get_object(fmt->itr, &key, &val)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    if (! qn_json_itr_push_object(fmt->itr, val, '}')) return qn_false;

    fmt->with_key = 1;
    fmt->tkn = QN_JSON_FMT_OPENING_BRACE;
    return qn_true;
}

static qn_bool qn_json_fmt_push_array(qn_json_formatter_ptr restrict fmt)
{
    qn_string key = NULL;
    qn_json_array_ptr val = NULL;

    assert(fmt->tkn == QN_JSON_FMT_ARRAY); 

    if (! qn_json_itr_get_array(fmt->itr, &key, &val)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    if (! qn_json_itr_push_array(fmt->itr, val, ']')) return qn_false;

    fmt->with_key = 1;
    fmt->tkn = QN_JSON_FMT_OPENING_BRACKET;
    return qn_true;
}

static qn_bool qn_json_fmt_output_string(qn_json_formatter_ptr restrict fmt)
{
    qn_string key = NULL;
    qn_string val = NULL;

    assert(fmt->tkn == QN_JSON_FMT_STRING); 

    if (! qn_json_itr_get_string(fmt->itr, &key, &val)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    fmt->tkn = QN_JSON_FMT_COMMA;
    qn_json_itr_advance(fmt->itr);

    fmt->string = val;
    fmt->string_pos = 0;
    if (! qn_json_fmt_serialize_string(fmt)) return qn_false;

    fmt->string = NULL;
    fmt->string_pos = 0;
    return qn_true;
}

static qn_bool qn_json_fmt_output_integer(qn_json_formatter_ptr restrict fmt)
{
    int ret = 0;
    qn_string key = NULL;
    qn_json_integer val = 0;
    qn_size free_size = 0;

    assert(fmt->tkn == QN_JSON_FMT_INTEGER); 

    if (! qn_json_itr_get_integer(fmt->itr, &key, &val)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    free_size = fmt->buf_capacity - fmt->buf_size;

#if defined(QN_CFG_SMALL_NUMBERS)
    ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%d", val);
#else
    ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%lld", val);
#endif

    if (ret < 0) {
        /* TODO : Set appropriate errors to accord to each system error accurately. */
        qn_err_set_try_again();
        return qn_false;
    }

    if (ret >= free_size) {
        qn_err_set_out_of_buffer();
        return qn_false;
    }

    /* All the output, include the final NUL character, has been written into the buffer. */
    fmt->buf_size += ret;

    fmt->tkn = QN_JSON_FMT_COMMA;
    qn_json_itr_advance(fmt->itr);
    return qn_true;
}

static qn_bool qn_json_fmt_output_number(qn_json_formatter_ptr restrict fmt)
{
    int ret = 0;
    qn_string key = NULL;
    qn_json_number val = 0.0;
    qn_size free_size = 0;

    assert(fmt->tkn == QN_JSON_FMT_NUMBER); 

    if (! qn_json_itr_get_number(fmt->itr, &key, &val)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    free_size = fmt->buf_capacity - fmt->buf_size;

#if defined(QN_CFG_SMALL_NUMBERS)
    ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%f", (double)val);
#else
    /* NOTE: %Lf is a synonym for %llf in C99. */
    /* ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%Lf", fmt->val.number); */
    ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%f", val);
#endif

    if (ret < 0) {
        /* TODO : Set appropriate errors to accord to each system error accurately. */
        qn_err_set_try_again();
        return qn_false;
    }

    if (ret >= free_size) {
        qn_err_set_out_of_buffer();
        return qn_false;
    }

    /* All the output, include the final NUL character, has been written into the buffer. */
    fmt->buf_size += ret;

    fmt->tkn = QN_JSON_FMT_COMMA;
    qn_json_itr_advance(fmt->itr);
    return qn_true;
}

static qn_bool qn_json_fmt_output_boolean(qn_json_formatter_ptr restrict fmt)
{
    qn_string key = NULL;
    qn_bool val = NULL;
    qn_size free_size = 0;

    assert(fmt->tkn == QN_JSON_FMT_BOOLEAN); 

    if (! qn_json_itr_get_boolean(fmt->itr, &key, &val)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    free_size = fmt->buf_capacity - fmt->buf_size;

    if (val) {
        if (free_size < 4) {
            qn_err_set_out_of_buffer();
            return qn_false;
        }
        memcpy(fmt->buf + fmt->buf_size, "true", 4);
        fmt->buf_size += 4;
    } else {            
        if (free_size < 5) {
            qn_err_set_out_of_buffer();
            return qn_false;
        }
        memcpy(fmt->buf + fmt->buf_size, "false", 5);
        fmt->buf_size += 5;
    } /* if */

    fmt->tkn = QN_JSON_FMT_COMMA;
    qn_json_itr_advance(fmt->itr);
    return qn_true;
}

static qn_bool qn_json_fmt_output_null(qn_json_formatter_ptr restrict fmt)
{
    qn_string key = NULL;
    qn_size free_size = 0;

    assert(fmt->tkn == QN_JSON_FMT_NULL); 

    if (! qn_json_itr_get_null(fmt->itr, &key)) return qn_false;
    if (fmt->with_key && key) return qn_json_fmt_output_key(fmt, key);

    if ((free_size = fmt->buf_capacity - fmt->buf_size) < 4) {
        qn_err_set_out_of_buffer();
        return qn_false;
    }

    memcpy(fmt->buf + fmt->buf_size, "null", 4);
    fmt->buf_size += 4;

    fmt->tkn = QN_JSON_FMT_COMMA;
    qn_json_itr_advance(fmt->itr);
    return qn_true;
}

static qn_uint32 qn_json_fmt_next_token(qn_json_formatter_ptr restrict fmt)
{
    switch (qn_json_itr_get_type(fmt->itr)) {
        case QN_JSON_OBJECT: return QN_JSON_FMT_OBJECT;
        case QN_JSON_ARRAY: return QN_JSON_FMT_ARRAY;
        case QN_JSON_STRING: return QN_JSON_FMT_STRING;
        case QN_JSON_INTEGER: return QN_JSON_FMT_INTEGER;
        case QN_JSON_NUMBER: return QN_JSON_FMT_NUMBER;
        case QN_JSON_BOOLEAN: return QN_JSON_FMT_BOOLEAN;
        case QN_JSON_NULL: return QN_JSON_FMT_NULL;
        default: break;
    } /* switch */
    return QN_JSON_FMT_ERROR;
}

static qn_bool qn_json_fmt_format(qn_json_formatter_ptr restrict fmt, char * restrict buf, qn_size * restrict buf_size)
{
    if (fmt->string && ! qn_json_fmt_serialize_string(fmt)) goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;

    while (fmt->tkn != QN_JSON_FMT_END) {
        switch (fmt->tkn) {
            case QN_JSON_FMT_COMMA:
                if (qn_json_itr_has_next_one(fmt->itr)) {
                    if (! qn_json_fmt_putc(fmt, ',')) goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;
                    fmt->with_key = 1;
                    fmt->tkn = qn_json_fmt_next_token(fmt);
                    continue;
                }
                fmt->tkn = QN_JSON_FMT_CLOSING;

            case QN_JSON_FMT_CLOSING:
                if (! qn_json_fmt_putc(fmt, qn_json_itr_get_top_status(fmt->itr) & 0xFF)) goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;
                qn_json_itr_pop(fmt->itr);

                if (qn_json_itr_is_empty(fmt->itr)) {
                    fmt->tkn = QN_JSON_FMT_END;
                } else {
                    qn_json_itr_advance(fmt->itr);
                    fmt->tkn = QN_JSON_FMT_COMMA;
                }
                break;

            case QN_JSON_FMT_COLON:
                if (! qn_json_fmt_putc(fmt, ':')) goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;
                fmt->tkn = qn_json_fmt_next_token(fmt);
                break;

            case QN_JSON_FMT_OPENING_BRACE:
                if (! qn_json_fmt_putc(fmt, '{')) goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;
                fmt->tkn = qn_json_itr_has_next_one(fmt->itr) ? qn_json_fmt_next_token(fmt) : QN_JSON_FMT_CLOSING;
                break;

            case QN_JSON_FMT_OPENING_BRACKET:
                if (! qn_json_fmt_putc(fmt, '[')) goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;
                fmt->tkn = qn_json_itr_has_next_one(fmt->itr) ? qn_json_fmt_next_token(fmt) : QN_JSON_FMT_CLOSING;
                break;

            case QN_JSON_FMT_OBJECT: if (! qn_json_fmt_push_object(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;
            case QN_JSON_FMT_ARRAY: if (! qn_json_fmt_push_array(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;
            case QN_JSON_FMT_STRING: if (! qn_json_fmt_output_string(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;
            case QN_JSON_FMT_BOOLEAN: if (! qn_json_fmt_output_boolean(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;
            case QN_JSON_FMT_INTEGER: if (! qn_json_fmt_output_integer(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;
            case QN_JSON_FMT_NUMBER: if (! qn_json_fmt_output_number(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;
            case QN_JSON_FMT_NULL: if (! qn_json_fmt_output_null(fmt)) { goto QN_JSON_FMT_FORMAT_ERROR_HANDLING; } break;

            default:
                /* TODO: Set appropriate error. */
                goto QN_JSON_FMT_FORMAT_ERROR_HANDLING;
        } /* switch */
    } /* while */

    *buf_size = fmt->buf_size;
    fmt->tkn = QN_JSON_FMT_START;
    return qn_true;

QN_JSON_FMT_FORMAT_ERROR_HANDLING:
    *buf_size = fmt->buf_size;
    return qn_false;
}

QN_SDK qn_bool qn_json_fmt_format_object(qn_json_formatter_ptr restrict fmt, qn_json_object_ptr restrict root, char * restrict buf, qn_size * restrict buf_size)
{
    fmt->buf = buf;
    fmt->buf_size = 0;
    fmt->buf_capacity = *buf_size;

    if (fmt->tkn == QN_JSON_FMT_START) {
        if (! qn_json_itr_start_with_object(fmt->itr, root, '}')) {
            *buf_size = 0;
            return qn_false;
        }
        fmt->with_key = 1;
        fmt->tkn = QN_JSON_FMT_OPENING_BRACE;
    }
    return qn_json_fmt_format(fmt, buf, buf_size);
}

QN_SDK qn_bool qn_json_fmt_format_array(qn_json_formatter_ptr restrict fmt, qn_json_array_ptr restrict root, char * restrict buf, qn_size * restrict buf_size)
{
    fmt->buf = buf;
    fmt->buf_size = 0;
    fmt->buf_capacity = *buf_size;

    if (fmt->tkn == QN_JSON_FMT_START) {
        if (! qn_json_itr_start_with_array(fmt->itr, root, ']')) {
            *buf_size = 0;
            return qn_false;
        }
        fmt->with_key = 1;
        fmt->tkn = QN_JSON_FMT_OPENING_BRACKET;
    }
    return qn_json_fmt_format(fmt, buf, buf_size);
}

QN_SDK qn_string qn_json_object_to_string(qn_json_object_ptr restrict root)
{
    qn_json_formatter_ptr fmt = NULL;
    char * buf;
    char * new_buf;
    qn_size capacity = 4096;
    qn_size new_capacity;
    qn_size size = capacity;
    qn_size final_size = 0;

    if (!root) return qn_str_empty_string;

    fmt = qn_json_fmt_create();
    if (!fmt) return NULL;

    buf = malloc(capacity);
    if (!buf) {
        qn_json_fmt_destroy(fmt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    while (!qn_json_fmt_format_object(fmt, root, buf + final_size, &size)) {
        if (qn_err_is_out_of_buffer()) {
            final_size += size;

            new_capacity = capacity + (capacity >> 1);
            new_buf = malloc(new_capacity);
            if (!new_buf) {
                free(buf);
                qn_json_fmt_destroy(fmt);
                qn_err_set_out_of_memory();
                return NULL;
            } // if

            memcpy(new_buf, buf, final_size);
            free(buf);
            buf = new_buf;
            capacity = new_capacity;
            size = new_capacity - final_size - 1;
        } else {
            free(buf);
            qn_json_fmt_destroy(fmt);
            return NULL;
        } // if
    } // while
    final_size += size;
    qn_json_fmt_destroy(fmt);

    buf[final_size] = '\0';
    return buf;
}

QN_SDK qn_string qn_json_array_to_string(qn_json_array_ptr restrict root)
{
    qn_json_formatter_ptr fmt = NULL;
    char * buf;
    char * new_buf;
    qn_size capacity = 4096;
    qn_size new_capacity;
    qn_size size = capacity;
    qn_size final_size = 0;

    if (!root) return qn_str_empty_string;

    fmt = qn_json_fmt_create();
    if (!fmt) return NULL;

    buf = malloc(capacity);
    if (!buf) {
        qn_json_fmt_destroy(fmt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    while (!qn_json_fmt_format_array(fmt, root, buf + final_size, &size)) {
        if (qn_err_is_out_of_buffer()) {
            final_size += size;

            new_capacity = capacity + (capacity >> 1);
            new_buf = malloc(new_capacity);
            if (!new_buf) {
                free(buf);
                qn_json_fmt_destroy(fmt);
                qn_err_set_out_of_memory();
                return NULL;
            } // if

            memcpy(new_buf, buf, final_size);
            free(buf);
            buf = new_buf;
            capacity = new_capacity;
            size = new_capacity - final_size - 1;
        } else {
            free(buf);
            qn_json_fmt_destroy(fmt);
            return NULL;
        } // if
    } // while
    final_size += size;
    qn_json_fmt_destroy(fmt);

    buf[final_size] = '\0';
    return buf;
}

#ifdef __cplusplus
}
#endif
