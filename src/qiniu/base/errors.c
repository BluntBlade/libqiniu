#include <stdlib.h>

#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
    QN_ERR_SUCCEED = 0,
    QN_ERR_OUT_OF_MEMORY = 1001,
    QN_ERR_TRY_AGAIN = 1002,
    QN_ERR_INVALID_ARGUMENT = 1003,
    QN_ERR_OVERFLOW_UPPER_BOUND = 1004,
    QN_ERR_OVERFLOW_LOWER_BOUND = 1005,
    QN_ERR_BAD_UTF8_SEQUENCE = 1006,
    QN_ERR_OUT_OF_BUFFER = 1007,
    QN_ERR_OUT_OF_CAPACITY = 1008,
    QN_ERR_NO_SUCH_ENTRY = 1009,

    QN_ERR_JSON_BAD_TEXT_INPUT  = 2001,
    QN_ERR_JSON_TOO_MANY_PARSING_LEVELS = 2002,
    QN_ERR_JSON_NEED_MORE_TEXT_INPUT = 2003,
    QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT = 2004,
    QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY = 2005,

    QN_ERR_HTTP_INVALID_HEADER_SYNTAX = 3001,
    QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED = 3002,
    QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED = 3003,
    QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED = 3004,

    QN_ERR_FL_OPENING_FILE_FAILED = 11001,
    QN_ERR_FL_DUPLICATING_FILE_FAILED = 11002,
    QN_ERR_FL_READING_FILE_FAILED = 11003,
    QN_ERR_FL_SEEKING_FILE_FAILED = 11004,
    QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED = 11101,

    QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION = 21001,
    QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION = 21002,
    QN_ERR_STOR_INVALID_LIST_RESULT = 21003,
    QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_PRE_CALLBACK = 21004,
    QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_POST_CALLBACK = 21005,
    QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT = 21006,
    QN_ERR_STOR_API_RETURN_NO_VALUE = 21007,

    QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED = 22001,
    QN_ERR_ETAG_UPDATING_CONTEXT_FAILED = 22002,
    QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED = 22003,
    QN_ERR_ETAG_UPDATING_BLOCK_FAILED = 22004,
    QN_ERR_ETAG_MAKING_DIGEST_FAILED = 22005
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
    {QN_ERR_OUT_OF_MEMORY, "Out of memory"},
    {QN_ERR_TRY_AGAIN, "Operation would be blocked, try again"},
    {QN_ERR_INVALID_ARGUMENT, "Invalid argument is passed"},
    {QN_ERR_OVERFLOW_UPPER_BOUND, "Integer value is overflow the upper bound"},
    {QN_ERR_OVERFLOW_LOWER_BOUND, "Integer value is overflow the lower bound"},
    {QN_ERR_BAD_UTF8_SEQUENCE, "The string contains a bad UTF-8 sequence"},
    {QN_ERR_OUT_OF_BUFFER, "Out of buffer"},
    {QN_ERR_OUT_OF_CAPACITY, "Out of capacity"},
    {QN_ERR_NO_SUCH_ENTRY, "No such entry to the specified key or name"},

    {QN_ERR_JSON_BAD_TEXT_INPUT, "Bad text input of a JSON string is read"},
    {QN_ERR_JSON_TOO_MANY_PARSING_LEVELS, "Parsing too many levels in a piece of JSON text"},
    {QN_ERR_JSON_NEED_MORE_TEXT_INPUT, "Need more text input to parse a JSON object or array"},
    {QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT, "Modifying an immutable JSON object"},
    {QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY, "Modifying an immutable JSON array"},

    {QN_ERR_HTTP_INVALID_HEADER_SYNTAX, "Invalid HTTP header syntax"},
    {QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED, "Adding string field to HTTP form failed"},
    {QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED, "Adding file field to HTTP form failed"},
    {QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED, "Adding buffer field to HTTP form failed"},

    {QN_ERR_FL_OPENING_FILE_FAILED, "Opening file failed"},
    {QN_ERR_FL_DUPLICATING_FILE_FAILED, "Duplicating file failed"},
    {QN_ERR_FL_READING_FILE_FAILED, "Reading file failed"},
    {QN_ERR_FL_SEEKING_FILE_FAILED, "Seeking file failed"},
    {QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED, "Stating file infomation failed"},

    {QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION, "Lack of auhorization information like token or put policy"},
    {QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION, "Invalid resumable session information"},
    {QN_ERR_STOR_INVALID_LIST_RESULT, "Invalid list result"},
    {QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_PRE_CALLBACK, "Putting file is aborted by filter PRE callback"},
    {QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_POST_CALLBACK, "Putting file is aborted by filter POST callback"},
    {QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT, "Invalid chunk put result"},
    {QN_ERR_STOR_API_RETURN_NO_VALUE, "API return no value"},

    {QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED, "Failed in initializing a new qetag context"},
    {QN_ERR_ETAG_UPDATING_CONTEXT_FAILED, "Failed in updating the qetag context"},
    {QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED, "Failed in initializing a new qetag block"},
    {QN_ERR_ETAG_UPDATING_BLOCK_FAILED, "Failed in updating the qetag block"},
    {QN_ERR_ETAG_MAKING_DIGEST_FAILED, "Failed in making the qetag digest"}
};

