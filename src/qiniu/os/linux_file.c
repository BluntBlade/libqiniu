#include "qiniu/os/file.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#if defined(QN_CFG_LARGE_FILE_SUPPORT_AWARE)
#include <dlfcn.h>
#endif

#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of file info ----

typedef struct _QN_FL_INFO
{
    qn_string fname;
    qn_fsize fsize;
} qn_file_info_st;

QN_SDK void qn_fl_info_destroy(qn_fl_info_ptr restrict fi)
{
    if (fi) {
        qn_str_destroy(fi->fname);
        free(fi);
    } // fi
}

QN_SDK qn_fsize qn_fl_info_fsize(qn_fl_info_ptr restrict fi)
{
    return fi->fsize;
}

QN_SDK qn_string qn_fl_info_fname(qn_fl_info_ptr restrict fi)
{
    return fi->fname;
}

static qn_fl_info_ptr qn_fl_info_duplicate(qn_fl_info_ptr restrict fi)
{
    qn_fl_info_ptr new_fi = calloc(1, sizeof(qn_file_info_st));
    if (! new_fi) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_fi->fname = qn_cs_duplicate(fi->fname);
    if (! new_fi->fname) {
        free(new_fi);
        return NULL;
    } // if

    new_fi->fsize = fi->fsize;
    return new_fi;
}

// ---- File Functions (abbreviation: fl) ----

#if defined(QN_CFG_LARGE_FILE_SUPPORT_AWARE)

static qn_foffset qn_fl_lseek_loader(int fd, qn_foffset offset, int whence);

typedef qn_foffset (*qn_fl_lseek_fn)(int, qn_foffset, int);
qn_fl_lseek_fn qn_fl_lseek_wrapper = &qn_fl_lseek_loader;

static qn_foffset qn_fl_lseek(int fd, qn_foffset offset, int whence)
{
    if (0xFFFFFFFFL < offset) {
        errno = EINVAL;
        return -1;
    } // if
    return lseek(fd, (off_t)(offset & 0xFFFFFFFFL), whence);
}

static qn_foffset qn_fl_lseek_loader(int fd, qn_foffset offset, int whence)
{
    void * libc = NULL;
    void * posix_lseek64 = NULL;

    libc = dlopen("libc.so.6", RTLD_LAZY);
    if (! libc) {
        libc = dlopen("libc.so", RTLD_LAZY);
        if (! libc) {
            dlclose(libc);
            return -1;
        } // if
    } // if

    posix_lseek64 = dlsym(libc, "lseek64");
    dlclose(libc);
    qn_fl_lseek_wrapper = (posix_lseek64) ? (qn_fl_lseek_fn)(posix_lseek64) : &qn_fl_lseek;
    return (*qn_fl_lseek_wrapper)(fd, offset, whence);
}

#else

static inline qn_foffset qn_fl_lseek_wrapper(int fd, qn_foffset offset, int whence)
{
#if defined(QN_CFG_LARGE_FILE_SUPPORT)
    return lseek64(fd, offset, whence);
#else
    if (sizeof(offset) == 8 && 0xFFFFFFFFL < offset) {
        errno = EINVAL;
        return -1;
    } // if
    return lseek(fd, (off_t)(offset & 0xFFFFFFFFL), whence);
#endif
}

#endif

struct _QN_FILE
{
    qn_io_reader_ptr rdr_vtbl;
    qn_fl_info_ptr fi;
    int fd;
} qn_file_st;

static inline qn_file_ptr qn_fl_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_file_ptr)( ( (char *) itf ) - (char *)( &((qn_file_ptr)0)->rdr_vtbl ) );
}

static void qn_fl_rdr_close_vfn(qn_io_reader_itf restrict itf)
{
    qn_fl_close(qn_fl_from_io_reader(itf));
}

static ssize_t qn_fl_rdr_peek_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_peek(qn_fl_from_io_reader(itf), buf, buf_size);
}

static ssize_t qn_fl_rdr_read_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_read(qn_fl_from_io_reader(itf), buf, buf_size);
}

static qn_bool qn_fl_rdr_seek_vfn(qn_io_reader_itf restrict itf, qn_foffset offset)
{
    return qn_fl_seek(qn_fl_from_io_reader(itf), offset);
}

static qn_bool qn_fl_rdr_advance_vfn(qn_io_reader_itf restrict itf, qn_foffset delta)
{
    return qn_fl_advance(qn_fl_from_io_reader(itf), delta);
}

static qn_string qn_fl_rdr_name_vfn(qn_io_reader_itf restrict itf)
{
    return qn_fl_fname(qn_fl_from_io_reader(itf));
}

