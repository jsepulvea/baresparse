#ifndef BARESPARSE_BRSP_H
#define BARESPARSE_BRSP_H

#include <baresparse/brsp_config.h>
#include <baresparse/brsp_csc.h>
#include <baresparse/brsp_dense_lu.h>
#include <baresparse/brsp_sparse_lu.h>
#include <baresparse/brsp_status.h>
#include <baresparse/brsp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *brsp_version_string(void);
const char *brsp_backend_name(void);

#ifdef __cplusplus
}
#endif

#endif
