#include "extlzham.h"

VALUE mConsts;

void
init_constants(void)
{
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
}