static qn_fsize qn_fl_rdr_size_vfn(qn_io_reader_itf restrict itf)
{
    return qn_fl_fsize(qn_fl_from_io_reader(itf));
}

static qn_io_reader_itf qn_fl_rdr_duplicate_vfn(qn_io_reader_itf restrict itf)
{
    qn_file_ptr new_file = qn_fl_duplicate(qn_fl_from_io_reader(itf));
    if (!new_file) return NULL;
    return qn_fl_to_io_reader(new_file);
}

static qn_io_reader_itf qn_fl_rdr_section_vfn(qn_io_reader_itf restrict itf, qn_foffset offset, size_t sec_size)
{
    qn_fl_section_ptr new_file = qn_fl_section(qn_fl_from_io_reader(itf), offset, sec_size);
    if (!new_file) return NULL;
    return qn_fl_sec_to_io_reader(new_file);
}

static qn_io_reader_st qn_fl_rdr_vtable = {
    &qn_fl_rdr_close_vfn,
    &qn_fl_rdr_peek_vfn,
    &qn_fl_rdr_read_vfn,
    &qn_fl_rdr_seek_vfn,
    &qn_fl_rdr_advance_vfn,
    &qn_fl_rdr_duplicate_vfn,
    &qn_fl_rdr_section_vfn,
    &qn_fl_rdr_name_vfn,
    &qn_fl_rdr_size_vfn
};

QN_SDK qn_file_ptr qn_fl_open(const char * restrict fname, qn_fl_open_extra_ptr restrict extra)
{
    qn_file_ptr new_file = calloc(1, sizeof(qn_file_st));
    if (!new_file) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_file->fd = open(fname, 0);
    if (new_file->fd < 0) {
        free(new_file);
        qn_err_fl_set_opening_file_failed();
        return NULL;
    } // if

    new_file->fi = qn_fl_info_stat(fname);
    if (! new_file->fi) {
        free(new_file);
        return NULL;
    } // if

    new_file->rdr_vtbl = &qn_fl_rdr_vtable;
    return new_file;
}

QN_SDK qn_file_ptr qn_fl_duplicate(qn_file_ptr restrict fl)
{
    qn_file_ptr new_file = calloc(1, sizeof(qn_file_st));
    if (!new_file) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    
    new_file->fd = dup(fl->fd);
    if (new_file->fd < 0) {
        free(new_file);
        qn_err_fl_set_duplicating_file_failed();
        return NULL;
    } // if

    new_file->fi = qn_fl_info_duplicate(fl->fi);
    if (! new_file->fi) {
        free(new_file);
        return NULL;
    } // if

    new_file->rdr_vtbl = &qn_fl_rdr_vtable;
    return new_file;
}

QN_SDK void qn_fl_close(qn_file_ptr restrict fl)
{
    if (fl) {
        qn_fl_info_destroy(fl->fi);
        close(fl->fd);
        free(fl);
    } // if
}

QN_SDK qn_io_reader_itf qn_fl_to_io_reader(qn_file_ptr restrict fl)
{
    return &fl->rdr_vtbl;
}

QN_SDK qn_string qn_fl_fname(qn_file_ptr restrict fl)
{
    return fl->fi->fname;
}

QN_SDK qn_fsize qn_fl_fsize(qn_file_ptr restrict fl)
{
    return fl->fi->fsize;
}

QN_SDK ssize_t qn_fl_peek(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size)
{
    ssize_t ret = qn_fl_read(fl, buf, buf_size);
    if (ret >= 0 && !qn_fl_advance(fl, -ret)) return QN_IO_RDR_READING_FAILED;
    return ret;
}

QN_SDK ssize_t qn_fl_read(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size)
{
    ssize_t ret = read(fl->fd, buf, buf_size);
    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if
    return ret;
}

