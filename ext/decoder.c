#include "extlzham.h"

VALUE cDecoder;

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
        p.m_dict_size_log2 = aux_getoptu32(opts, id_dictsize, LZHAM_MIN_DICT_SIZE_LOG2);
        p.m_table_update_rate = aux_getoptu32(opts, id_table_update_rate, 0);
        p.m_decompress_flags = aux_getoptu32(opts, id_flags, 0);
        p.m_num_seed_bytes = 0;
        p.m_pSeed_bytes = NULL;
        p.m_table_max_update_interval = aux_getoptu32(opts, id_table_max_update_interval, 0);
        p.m_table_update_interval_slow_rate = aux_getoptu32(opts, id_table_update_interval_slow_rate, 0);
    }

    return p;
}

/*
 * call-seq:
 *  decode(string, max_decoded_size, opts = {}) -> decoded string
 */
static VALUE
dec_s_decode(int argc, VALUE argv[], VALUE mod)
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
        extlzham_decode_error(s);
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
dec_mark(struct decoder *p)
{
    if (p) {
        rb_gc_mark(p->outport);
        rb_gc_mark(p->outbuf);
    }
}

static void
dec_free(struct decoder *p)
{
    if (p) {
        if (p->decoder) {
            lzham_decompress_deinit(p->decoder);
        }
    }
}

static VALUE
dec_alloc(VALUE klass)
{
    struct decoder *p;
    VALUE obj = Data_Make_Struct(klass, struct decoder, dec_mark, dec_free, p);
    return obj;
}

static inline struct decoder *
getdecoderp(VALUE obj)
{
    struct decoder *p;
    Data_Get_Struct(obj, struct decoder, p);
    return p;
}

static inline struct decoder *
getdecoder(VALUE obj)
{
    struct decoder *p = getdecoderp(obj);
    if (!p || !p->decoder) {
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
dec_init(int argc, VALUE argv[], VALUE dec)
{
    struct decoder *p = DATA_PTR(dec);
    if (p->decoder) {
        rb_raise(eError,
                 "already initialized - #<%s:%p>",
                 rb_obj_classname(dec), (void *)dec);
    }

    VALUE outport, opts;
    rb_scan_args(argc, argv, "01:", &outport, &opts);
    if (NIL_P(outport)) { outport = rb_str_buf_new(0); }
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
    struct decoder *p = getdecoder(args->decoder);
    const char *inbuf, *intail;

    if (NIL_P(args->src)) {
        inbuf = NULL;
        intail = NULL;
    } else {
        inbuf = RSTRING_PTR(args->src);
        intail = inbuf + RSTRING_LEN(args->src);
    }

    lzham_decompress_status_t s;
    do {
        size_t insize = intail - inbuf;
        aux_str_reserve(p->outbuf, WORKBUF_SIZE);
        rb_str_locktmp(p->outbuf);
        size_t outsize = rb_str_capacity(p->outbuf);
        s = aux_lzham_decompress(p->decoder,
                (lzham_uint8 *)inbuf, &insize,
                (lzham_uint8 *)RSTRING_PTR(p->outbuf), &outsize, args->flush);
        rb_str_unlocktmp(p->outbuf);
//fprintf(stderr, "%s:%d:%s: status=%s (%d), insize=%zu, outsize=%zu\n", __FILE__, __LINE__, __func__, extlzham_decode_status_str(s), s, insize, outsize);
        if (!NIL_P(args->src)) {
            inbuf += insize;
        }
        if (s != LZHAM_DECOMP_STATUS_NOT_FINISHED &&
            s != LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT &&
            s != LZHAM_DECOMP_STATUS_NEEDS_MORE_INPUT &&
            s != LZHAM_DECOMP_STATUS_SUCCESS) {

            extlzham_decode_error(s);
        }
        if (outsize > 0) {
            rb_str_set_len(p->outbuf, outsize);
            rb_funcall2(p->outport, id_op_lshift, 1, &p->outbuf);
        }
    } while (inbuf < intail || s == LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT);

    return 0;
}

static inline void
dec_update_common(VALUE dec, VALUE src, int flush)
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
dec_update(int argc, VALUE argv[], VALUE dec)
{
    VALUE src, flush;
    rb_scan_args(argc, argv, "11", &src, &flush);
    rb_check_type(src, RUBY_T_STRING);
    dec_update_common(dec, src, RTEST(flush) ? 1 : 0);
    return dec;
}

static VALUE
dec_finish(VALUE dec)
{
    dec_update_common(dec, Qnil, 1);
    return dec;
}

static VALUE
dec_op_lshift(VALUE dec, VALUE src)
{
    rb_check_type(src, RUBY_T_STRING);
    dec_update_common(dec, src, 0);
    return dec;
}

static VALUE
dec_get_outport(VALUE dec)
{
    return getdecoder(dec)->outport;
}

static VALUE
dec_set_outport(VALUE dec, VALUE outport)
{
    return getdecoder(dec)->outport = outport;
}

void
extlzham_init_decoder(void)
{
    RDOCFAKE(mLZHAM = rb_define_module("LZHAM"));
    cDecoder = rb_define_class_under(mLZHAM, "Decoder", rb_cObject);
    rb_include_module(cDecoder, mConsts);
    rb_define_alloc_func(cDecoder, dec_alloc);
    rb_define_singleton_method(cDecoder, "decode", RUBY_METHOD_FUNC(dec_s_decode), -1);
    rb_define_method(cDecoder, "initialize", RUBY_METHOD_FUNC(dec_init), -1);
    rb_define_method(cDecoder, "update", RUBY_METHOD_FUNC(dec_update), -1);
    rb_define_method(cDecoder, "finish", RUBY_METHOD_FUNC(dec_finish), 0);
    rb_define_method(cDecoder, "<<", RUBY_METHOD_FUNC(dec_op_lshift), 1);
    rb_define_method(cDecoder, "outport", RUBY_METHOD_FUNC(dec_get_outport), 0);
    rb_define_method(cDecoder, "outport=", RUBY_METHOD_FUNC(dec_set_outport), 1);

}
