#include <stdint.h>
#include <ruby.h>
#include <ruby/thread.h>
#include <lzham.h>

static VALUE mLZHAM;
static VALUE eError;
static VALUE cEncoder;
static VALUE cDecoder;
static VALUE mConsts;

static ID ID_op_lshift;
static ID IDdictsize;
static ID IDlevel;
static ID IDtable_update_rate;
static ID IDthreads;
static ID IDflags;
static ID IDtable_max_update_interval;
static ID IDtable_update_interval_slow_rate;

enum {
    WORKBUF_SIZE = 256 * 1024, /* 256 KiB */
};

#define SET_MESSAGE(MESG, CONST) \
    case CONST:                  \
        MESG = #CONST;           \
        break                    \

static inline void
aux_encode_error(lzham_compress_status_t status)
{
    const char *mesg;
    switch (status) {
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_NOT_FINISHED);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_NEEDS_MORE_INPUT);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_HAS_MORE_OUTPUT);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_FIRST_SUCCESS_OR_FAILURE_CODE);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_FIRST_FAILURE_CODE);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_FAILED_INITIALIZING);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_INVALID_PARAMETER);
        SET_MESSAGE(mesg, LZHAM_COMP_STATUS_OUTPUT_BUF_TOO_SMALL);
    default:
        mesg = "unknown code";
        break;
    }

    rb_raise(eError,
             "LZHAM encode error - %s (0x%04X)",
             mesg, status);
}

static inline void
aux_decode_error(lzham_decompress_status_t status)
{
    const char *mesg;
    switch (status) {
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_NOT_FINISHED);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_NEEDS_MORE_INPUT);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FIRST_SUCCESS_OR_FAILURE_CODE);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FIRST_FAILURE_CODE);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_DEST_BUF_TOO_SMALL);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_EXPECTED_MORE_RAW_BYTES);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_BAD_CODE);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_ADLER32);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_BAD_RAW_BLOCK);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_BAD_COMP_BLOCK_SYNC_CHECK);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_BAD_ZLIB_HEADER);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_NEED_SEED_BYTES);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_BAD_SEED_BYTES);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_FAILED_BAD_SYNC_BLOCK);
        SET_MESSAGE(mesg, LZHAM_DECOMP_STATUS_INVALID_PARAMETER);
    default:
        mesg = "unknown code";
        break;
    }

    rb_raise(eError,
             "LZHAM decode error - %s (0x%04X)",
             mesg, status);
}

static inline uint32_t
aux_hash_lookup_to_u32(VALUE hash, ID key, uint32_t defaultvalue)
{
    VALUE d = rb_hash_lookup2(hash, ID2SYM(key), Qundef);
    if (d == Qundef) { return defaultvalue; }
    return NUM2UINT(d);
}

static inline lzham_compress_params
aux_conv_encode_params(VALUE opts)
{
    lzham_compress_params p;
    memset(&p, 0, sizeof(p));
    p.m_struct_size = sizeof(p);
    if (NIL_P(opts)) {
        p.m_dict_size_log2 = LZHAM_MIN_DICT_SIZE_LOG2;
        p.m_level = LZHAM_COMP_LEVEL_DEFAULT;
        p.m_table_update_rate = 0;
        p.m_max_helper_threads = -1;
        p.m_compress_flags = 0; // (see lzham_compress_flags enum)
        p.m_num_seed_bytes = 0;
        p.m_pSeed_bytes = NULL;
        p.m_table_max_update_interval = 0;
        p.m_table_update_interval_slow_rate = 0;
    } else {
        p.m_dict_size_log2 = aux_hash_lookup_to_u32(opts, IDdictsize, LZHAM_MIN_DICT_SIZE_LOG2);
        p.m_level = aux_hash_lookup_to_u32(opts, IDlevel, LZHAM_COMP_LEVEL_DEFAULT);
        p.m_table_update_rate = aux_hash_lookup_to_u32(opts, IDtable_update_rate, 0);
        p.m_max_helper_threads = aux_hash_lookup_to_u32(opts, IDthreads, -1);
        p.m_compress_flags = aux_hash_lookup_to_u32(opts, IDflags, 0);
        p.m_num_seed_bytes = 0;
        p.m_pSeed_bytes = NULL;
        p.m_table_max_update_interval = aux_hash_lookup_to_u32(opts, IDtable_max_update_interval, 0);
        p.m_table_update_interval_slow_rate = aux_hash_lookup_to_u32(opts, IDtable_update_interval_slow_rate, 0);
    }
    return p;
}