QN_SDK qn_bool qn_fl_seek(qn_file_ptr restrict fl, qn_foffset offset)
{
    if (qn_fl_lseek_wrapper(fl->fd, offset, SEEK_SET) < 0) {
        qn_err_fl_set_seeking_file_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_fl_advance(qn_file_ptr restrict fl, qn_foffset delta)
{
    if (qn_fl_lseek_wrapper(fl->fd, delta, SEEK_CUR) < 0) {
        qn_err_fl_set_seeking_file_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK ssize_t qn_fl_write(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size)
{
    ssize_t ret = write(fl->fd, buf, buf_size);
    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if
    return ret;
}

QN_SDK qn_fl_section_ptr qn_fl_section(qn_file_ptr restrict fl, qn_foffset offset, size_t sec_size)
{
    qn_fl_section_ptr new_sec = qn_fl_sec_create(fl, offset, sec_size);
    if (!new_sec) return NULL;
    return new_sec;
}

QN_SDK size_t qn_fl_reader_read_cfn(void * restrict user_data, char * restrict buf, size_t buf_size)
{
    qn_file_ptr fl = (qn_file_ptr) user_data;
    return qn_fl_read(fl, buf, buf_size);
}

// ---- Definition of file info depends on operating system ----

QN_SDK qn_fl_info_ptr qn_fl_info_stat(const char * restrict fname)
{
    struct stat st;

    qn_fl_info_ptr new_fi = calloc(1, sizeof(qn_file_info_st));
    if (!new_fi) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    if (stat(fname, &st) < 0) {
        qn_fl_info_destroy(new_fi);
        qn_err_fl_info_set_stating_file_info_failed();
        return NULL;
    } // if

    new_fi->fname = qn_cs_duplicate(fname);
    if (!new_fi->fname) {
        qn_fl_info_destroy(new_fi);
        return NULL;
    } // if
    new_fi->fsize = st.st_size;

    return new_fi;
}

// ---- Definition of file section depends on operating system ----

typedef struct _QN_FL_SECTION
{
    qn_io_reader_ptr rdr_vtbl;
    qn_file_ptr file;
    qn_foffset offset;
    size_t sec_size;
    size_t rem_size;
} qn_fl_section_st;

static inline qn_fl_section_ptr qn_fl_sec_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_fl_section_ptr)( ( (char *) itf ) - (char *)( &((qn_fl_section_ptr)0)->rdr_vtbl ) );
}

static void qn_fl_sec_rdr_close_vfn(qn_io_reader_itf restrict itf)
{
    qn_fl_sec_destroy(qn_fl_sec_from_io_reader(itf));
}

static ssize_t qn_fl_sec_rdr_peek_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_sec_peek(qn_fl_sec_from_io_reader(itf), buf, buf_size);
}

static ssize_t qn_fl_sec_rdr_read_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_sec_read(qn_fl_sec_from_io_reader(itf), buf, buf_size);
}

static qn_bool qn_fl_sec_rdr_seek_vfn(qn_io_reader_itf restrict itf, qn_foffset offset)
{
    return qn_fl_sec_seek(qn_fl_sec_from_io_reader(itf), offset);
}

static qn_bool qn_fl_sec_rdr_advance_vfn(qn_io_reader_itf restrict itf, qn_foffset delta)
{
    return qn_fl_sec_advance(qn_fl_sec_from_io_reader(itf), delta);
}

static qn_io_reader_itf qn_fl_sec_rdr_duplicate_vfn(qn_io_reader_itf restrict itf)
{
    qn_fl_section_ptr new_section = qn_fl_sec_duplicate(qn_fl_sec_from_io_reader(itf));
    if (!new_section) return NULL;
    return qn_fl_sec_to_io_reader(new_section);
}

static qn_io_reader_itf qn_fl_sec_rdr_section_vfn(qn_io_reader_itf restrict itf, qn_foffset offset, size_t sec_size)
{
    qn_fl_section_ptr new_section = qn_fl_sec_section(qn_fl_sec_from_io_reader(itf), offset, sec_size);
    if (!new_section) return NULL;
    return qn_fl_sec_to_io_reader(new_section);
}

static qn_io_reader_st qn_fl_sec_rdr_vtable = {
    &qn_fl_sec_rdr_close_vfn,
    &qn_fl_sec_rdr_peek_vfn,
    &qn_fl_sec_rdr_read_vfn,
    &qn_fl_sec_rdr_seek_vfn,
    &qn_fl_sec_rdr_advance_vfn,
    &qn_fl_sec_rdr_duplicate_vfn,
    &qn_fl_sec_rdr_section_vfn
};

QN_SDK qn_fl_section_ptr qn_fl_sec_create(qn_file_ptr restrict fl, qn_foffset offset, size_t sec_size)
{
    qn_fl_section_ptr new_section = calloc(1, sizeof(qn_fl_section_st));
    if (!new_section) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
    new_section->file = qn_fl_duplicate(fl);
#else
    new_section->file = fl;
#endif

    new_section->offset = offset;
    new_section->sec_size = sec_size;
    new_section->rem_size = sec_size;

    if (qn_fl_fsize(fl) <= offset) {
        qn_err_set_out_of_range();
        return NULL;
    } // if

    if (! qn_fl_sec_seek(new_section, offset)) {
        qn_fl_sec_destroy(new_section);
        return NULL;
    } // if

    new_section->rdr_vtbl = &qn_fl_sec_rdr_vtable;
    return new_section;
}

