#ifndef EXTLZHAM_H
#define EXTLZHAM_H 1

#include <stdint.h>
#include <ruby.h>
#include <ruby/thread.h>
#include <lzham.h>

extern VALUE mLZHAM;
extern VALUE eError;
extern VALUE cEncoder;
extern VALUE cDecoder;
extern VALUE mConsts;

extern ID ID_op_lshift;
extern ID IDdictsize;
extern ID IDlevel;
extern ID IDtable_update_rate;
extern ID IDthreads;
extern ID IDflags;
extern ID IDtable_max_update_interval;
extern ID IDtable_update_interval_slow_rate;

enum {
    //WORKBUF_SIZE = 256 * 1024, /* 256 KiB */
    WORKBUF_SIZE = 1 << 20, /* 1 MiB */
    //WORKBUF_SIZE = 1 << 16, /* 64 KiB */
};

const char *aux_encode_status_str(lzham_compress_status_t status);
const char *aux_decode_status_str(lzham_decompress_status_t status);
void aux_encode_error(lzham_compress_status_t status);
void aux_decode_error(lzham_decompress_status_t status);

void init_error(void);
void init_constants(void);
void init_encoder(void);
void init_decoder(void);

static inline uint32_t
aux_hash_lookup_to_u32(VALUE hash, ID key, uint32_t defaultvalue)
{
    VALUE d = rb_hash_lookup2(hash, ID2SYM(key), Qundef);
    if (d == Qundef) { return defaultvalue; }
    return NUM2UINT(d);
}

static inline VALUE
aux_str_reserve(VALUE str, size_t size)
{
    rb_str_modify(str);
    rb_str_set_len(str, 0);
    rb_str_modify_expand(str, size);
    return str;
}

#endif /* !EXTLZHAM_H */