/*
 * call-seq:
 *  encode(string, opts = {}) -> encoded string
 */
static VALUE
ext_s_encode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, opts;
    rb_scan_args(argc, argv, "1:", &src, &opts);
    rb_check_type(src, RUBY_T_STRING);
    rb_str_locktmp(src);
    size_t srcsize = RSTRING_LEN(src);
    size_t destsize = lzham_z_compressBound(srcsize);
    VALUE dest = rb_str_buf_new(destsize);
    lzham_compress_params params = aux_conv_encode_params(opts);
    lzham_compress_status_t s;
    s = lzham_compress_memory(&params,
                              (lzham_uint8 *)RSTRING_PTR(dest), &destsize,
                              (lzham_uint8 *)RSTRING_PTR(src), srcsize, NULL);

    rb_str_unlocktmp(src);

    if (s != LZHAM_COMP_STATUS_SUCCESS) {
        rb_str_resize(dest, 0);
        aux_encode_error(s);
    }

    rb_str_resize(dest, destsize);
    rb_str_set_len(dest, destsize);

    return dest;
}

struct encoder
{
    lzham_compress_state_ptr encoder;
    VALUE outport;
    VALUE outbuf;
};

static void
ext_enc_mark(struct encoder *p)
{
    if (p) {
        rb_gc_mark(p->outport);
        rb_gc_mark(p->outbuf);
    }
}

static void
ext_enc_free(struct encoder *p)
{
    if (p) {
        if (p->encoder) {
            lzham_compress_deinit(p->encoder);
        }
    }
}

static VALUE
ext_enc_alloc(VALUE klass)
{
    struct encoder *p;
    VALUE obj = Data_Make_Struct(klass, struct encoder, ext_enc_mark, ext_enc_free, p);
    return obj;
}

static inline struct encoder *
aux_encoder_refp(VALUE obj)
{
    struct encoder *p;
    Data_Get_Struct(obj, struct encoder, p);
    return p;
}

static inline struct encoder *
aux_encoder_ref(VALUE obj)
{
    struct encoder *p = aux_encoder_refp(obj);
    if (!p || !p->encoder) {
        rb_raise(eError,
                 "not initialized - #<%s:%p>",
                 rb_obj_classname(obj), (void *)obj);
    }
    return p;
}

/*
 * call-seq:
 *  initialize(outport, opts = {})
 */
static VALUE
ext_enc_init(int argc, VALUE argv[], VALUE enc)
{
    struct encoder *p = DATA_PTR(enc);
    if (p->encoder) {
        rb_raise(eError,
                 "already initialized - #<%s:%p>",
                 rb_obj_classname(enc), (void *)enc);
    }

    VALUE outport, opts;
    rb_scan_args(argc, argv, "1:", &outport, &opts);
    lzham_compress_params params = aux_conv_encode_params(opts);
    p->encoder = lzham_compress_init(&params);
    if (!p->encoder) {
        rb_raise(eError,
                 "failed lzham_compress_init - #<%s:%p>",
                 rb_obj_classname(enc), (void *)enc);
    }

    p->outbuf = rb_str_buf_new(WORKBUF_SIZE);
    p->outport = outport;

    return enc;
}

struct enc_update_args
{
    VALUE encoder;
    VALUE src;
    int flush;
};

struct aux_lzham_compress2_nogvl
{
    lzham_compress_state_ptr state;
    const lzham_uint8 *inbuf;
    size_t *insize;
    lzham_uint8 *outbuf;
    size_t *outsize;
    lzham_flush_t flush;
};

static void *
aux_lzham_compress2_nogvl(void *px)
{
    struct aux_lzham_compress2_nogvl *p = px;
    return (void *)lzham_compress2(p->state,
            p->inbuf, p->insize, p->outbuf, p->outsize, p->flush);
}