static int qn_err_compare(const void * restrict key, const void * restrict item)
{
    if (*((qn_err_enum *)key) < ((qn_error_ptr)item)->code) {
        return -1;
    } // if
    if (*((qn_err_enum *)key) > ((qn_error_ptr)item)->code) {
        return 1;
    } // if
    return 0;
}

QN_API const char * qn_err_get_message(void)
{
    qn_error_ptr error = (qn_error_ptr) bsearch(&qn_err_code, &qn_errors, sizeof(qn_errors) / sizeof(qn_errors[0]), sizeof(qn_errors[0]), &qn_err_compare);
    return error->message;
}

QN_API void qn_err_set_succeed(void)
{
    qn_err_code = QN_ERR_SUCCEED;
}

QN_API void qn_err_set_out_of_memory(void)
{
    qn_err_code = QN_ERR_OUT_OF_MEMORY;
}

QN_API void qn_err_set_try_again(void)
{
    qn_err_code = QN_ERR_TRY_AGAIN;
}

QN_API void qn_err_set_invalid_argument(void)
{
    qn_err_code = QN_ERR_INVALID_ARGUMENT;
}

QN_API void qn_err_set_overflow_upper_bound(void)
{
    qn_err_code = QN_ERR_OVERFLOW_UPPER_BOUND;
}

QN_API void qn_err_set_overflow_lower_bound(void)
{
    qn_err_code = QN_ERR_OVERFLOW_LOWER_BOUND;
}

QN_API void qn_err_set_bad_utf8_sequence(void)
{
    qn_err_code = QN_ERR_BAD_UTF8_SEQUENCE;
}

QN_API void qn_err_set_out_of_buffer(void)
{
    qn_err_code = QN_ERR_OUT_OF_BUFFER;
}

QN_API void qn_err_set_out_of_capacity(void)
{
    qn_err_code = QN_ERR_OUT_OF_CAPACITY;
}

QN_API void qn_err_set_no_such_entry(void)
{
    qn_err_code = QN_ERR_NO_SUCH_ENTRY;
}

QN_API void qn_err_json_set_bad_text_input(void)
{
    qn_err_code = QN_ERR_JSON_BAD_TEXT_INPUT;
}

QN_API void qn_err_json_set_too_many_parsing_levels(void)
{
    qn_err_code = QN_ERR_JSON_TOO_MANY_PARSING_LEVELS;
}

QN_API void qn_err_json_set_need_more_text_input(void)
{
    qn_err_code = QN_ERR_JSON_NEED_MORE_TEXT_INPUT;
}

QN_API void qn_err_json_set_modifying_immutable_object(void)
{
    qn_err_code = QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT;
}

QN_API void qn_err_json_set_modifying_immutable_array(void)
{
    qn_err_code = QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY;
}

QN_API void qn_err_http_set_invalid_header_syntax(void)
{
    qn_err_code = QN_ERR_HTTP_INVALID_HEADER_SYNTAX;
}

