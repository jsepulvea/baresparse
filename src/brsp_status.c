#include <baresparse/brsp_status.h>

const char *brsp_status_string(brsp_status status) {
    switch (status) {
    case BRSP_STATUS_OK:
        return "ok";
    case BRSP_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case BRSP_STATUS_INVALID_STRUCTURE:
        return "invalid structure";
    case BRSP_STATUS_INSUFFICIENT_CAPACITY:
        return "insufficient capacity";
    case BRSP_STATUS_SINGULAR:
        return "singular matrix";
    case BRSP_STATUS_NOT_IMPLEMENTED:
        return "not implemented";
    case BRSP_STATUS_OVERFLOW:
        return "index or size overflow";
    case BRSP_STATUS_PIVOT_REJECTED:
        return "fixed pivot rejected";
    case BRSP_STATUS_NONFINITE:
        return "non-finite numerical value";
    case BRSP_STATUS_NOT_FACTORED:
        return "no valid numeric factors";
    default:
        return "unknown status";
    }
}