QN_SDK qn_fl_section_ptr qn_fl_sec_duplicate(qn_fl_section_ptr restrict fs)
{
    return qn_fl_sec_create(fs->file, fs->offset, fs->sec_size);
}

QN_SDK qn_fl_section_ptr qn_fl_sec_section(qn_fl_section_ptr restrict fs, qn_foffset offset, size_t sec_size)
{
    if (offset < fs->offset || offset + sec_size > fs->offset + fs->sec_size) return NULL;
    qn_fl_section_ptr new_section = qn_fl_sec_create(fs->file, offset, sec_size);
    if (!new_section) return NULL;
    return new_section;
}

QN_SDK void qn_fl_sec_destroy(qn_fl_section_ptr restrict fs)
{
    if (fs) {
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
        qn_fl_close(fs->file);
#endif
        free(fs);
    } // if
}

QN_SDK qn_bool qn_fl_sec_reset(qn_fl_section_ptr restrict fs)
{
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
    if (! qn_fl_seek(fs->file, fs->offset)) return qn_false;
#endif

    fs->rem_size = fs->sec_size;
    return qn_true;
}

QN_SDK qn_io_reader_itf qn_fl_sec_to_io_reader(qn_fl_section_ptr restrict fs)
{
    return &fs->rdr_vtbl;
}

QN_SDK ssize_t qn_fl_sec_peek(qn_fl_section_ptr restrict fs, char * restrict buf, size_t buf_size)
{
    ssize_t ret;
    size_t read_size;

    if (fs->rem_size == 0) return QN_IO_RDR_EOF;

    read_size = (buf_size < fs->rem_size) ? buf_size : fs->rem_size;

#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
    ret = read(fs->file->fd, buf, read_size); 
    if (ret > 0 && !qn_fl_sec_advance(fs, -read_size)) return QN_IO_RDR_READING_FAILED;
#else
    ret = pread(fs->file->fd, buf, read_size, fs->offset + (fs->sec_size - fs->rem_size));
#endif

    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if

    fs->rem_size -= ret;
    return ret;
}

QN_SDK ssize_t qn_fl_sec_read(qn_fl_section_ptr restrict fs, char * restrict buf, size_t buf_size)
{
    ssize_t ret;
    size_t read_size;

    if (fs->rem_size == 0) return QN_IO_RDR_EOF;

    read_size = (buf_size < fs->rem_size) ? buf_size : fs->rem_size;

#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
    ret = read(fs->file->fd, buf, read_size); 
#else
    ret = pread(fs->file->fd, buf, read_size, fs->offset + (fs->sec_size - fs->rem_size));
#endif

    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if

    fs->rem_size -= ret;
    return ret;
}

QN_SDK qn_bool qn_fl_sec_seek(qn_fl_section_ptr restrict fs, qn_foffset offset)
{
    if (offset < fs->offset) {
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
        if (! qn_fl_seek(fs->file, fs->offset)) return qn_false;
#endif
        fs->rem_size = fs->sec_size;
    } else if (offset > fs->offset + fs->sec_size) {
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
        if (! qn_fl_seek(fs->file, fs->offset + fs->sec_size)) return qn_false;
#endif
        fs->rem_size = 0;
    } else {
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
        if (! qn_fl_seek(fs->file, offset)) return qn_false;
#endif
        fs->rem_size = fs->offset + fs->sec_size - offset;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_fl_sec_advance(qn_fl_section_ptr restrict fs, qn_foffset delta)
{
    if (delta <= fs->rem_size) {
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
        if (! qn_fl_advance(fs->file, delta)) return qn_false;
#endif
        fs->rem_size -= delta;
    } else {
#if ! defined(QN_CFG_SHARED_FD_FOR_SECTIONS)
        if (! qn_fl_advance(fs->file, fs->rem_size)) return qn_false;
#endif
        fs->rem_size = 0;
    } // if
    return qn_true;
}

QN_SDK size_t qn_fl_sec_reader_read_cfn(void * restrict user_data, char * restrict buf, size_t buf_size)
{
    size_t ret;
    qn_fl_section_ptr fs = (qn_fl_section_ptr) user_data;
    if ((ret = qn_fl_sec_read(fs, buf, buf_size)) <= 0) return 0;
    return ret;
}

#ifdef __cplusplus
}
#endif