QN_API void qn_err_http_set_adding_string_field_failed(void)
{
    qn_err_code = QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED;
}

QN_API void qn_err_http_set_adding_file_field_failed(void)
{
    qn_err_code = QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED;
}

QN_API void qn_err_http_set_adding_buffer_field_failed(void)
{
    qn_err_code = QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED;
}

QN_API void qn_err_fl_set_opening_file_failed(void)
{
    qn_err_code = QN_ERR_FL_OPENING_FILE_FAILED;
}

QN_API void qn_err_fl_set_duplicating_file_failed(void)
{
    qn_err_code = QN_ERR_FL_DUPLICATING_FILE_FAILED;
}

QN_API void qn_err_fl_set_reading_file_failed(void)
{
    qn_err_code = QN_ERR_FL_READING_FILE_FAILED;
}

QN_API void qn_err_fl_set_seeking_file_failed(void)
{
    qn_err_code = QN_ERR_FL_SEEKING_FILE_FAILED;
}

QN_API void qn_err_fl_info_set_stating_file_info_failed(void)
{
    qn_err_code = QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED;
}

QN_API void qn_err_stor_set_lack_of_authorization_information(void)
{
    qn_err_code = QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION;
}

QN_API void qn_err_stor_set_invalid_resumable_session_information(void)
{
    qn_err_code = QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION;
}

QN_API void qn_err_stor_set_invalid_list_result(void)
{
    qn_err_code = QN_ERR_STOR_INVALID_LIST_RESULT;
}

QN_API void qn_err_stor_set_putting_aborted_by_filter_pre_callback(void)
{
    qn_err_code = QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_PRE_CALLBACK;
}

QN_API void qn_err_stor_set_putting_aborted_by_filter_post_callback(void)
{
    qn_err_code = QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_POST_CALLBACK;
}

QN_API void qn_err_stor_set_invalid_chunk_put_result(void)
{
    qn_err_code = QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT;
}

QN_API void qn_err_stor_set_api_return_no_value(void)
{
    qn_err_code = QN_ERR_STOR_API_RETURN_NO_VALUE;
}

QN_API void qn_err_etag_set_initializing_context_failed(void)
{
    qn_err_code = QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED;
}

QN_API void qn_err_etag_set_updating_context_failed(void)
{
    qn_err_code = QN_ERR_ETAG_UPDATING_CONTEXT_FAILED;
}

QN_API void qn_err_etag_set_initializing_block_failed(void)
{
    qn_err_code = QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED;
}

QN_API void qn_err_etag_set_updating_block_failed(void)
{
    qn_err_code = QN_ERR_ETAG_UPDATING_BLOCK_FAILED;
}

QN_API void qn_err_etag_set_making_digest_failed(void)
{
    qn_err_code = QN_ERR_ETAG_MAKING_DIGEST_FAILED;
}

// ----

QN_API qn_bool qn_err_is_succeed(void)
{
    return (qn_err_code == QN_ERR_SUCCEED);
}

QN_API qn_bool qn_err_is_out_of_memory(void)
{
    return (qn_err_code == QN_ERR_OUT_OF_MEMORY);
}

QN_API qn_bool qn_err_is_try_again(void)
{
    return (qn_err_code == QN_ERR_TRY_AGAIN);
}

QN_API qn_bool qn_err_is_invalid_argument(void)
{
    return (qn_err_code == QN_ERR_TRY_AGAIN);
}

QN_API qn_bool qn_err_is_overflow_upper_bound(void)
{
    return (qn_err_code == QN_ERR_OVERFLOW_UPPER_BOUND);
}

QN_API qn_bool qn_err_is_overflow_lower_bound(void)
{
    return (qn_err_code == QN_ERR_OVERFLOW_LOWER_BOUND);
}

QN_API qn_bool qn_err_is_bad_utf8_sequence(void)
{
    return (qn_err_code == QN_ERR_BAD_UTF8_SEQUENCE);
}

QN_API qn_bool qn_err_is_out_of_buffer(void)
{
    return (qn_err_code == QN_ERR_OUT_OF_BUFFER);
}

QN_API qn_bool qn_err_is_out_of_capacity(void)
{
    return (qn_err_code == QN_ERR_OUT_OF_CAPACITY);
}

