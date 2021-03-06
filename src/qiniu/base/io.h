#ifndef __QN_IO_H__
#define __QN_IO_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
    QN_IO_RDR_FILTERING_FAILED = -3,
    QN_IO_RDR_READING_ABORTED = -2,
    QN_IO_RDR_READING_FAILED = -1,
    QN_IO_RDR_EOF = 0
};

struct _QN_IO_READER;
typedef struct _QN_IO_READER * qn_io_reader_ptr;
typedef qn_io_reader_ptr * qn_io_reader_itf;

typedef void (*qn_io_rdr_close_virtual_fn)(qn_io_reader_itf restrict itf);
typedef ssize_t (*qn_io_rdr_peek_virtual_fn)(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size);
typedef ssize_t (*qn_io_rdr_read_virtual_fn)(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size);
typedef qn_bool (*qn_io_rdr_seek_virtual_fn)(qn_io_reader_itf restrict itf, qn_foffset offset);
typedef qn_bool (*qn_io_rdr_advance_virtual_fn)(qn_io_reader_itf restrict itf, qn_foffset delta);

typedef qn_io_reader_itf (*qn_io_rdr_duplicate_virtual_fn)(qn_io_reader_itf restrict itf);
typedef qn_io_reader_itf (*qn_io_rdr_section_virtual_fn)(qn_io_reader_itf restrict itf, qn_foffset offset, size_t sec_size);

typedef qn_string (*qn_io_rdr_name_virtual_fn)(qn_io_reader_itf restrict itf);
typedef qn_fsize (*qn_io_rdr_size_virtual_fn)(qn_io_reader_itf restrict itf);

typedef struct _QN_IO_READER
{
    qn_io_rdr_close_virtual_fn close;
    qn_io_rdr_peek_virtual_fn peek;
    qn_io_rdr_read_virtual_fn read;
    qn_io_rdr_seek_virtual_fn seek;
    qn_io_rdr_advance_virtual_fn advance;

    qn_io_rdr_duplicate_virtual_fn duplicate;
    qn_io_rdr_section_virtual_fn section;

    qn_io_rdr_name_virtual_fn name;
    qn_io_rdr_size_virtual_fn size;
} qn_io_reader_st;

static inline void qn_io_rdr_close(qn_io_reader_itf restrict itf)
{
    (*itf)->close(itf);
}

static inline ssize_t qn_io_rdr_peek(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return (*itf)->peek(itf, buf, buf_size);
}

static inline ssize_t qn_io_rdr_read(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return (*itf)->read(itf, buf, buf_size);
}

static inline qn_bool qn_io_rdr_seek(qn_io_reader_itf restrict itf, qn_foffset offset)
{
    return (*itf)->seek(itf, offset);
}

static inline qn_bool qn_io_rdr_advance(qn_io_reader_itf restrict itf, qn_foffset delta)
{
    return (*itf)->advance(itf, delta);
}

static inline qn_string qn_io_rdr_name(qn_io_reader_itf restrict itf)
{
    return (*itf)->name(itf);
}

static inline qn_fsize qn_io_rdr_size(qn_io_reader_itf restrict itf)
{
    return (*itf)->size(itf);
}

static inline qn_io_reader_itf qn_io_rdr_duplicate(qn_io_reader_itf restrict itf)
{
    return (*itf)->duplicate(itf);
}

static inline qn_io_reader_itf qn_io_rdr_section(qn_io_reader_itf restrict itf, qn_foffset offset, size_t sec_size)
{
    return (*itf)->section(itf, offset, sec_size);
}

// ----

struct _QN_IO_SECTION_READER;
typedef struct _QN_IO_SECTION_READER * qn_io_section_reader_ptr;

QN_SDK extern qn_io_section_reader_ptr qn_io_srdr_create(qn_io_reader_itf restrict src_rdr, size_t section_size);
QN_SDK extern void qn_io_srdr_destroy(qn_io_section_reader_ptr restrict srdr);

QN_SDK extern void qn_io_srdr_reset(qn_io_section_reader_ptr restrict srdr, qn_io_reader_itf restrict src_rdr, size_t section_size);
QN_SDK extern qn_io_reader_itf qn_io_srdr_to_io_reader(qn_io_section_reader_ptr restrict srdr);

// ----

struct _QN_IO_WRITER;
typedef struct _QN_IO_WRITER * qn_io_writer_ptr;
typedef qn_io_writer_ptr * qn_io_writer_itf;

typedef void (*qn_io_wrt_close_virtual_fn)(qn_io_writer_itf restrict itf);
typedef ssize_t (*qn_io_wrt_write_virtual_fn)(qn_io_writer_itf restrict itf, const char * restrict buf, size_t buf_size);

typedef qn_io_writer_itf (*qn_io_wrt_duplicate_virtual_fn)(qn_io_writer_itf restrict itf);

typedef struct _QN_IO_WRITER
{
    qn_io_wrt_close_virtual_fn close;
    qn_io_wrt_write_virtual_fn write;

    qn_io_wrt_duplicate_virtual_fn duplicate;
} qn_io_writer_st;

static inline void qn_io_wrt_close(qn_io_writer_itf restrict itf)
{
    (*itf)->close(itf);
}

static inline ssize_t qn_io_wrt_write(qn_io_writer_itf restrict itf, const char * restrict buf, size_t buf_size)
{
    return (*itf)->write(itf, buf, buf_size);
}

static inline qn_io_writer_itf qn_io_wrt_duplicate(qn_io_writer_itf restrict itf)
{
    return (*itf)->duplicate(itf);
}

#ifdef __cplusplus
}
#endif

#endif // __QN_IO_H__

