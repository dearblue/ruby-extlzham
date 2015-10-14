#include "extlzham.h"

VALUE eError;

#define SET_MESSAGE(MESG, CONST) \
    case CONST:                  \
        MESG = #CONST;           \
        break                    \

const char *
extlzham_encode_status_str(lzham_compress_status_t status)
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

    return mesg;
}

void
extlzham_encode_error(lzham_compress_status_t status)
{
    rb_raise(eError,
             "LZHAM encode error - %s (0x%04X)",
             extlzham_encode_status_str(status), status);
}

const char *
extlzham_decode_status_str(lzham_decompress_status_t status)
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

    return mesg;
}

void
extlzham_decode_error(lzham_decompress_status_t status)
{
    rb_raise(eError,
             "LZHAM decode error - %s (0x%04X)",
             extlzham_decode_status_str(status), status);
}

void
extlzham_init_error(void)
{
    eError = rb_define_class_under(mLZHAM, "Error", rb_eRuntimeError);
}