QN_API qn_bool qn_err_is_no_such_entry(void)
{
    return (qn_err_code == QN_ERR_NO_SUCH_ENTRY);
}

QN_API qn_bool qn_err_json_is_bad_text_input(void)
{
    return (qn_err_code == QN_ERR_JSON_BAD_TEXT_INPUT);
}

QN_API qn_bool qn_err_json_is_too_many_parsing_levels(void)
{
    return (qn_err_code == QN_ERR_JSON_TOO_MANY_PARSING_LEVELS);
}

QN_API qn_bool qn_err_json_is_need_more_text_input(void)
{
    return (qn_err_code == QN_ERR_JSON_NEED_MORE_TEXT_INPUT);
}

QN_API qn_bool qn_err_json_is_modifying_immutable_object(void)
{
    return (qn_err_code == QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT);
}

QN_API qn_bool qn_err_json_is_modifying_immutable_array(void)
{
    return (qn_err_code == QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY);
}

QN_API qn_bool qn_err_http_is_invalid_header_syntax(void)
{
    return (qn_err_code == QN_ERR_HTTP_INVALID_HEADER_SYNTAX);
}

QN_API qn_bool qn_err_http_is_adding_string_field_failed(void)
{
    return (qn_err_code == QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED);
}

QN_API qn_bool qn_err_http_is_adding_file_field_failed(void)
{
    return (qn_err_code == QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED);
}

QN_API qn_bool qn_err_http_is_adding_buffer_field_failed(void)
{
    return (qn_err_code == QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED);
}

QN_API qn_bool qn_err_fl_is_opening_file_failed(void)
{
    return (qn_err_code == QN_ERR_FL_OPENING_FILE_FAILED);
}

QN_API qn_bool qn_err_fl_is_duplicating_file_failed(void)
{
    return (qn_err_code == QN_ERR_FL_DUPLICATING_FILE_FAILED);
}

QN_API qn_bool qn_err_fl_is_reading_file_failed(void)
{
    return (qn_err_code == QN_ERR_FL_READING_FILE_FAILED);
}

QN_API qn_bool qn_err_fl_is_seeking_file_failed(void)
{
    return (qn_err_code == QN_ERR_FL_SEEKING_FILE_FAILED);
}

QN_API qn_bool qn_err_fl_info_is_stating_file_info_failed(void)
{
    return (qn_err_code == QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED);
}

QN_API qn_bool qn_err_stor_is_lack_of_authorization_information(void)
{
    return (qn_err_code == QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION);
}

QN_API qn_bool qn_err_stor_is_invalid_resumable_session_information(void)
{
    return (qn_err_code == QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION);
}

QN_API qn_bool qn_err_stor_is_invalid_list_result(void)
{
    return (qn_err_code == QN_ERR_STOR_INVALID_LIST_RESULT);
}

QN_API qn_bool qn_err_stor_is_putting_aborted_by_filter_pre_callback(void)
{
    return (qn_err_code == QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_PRE_CALLBACK);
}

QN_API qn_bool qn_err_stor_is_putting_aborted_by_filter_post_callback(void)
{
    return (qn_err_code == QN_ERR_STOR_PUTTING_ABORTED_BY_FILTER_POST_CALLBACK);
}

QN_API qn_bool qn_err_stor_is_invalid_chunk_put_result(void)
{
    return (qn_err_code == QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT);
}

QN_API qn_bool qn_err_stor_is_api_return_no_value(void)
{
    return (qn_err_code == QN_ERR_STOR_API_RETURN_NO_VALUE);
}

QN_API qn_bool qn_err_etag_is_initializing_context_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED);
}

QN_API qn_bool qn_err_etag_is_updating_context_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_UPDATING_CONTEXT_FAILED);
}

QN_API qn_bool qn_err_etag_is_initializing_block_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED);
}

QN_API qn_bool qn_err_etag_is_updating_block_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_UPDATING_BLOCK_FAILED);
}

QN_API qn_bool qn_err_etag_is_making_digest_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_MAKING_DIGEST_FAILED);
}

#ifdef __cplusplus
}
#endif