static inline lzham_compress_status_t
aux_lzham_compress2(lzham_compress_state_ptr state,
        const lzham_uint8 *inbuf, size_t *insize,
        lzham_uint8 *outbuf, size_t *outsize,
        lzham_flush_t flush)
{
    struct aux_lzham_compress2_nogvl p = {
        .state = state,
        .inbuf = inbuf,
        .insize = insize,
        .outbuf = outbuf,
        .outsize = outsize,
        .flush = flush,
    };

    return (lzham_compress_status_t)rb_thread_call_without_gvl(aux_lzham_compress2_nogvl, &p, 0, 0);
}

static VALUE
enc_update_protected(struct enc_update_args *args)
{
    struct encoder *p = aux_encoder_ref(args->encoder);
    const char *inbuf, *intail;

    if (NIL_P(args->src)) {
        inbuf = NULL;
        intail = NULL;
    } else {
        inbuf = RSTRING_PTR(args->src);
        intail = inbuf + RSTRING_LEN(args->src);
    }

    for (;;) {
        size_t insize = intail - inbuf;
        rb_str_locktmp(p->outbuf);
        size_t outsize = rb_str_capacity(p->outbuf);
        lzham_compress_status_t s;
        s = aux_lzham_compress2(p->encoder,
                (lzham_uint8 *)inbuf, &insize,
                (lzham_uint8 *)RSTRING_PTR(p->outbuf), &outsize, args->flush);
        rb_str_unlocktmp(p->outbuf);
        if (!NIL_P(args->src)) {
            inbuf += insize;
            if (inbuf == intail) {
                inbuf = intail = NULL;
            }
        }
//fprintf(stderr, "%s:%d:%s: status=%d, insize=%zu, outsize=%zu\n", __FILE__, __LINE__, __func__, s, insize, outsize);
        if (s != LZHAM_COMP_STATUS_SUCCESS &&
            s != LZHAM_COMP_STATUS_NEEDS_MORE_INPUT &&
            s != LZHAM_COMP_STATUS_NOT_FINISHED &&
            s != LZHAM_COMP_STATUS_HAS_MORE_OUTPUT) {

            aux_encode_error(s);
        }
        if (outsize > 0) {
            rb_str_set_len(p->outbuf, outsize);
            rb_funcall2(p->outport, ID_op_lshift, 1, &p->outbuf);
        }
        if (s != LZHAM_COMP_STATUS_HAS_MORE_OUTPUT) {
            break;
        }
    }

    return 0;
}

static inline void
enc_update(VALUE enc, VALUE src, int flush)
{
    struct enc_update_args args = { enc, src, flush };
    if (NIL_P(src)) {
        enc_update_protected(&args);
    } else {
        rb_str_locktmp(src);
        int state;
        rb_protect((VALUE (*)(VALUE))enc_update_protected, (VALUE)&args, &state);
        rb_str_unlocktmp(src);
        if (state) {
            rb_jump_tag(state);
        }
    }
}

/*
 * call-seq:
 *  update(src, flush = LZHAM::NO_FLUSH) -> self
 */
static VALUE
ext_enc_update(int argc, VALUE argv[], VALUE enc)
{
    VALUE src, flush;
    rb_scan_args(argc, argv, "11", &src, &flush);
    rb_check_type(src, RUBY_T_STRING);
    if (NIL_P(flush)) {
        enc_update(enc, src, 0);
    } else {
        enc_update(enc, src, NUM2INT(flush));
    }
    return enc;
}

static VALUE
ext_enc_finish(VALUE enc)
{
    enc_update(enc, Qnil, LZHAM_FINISH);
    return enc;
}

/*
 * same as <tt>enc.update(src)</tt>
 */
static VALUE
ext_enc_op_lshift(VALUE enc, VALUE src)
{
    rb_check_type(src, RUBY_T_STRING);
    enc_update(enc, src, 0);
    return enc;
}

/*
 * decoder
 */

