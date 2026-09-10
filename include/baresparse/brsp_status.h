#ifndef BARESPARSE_BRSP_STATUS_H
#define BARESPARSE_BRSP_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum brsp_status {
    BRSP_STATUS_OK = 0,
    BRSP_STATUS_INVALID_ARGUMENT = 1,
    BRSP_STATUS_INVALID_STRUCTURE = 2,
    BRSP_STATUS_INSUFFICIENT_CAPACITY = 3,
    BRSP_STATUS_SINGULAR = 4,
    BRSP_STATUS_NOT_IMPLEMENTED = 5,
    BRSP_STATUS_OVERFLOW = 6,
    BRSP_STATUS_PIVOT_REJECTED = 7,
    BRSP_STATUS_NONFINITE = 8,
    BRSP_STATUS_NOT_FACTORED = 9
} brsp_status;

const char *brsp_status_string(brsp_status status);

#ifdef __cplusplus
}
#endif

#endif
