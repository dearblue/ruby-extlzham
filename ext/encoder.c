#include "extlzham.h"

VALUE cEncoder;

static inline lzham_compress_params
scan_encode_params(VALUE opts)
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

static void
scan_encode_args(int argc, VALUE argv[], VALUE *src, size_t *srcsize, VALUE *dest, size_t *destsize, lzham_compress_params *params)
{
    VALUE opts;
    rb_scan_args(argc, argv, "11:", src, dest, &opts);
    rb_check_type(*src, RUBY_T_STRING);
    //rb_str_locktmp(src);
    *srcsize = RSTRING_LEN(*src);
    *destsize = lzham_z_compressBound(*srcsize);
    if (NIL_P(*dest)) {
        *dest = rb_str_buf_new(*destsize);
    } else {
        rb_check_type(*dest, RUBY_T_STRING);
        rb_str_modify(*dest);
        rb_str_set_len(*dest, 0);
        rb_str_modify_expand(*dest, *destsize);
    }
    *params = scan_encode_params(opts);
}

/*
 * call-seq:
 *  encode(src, opts = {}) -> encoded string
 *  encode(src, dest, opts = {}) -> encoded string
 */
static VALUE
ext_s_encode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, dest;
    size_t srcsize, destsize;
    lzham_compress_params params;
    scan_encode_args(argc, argv, &src, &srcsize, &dest, &destsize, &params);
    lzham_compress_status_t s;
    s = lzham_compress_memory(&params,
                              (lzham_uint8 *)RSTRING_PTR(dest), &destsize,
                              (lzham_uint8 *)RSTRING_PTR(src), srcsize, NULL);
    //rb_str_unlocktmp(src);

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
 *  initialize(outport = nil, opts = {})
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
    rb_scan_args(argc, argv, "01:", &outport, &opts);
    if (NIL_P(outport)) { outport = rb_str_buf_new(0); }
    lzham_compress_params params = scan_encode_params(opts);
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

    lzham_compress_status_t s;
    do {
        size_t insize = intail - inbuf;
        aux_str_reserve(p->outbuf, WORKBUF_SIZE);
        rb_str_locktmp(p->outbuf);
        size_t outsize = rb_str_capacity(p->outbuf);
        s = aux_lzham_compress2(p->encoder,
                (lzham_uint8 *)inbuf, &insize,
                (lzham_uint8 *)RSTRING_PTR(p->outbuf), &outsize, args->flush);
        rb_str_unlocktmp(p->outbuf);
        if (!NIL_P(args->src)) {
            inbuf += insize;
        }
//fprintf(stderr, "%s:%d:%s: status=%s (%d), insize=%zu, outsize=%zu\n", __FILE__, __LINE__, __func__, aux_encode_status_str(s), s, insize, outsize);
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
    } while (inbuf < intail || s == LZHAM_COMP_STATUS_HAS_MORE_OUTPUT);

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
        enc_update(enc, src, LZHAM_NO_FLUSH);
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
    enc_update(enc, src, LZHAM_NO_FLUSH);
    return enc;
}


static VALUE
ext_enc_get_outport(VALUE enc)
{
    struct encoder *p = aux_encoder_ref(enc);
    return p->outport;
}

static VALUE
ext_enc_set_outport(VALUE enc, VALUE outport)
{
    struct encoder *p = aux_encoder_ref(enc);
    p->outport = outport;
    return outport;
}

void
init_encoder(void)
{
    cEncoder = rb_define_class_under(mLZHAM, "Encoder", rb_cObject);
    rb_include_module(cEncoder, mConsts);
    rb_define_alloc_func(cEncoder, ext_enc_alloc);
    rb_define_singleton_method(cEncoder, "encode", RUBY_METHOD_FUNC(ext_s_encode), -1);
    rb_define_method(cEncoder, "initialize", RUBY_METHOD_FUNC(ext_enc_init), -1);
    rb_define_method(cEncoder, "update", RUBY_METHOD_FUNC(ext_enc_update), -1);
    rb_define_method(cEncoder, "finish", RUBY_METHOD_FUNC(ext_enc_finish), 0);
    rb_define_method(cEncoder, "<<", RUBY_METHOD_FUNC(ext_enc_op_lshift), 1);
    rb_define_method(cEncoder, "outport", RUBY_METHOD_FUNC(ext_enc_get_outport), 0);
    rb_define_method(cEncoder, "outport=", RUBY_METHOD_FUNC(ext_enc_set_outport), 1);
}