static inline lzham_decompress_params
aux_conv_decode_params(VALUE opts)
{
    lzham_decompress_params p;
    memset(&p, 0, sizeof(p));
    p.m_struct_size = sizeof(p);
    if (NIL_P(opts)) {
        p.m_dict_size_log2 = LZHAM_MIN_DICT_SIZE_LOG2;
        p.m_table_update_rate = 0;
        p.m_decompress_flags = 0; // (see lzham_decompress_flags enum)
        p.m_num_seed_bytes = 0;
        p.m_pSeed_bytes = NULL;
        p.m_table_max_update_interval = 0;
        p.m_table_update_interval_slow_rate = 0;
    } else {
        p.m_dict_size_log2 = aux_hash_lookup_to_u32(opts, IDdictsize, LZHAM_MIN_DICT_SIZE_LOG2);
        p.m_table_update_rate = aux_hash_lookup_to_u32(opts, IDtable_update_rate, 0);
        p.m_decompress_flags = aux_hash_lookup_to_u32(opts, IDflags, 0);
        p.m_num_seed_bytes = 0;
        p.m_pSeed_bytes = NULL;
        p.m_table_max_update_interval = aux_hash_lookup_to_u32(opts, IDtable_max_update_interval, 0);
        p.m_table_update_interval_slow_rate = aux_hash_lookup_to_u32(opts, IDtable_update_interval_slow_rate, 0);
    }

    return p;
}

/*
 * call-seq:
 *  decode(string, max_decoded_size, opts = {}) -> decoded string
 */
static VALUE
ext_s_decode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, size, opts;
    rb_scan_args(argc, argv, "2:", &src, &size, &opts);
    rb_check_type(src, RUBY_T_STRING);
    rb_str_locktmp(src);
    size_t srcsize = RSTRING_LEN(src);
    size_t destsize = NUM2SIZET(size);
    VALUE dest = rb_str_buf_new(destsize);
    lzham_decompress_params p = aux_conv_decode_params(opts);
    lzham_decompress_status_t s;
    s = lzham_decompress_memory(&p,
            (lzham_uint8 *)RSTRING_PTR(dest), &destsize,
            (lzham_uint8 *)RSTRING_PTR(src), srcsize, NULL);

    rb_str_unlocktmp(src);

    if (s != LZHAM_DECOMP_STATUS_SUCCESS) {
        rb_str_resize(dest, 0);
        aux_decode_error(s);
    }

    rb_str_resize(dest, destsize);
    rb_str_set_len(dest, destsize);

    return dest;
}

struct decoder
{
    lzham_decompress_state_ptr decoder;
    VALUE outport;
    VALUE outbuf;
};

static void
ext_dec_mark(struct decoder *p)
{
    if (p) {
        rb_gc_mark(p->outport);
        rb_gc_mark(p->outbuf);
    }
}

static void
ext_dec_free(struct decoder *p)
{
    if (p) {
        if (p->decoder) {
            lzham_decompress_deinit(p->decoder);
        }
    }
}

static VALUE
ext_dec_alloc(VALUE klass)
{
    struct decoder *p;
    VALUE obj = Data_Make_Struct(klass, struct decoder, ext_dec_mark, ext_dec_free, p);
    return obj;
}

static inline struct decoder *
aux_decoder_refp(VALUE obj)
{
    struct decoder *p;
    Data_Get_Struct(obj, struct decoder, p);
    return p;
}

static inline struct decoder *
aux_decoder_ref(VALUE obj)
{
    struct decoder *p = aux_decoder_refp(obj);
    if (!p || !p->decoder) {
        rb_raise(eError,
                 "not initialized - #<%s:%p>",
                 rb_obj_classname(obj), (void *)obj);
    }
    return p;
}

static VALUE
ext_dec_init(int argc, VALUE argv[], VALUE dec)
{
    struct decoder *p = DATA_PTR(dec);
    if (p->decoder) {
        rb_raise(eError,
                 "already initialized - #<%s:%p>",
                 rb_obj_classname(dec), (void *)dec);
    }

    VALUE outport, opts;
    rb_scan_args(argc, argv, "1:", &outport, &opts);
    lzham_decompress_params params = aux_conv_decode_params(opts);
    p->decoder = lzham_decompress_init(&params);
    if (!p->decoder) {
        rb_raise(eError,
                 "failed lzham_decompress_init - #<%s:%p>",
                 rb_obj_classname(dec), (void *)dec);
    }

    p->outbuf = rb_str_buf_new(WORKBUF_SIZE);
    p->outport = outport;

    return dec;
}

struct aux_lzham_decompress_nogvl
{
    lzham_decompress_state_ptr state;
    const lzham_uint8 *inbuf;
    size_t *insize;
    lzham_uint8 *outbuf;
    size_t *outsize;
    lzham_bool flags;
};

