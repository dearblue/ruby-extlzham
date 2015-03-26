#include "extlzham.h"

VALUE mLZHAM;

ID ID_op_lshift;
ID IDdictsize;
ID IDlevel;
ID IDtable_update_rate;
ID IDthreads;
ID IDflags;
ID IDtable_max_update_interval;
ID IDtable_update_interval_slow_rate;

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

    init_error();
    init_constants();
    init_encoder();
    init_decoder();
}
