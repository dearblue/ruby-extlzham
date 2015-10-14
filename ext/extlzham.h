#ifndef EXTLZHAM_H
#define EXTLZHAM_H 1

#include <stdint.h>
#include <ruby.h>
#include <ruby/thread.h>
#include <lzham.h>

#define RDOCFAKE(STMT)

extern VALUE mLZHAM;
extern VALUE eError;
extern VALUE cEncoder;
extern VALUE cDecoder;
extern VALUE mConsts;

extern ID id_op_lshift;
extern ID id_dictsize;
extern ID id_level;
extern ID id_table_update_rate;
extern ID id_threads;
extern ID id_flags;
extern ID id_table_max_update_interval;
extern ID id_table_update_interval_slow_rate;

enum {
    //WORKBUF_SIZE = 256 * 1024, /* 256 KiB */
    WORKBUF_SIZE = 1 << 20, /* 1 MiB */
    //WORKBUF_SIZE = 1 << 16, /* 64 KiB */
};

const char *extlzham_encode_status_str(lzham_compress_status_t status);
const char *extlzham_decode_status_str(lzham_decompress_status_t status);
void extlzham_encode_error(lzham_compress_status_t status);
void extlzham_decode_error(lzham_decompress_status_t status);

void extlzham_init_error(void);
void extlzham_init_constants(void);
void extlzham_init_encoder(void);
void extlzham_init_decoder(void);

static inline uint32_t
aux_getoptu32(VALUE hash, ID key, uint32_t defaultvalue)
{
    VALUE d = rb_hash_lookup2(hash, ID2SYM(key), Qundef);
    if (d == Qundef) { return defaultvalue; }
    return NUM2UINT(d);
}

static inline VALUE
aux_str_reserve(VALUE str, size_t size)
{
    if (size > rb_str_capacity(str)) {
        rb_str_modify_expand(str, size - RSTRING_LEN(str));
    } else {
        rb_str_modify(str);
    }

    return str;
}

#endif /* !EXTLZHAM_H */