static void *
aux_lzham_decompress_nogvl(void *px)
{
    struct aux_lzham_decompress_nogvl *p = px;
    return (void *)lzham_decompress(p->state, p->inbuf, p->insize, p->outbuf, p->outsize, p->flags);
}

static inline lzham_decompress_status_t
aux_lzham_decompress(lzham_decompress_state_ptr state,
        const lzham_uint8 *inbuf, size_t *insize,
        lzham_uint8 *outbuf, size_t *outsize,
        lzham_bool flags)
{
    struct aux_lzham_decompress_nogvl p = {
        .state = state,
        .inbuf = inbuf,
        .insize = insize,
        .outbuf = outbuf,
        .outsize = outsize,
        .flags = flags,
    };

    return (lzham_decompress_status_t)rb_thread_call_without_gvl(aux_lzham_decompress_nogvl, &p, 0, 0);
}

struct dec_update_args
{
    VALUE decoder;
    VALUE src;
    int flush;
};

static VALUE
dec_update_protected(struct dec_update_args *args)
{
    struct decoder *p = aux_decoder_ref(args->decoder);
    const char *inbuf, *intail;

    if (NIL_P(args->src)) {
        inbuf = NULL;
        intail = NULL;
    } else {
        inbuf = RSTRING_PTR(args->src);
        intail = inbuf + RSTRING_LEN(args->src);
    }

    for (;;) {
        size_t insize = intail - inbuf;
        rb_str_locktmp(p->outbuf);
        size_t outsize = rb_str_capacity(p->outbuf);
        lzham_decompress_status_t s;
//fprintf(stderr, "%s:%d:%s: inbuf=%p, insize=%zu, outbuf=%p, outsize=%zu\n", __FILE__, __LINE__, __func__, inbuf, insize, RSTRING_PTR(p->outbuf), outsize);
        s = aux_lzham_decompress(p->decoder,
                (lzham_uint8 *)inbuf, &insize,
                (lzham_uint8 *)RSTRING_PTR(p->outbuf), &outsize, args->flush);
        rb_str_unlocktmp(p->outbuf);
//fprintf(stderr, "%s:%d:%s: status=%d, insize=%zu, outsize=%zu\n", __FILE__, __LINE__, __func__, s, insize, outsize);
        if (!NIL_P(args->src)) {
            inbuf += insize;
        }
        if (s != LZHAM_DECOMP_STATUS_NOT_FINISHED &&
            s != LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT &&
            s != LZHAM_DECOMP_STATUS_NEEDS_MORE_INPUT &&
            s != LZHAM_DECOMP_STATUS_SUCCESS) {

            aux_decode_error(s);
        }
        if (outsize > 0) {
            rb_str_set_len(p->outbuf, outsize);
            rb_funcall2(p->outport, ID_op_lshift, 1, &p->outbuf);
        }
        if (s != LZHAM_DECOMP_STATUS_NOT_FINISHED) {
            break;
        }
    }

    return 0;
}

static inline void
dec_update(VALUE dec, VALUE src, int flush)
{
    struct dec_update_args args = { dec, src, flush };
    if (NIL_P(src)) {
        dec_update_protected(&args);
    } else {
        rb_str_locktmp(src);
        int state;
        rb_protect((VALUE (*)(VALUE))dec_update_protected, (VALUE)&args, &state);
        rb_str_unlocktmp(src);
        if (state) {
            rb_jump_tag(state);
        }
    }
}

/*
 * call-seq:
 *  update(src, flush = false) -> self
 */
static VALUE
ext_dec_update(int argc, VALUE argv[], VALUE dec)
{
    VALUE src, flush;
    rb_scan_args(argc, argv, "11", &src, &flush);
    rb_check_type(src, RUBY_T_STRING);
    dec_update(dec, src, RTEST(flush) ? 1 : 0);
    return dec;
}

static VALUE
ext_dec_finish(VALUE dec)
{
    dec_update(dec, Qnil, 1);
    return dec;
}

static VALUE
ext_dec_op_lshift(VALUE dec, VALUE src)
{
    rb_check_type(src, RUBY_T_STRING);
    dec_update(dec, src, 0);
    return dec;
}

