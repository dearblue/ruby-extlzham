#include "extlzham.h"

VALUE mLZHAM;

ID id_op_lshift;
ID id_dictsize;
ID id_level;
ID id_table_update_rate;
ID id_threads;
ID id_flags;
ID id_table_max_update_interval;
ID id_table_update_interval_slow_rate;

void
Init_extlzham(void)
{
    id_op_lshift = rb_intern("<<");
    id_dictsize = rb_intern("dictsize");
    id_level = rb_intern("level");
    id_table_update_rate = rb_intern("table_update_rate");
    id_threads = rb_intern("threads");
    id_flags = rb_intern("flags");
    id_table_max_update_interval = rb_intern("table_max_update_interval");
    id_table_update_interval_slow_rate = rb_intern("table_update_interval_slow_rate");

    mLZHAM = rb_define_module("LZHAM");
    rb_define_const(mLZHAM, "LZHAM", mLZHAM);
    rb_define_const(mLZHAM, "LIBVERSION", UINT2NUM(lzham_get_version()));

    extlzham_init_error();
    extlzham_init_constants();
    extlzham_init_encoder();
    extlzham_init_decoder();
}