void
Init_extlzham(void)
{
    ID_op_lshift = rb_intern("<<");
    IDdictsize = rb_intern("dictsize");
    IDlevel = rb_intern("level");
    IDtable_update_rate = rb_intern("table_update_rate");
    IDthreads = rb_intern("threads");
    IDflags = rb_intern("flags");
    IDtable_max_update_interval = rb_intern("table_max_update_interval");
    IDtable_update_interval_slow_rate = rb_intern("table_update_interval_slow_rate");

    mLZHAM = rb_define_module("LZHAM");
    rb_define_const(mLZHAM, "LZHAM", mLZHAM);
    rb_define_const(mLZHAM, "LIBVERSION", UINT2NUM(lzham_get_version()));

    mConsts = rb_define_module_under(mLZHAM, "Constants");
    rb_include_module(mLZHAM, mConsts);
    rb_define_const(mConsts, "NO_FLUSH", INT2FIX(LZHAM_NO_FLUSH));
    rb_define_const(mConsts, "SYNC_FLUSH", INT2FIX(LZHAM_SYNC_FLUSH));
    rb_define_const(mConsts, "FULL_FLUSH", INT2FIX(LZHAM_FULL_FLUSH));
    rb_define_const(mConsts, "FINISH", INT2FIX(LZHAM_FINISH));
    rb_define_const(mConsts, "TABLE_FLUSH", INT2FIX(LZHAM_TABLE_FLUSH));
    rb_define_const(mConsts, "MIN_DICT_SIZE_LOG2", INT2FIX(LZHAM_MIN_DICT_SIZE_LOG2));
    rb_define_const(mConsts, "MAX_DICT_SIZE_LOG2_X86", INT2FIX(LZHAM_MAX_DICT_SIZE_LOG2_X86));
    rb_define_const(mConsts, "MAX_DICT_SIZE_LOG2_X64", INT2FIX(LZHAM_MAX_DICT_SIZE_LOG2_X64));
    rb_define_const(mConsts, "MAX_HELPER_THREADS", INT2FIX(LZHAM_MAX_HELPER_THREADS));
    rb_define_const(mConsts, "COMP_FLAG_EXTREME_PARSING", INT2FIX(LZHAM_COMP_FLAG_EXTREME_PARSING));
    rb_define_const(mConsts, "COMP_FLAG_DETERMINISTIC_PARSING", INT2FIX(LZHAM_COMP_FLAG_DETERMINISTIC_PARSING));
    rb_define_const(mConsts, "COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO", INT2FIX(LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO));
    rb_define_const(mConsts, "COMP_FLAG_WRITE_ZLIB_STREAM", INT2FIX(LZHAM_COMP_FLAG_WRITE_ZLIB_STREAM));
    rb_define_const(mConsts, "INSANELY_SLOW_TABLE_UPDATE_RATE", INT2FIX(LZHAM_INSANELY_SLOW_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "SLOWEST_TABLE_UPDATE_RATE", INT2FIX(LZHAM_SLOWEST_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "DEFAULT_TABLE_UPDATE_RATE", INT2FIX(LZHAM_DEFAULT_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "FASTEST_TABLE_UPDATE_RATE", INT2FIX(LZHAM_FASTEST_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "DECOMP_FLAG_OUTPUT_UNBUFFERED", INT2FIX(LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED));
    rb_define_const(mConsts, "DECOMP_FLAG_COMPUTE_ADLER32", INT2FIX(LZHAM_DECOMP_FLAG_COMPUTE_ADLER32));
    rb_define_const(mConsts, "DECOMP_FLAG_READ_ZLIB_STREAM", INT2FIX(LZHAM_DECOMP_FLAG_READ_ZLIB_STREAM));
    rb_define_const(mConsts, "LZHAM_NO_FLUSH", INT2FIX(LZHAM_NO_FLUSH));
    rb_define_const(mConsts, "LZHAM_SYNC_FLUSH", INT2FIX(LZHAM_SYNC_FLUSH));
    rb_define_const(mConsts, "LZHAM_FULL_FLUSH", INT2FIX(LZHAM_FULL_FLUSH));
    rb_define_const(mConsts, "LZHAM_FINISH", INT2FIX(LZHAM_FINISH));
    rb_define_const(mConsts, "LZHAM_TABLE_FLUSH", INT2FIX(LZHAM_TABLE_FLUSH));
    rb_define_const(mConsts, "LZHAM_MIN_DICT_SIZE_LOG2", INT2FIX(LZHAM_MIN_DICT_SIZE_LOG2));
    rb_define_const(mConsts, "LZHAM_MAX_DICT_SIZE_LOG2_X86", INT2FIX(LZHAM_MAX_DICT_SIZE_LOG2_X86));
    rb_define_const(mConsts, "LZHAM_MAX_DICT_SIZE_LOG2_X64", INT2FIX(LZHAM_MAX_DICT_SIZE_LOG2_X64));
    rb_define_const(mConsts, "LZHAM_MAX_HELPER_THREADS", INT2FIX(LZHAM_MAX_HELPER_THREADS));
    rb_define_const(mConsts, "LZHAM_COMP_FLAG_EXTREME_PARSING", INT2FIX(LZHAM_COMP_FLAG_EXTREME_PARSING));
    rb_define_const(mConsts, "LZHAM_COMP_FLAG_DETERMINISTIC_PARSING", INT2FIX(LZHAM_COMP_FLAG_DETERMINISTIC_PARSING));
    rb_define_const(mConsts, "LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO", INT2FIX(LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO));
    rb_define_const(mConsts, "LZHAM_COMP_FLAG_WRITE_ZLIB_STREAM", INT2FIX(LZHAM_COMP_FLAG_WRITE_ZLIB_STREAM));
    rb_define_const(mConsts, "LZHAM_INSANELY_SLOW_TABLE_UPDATE_RATE", INT2FIX(LZHAM_INSANELY_SLOW_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "LZHAM_SLOWEST_TABLE_UPDATE_RATE", INT2FIX(LZHAM_SLOWEST_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "LZHAM_DEFAULT_TABLE_UPDATE_RATE", INT2FIX(LZHAM_DEFAULT_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "LZHAM_FASTEST_TABLE_UPDATE_RATE", INT2FIX(LZHAM_FASTEST_TABLE_UPDATE_RATE));
    rb_define_const(mConsts, "LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED", INT2FIX(LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED));
    rb_define_const(mConsts, "LZHAM_DECOMP_FLAG_COMPUTE_ADLER32", INT2FIX(LZHAM_DECOMP_FLAG_COMPUTE_ADLER32));
    rb_define_const(mConsts, "LZHAM_DECOMP_FLAG_READ_ZLIB_STREAM", INT2FIX(LZHAM_DECOMP_FLAG_READ_ZLIB_STREAM));

    cEncoder = rb_define_class_under(mLZHAM, "Encoder", rb_cObject);
    rb_include_module(cEncoder, mConsts);
    rb_define_alloc_func(cEncoder, ext_enc_alloc);
    rb_define_singleton_method(cEncoder, "encode", RUBY_METHOD_FUNC(ext_s_encode), -1);
    rb_define_method(cEncoder, "initialize", RUBY_METHOD_FUNC(ext_enc_init), -1);
    rb_define_method(cEncoder, "update", RUBY_METHOD_FUNC(ext_enc_update), -1);
    rb_define_method(cEncoder, "finish", RUBY_METHOD_FUNC(ext_enc_finish), 0);
    rb_define_method(cEncoder, "<<", RUBY_METHOD_FUNC(ext_enc_op_lshift), 1);

    cDecoder = rb_define_class_under(mLZHAM, "Decoder", rb_cObject);
    rb_include_module(cDecoder, mConsts);
    rb_define_alloc_func(cDecoder, ext_dec_alloc);
    rb_define_singleton_method(cDecoder, "decode", RUBY_METHOD_FUNC(ext_s_decode), -1);
    rb_define_method(cDecoder, "initialize", RUBY_METHOD_FUNC(ext_dec_init), -1);
    rb_define_method(cDecoder, "update", RUBY_METHOD_FUNC(ext_dec_update), -1);
    rb_define_method(cDecoder, "finish", RUBY_METHOD_FUNC(ext_dec_finish), 0);
    rb_define_method(cDecoder, "<<", RUBY_METHOD_FUNC(ext_dec_op_lshift), 1);

    eError = rb_define_class_under(mLZHAM, "Error", rb_eRuntimeError);
}
