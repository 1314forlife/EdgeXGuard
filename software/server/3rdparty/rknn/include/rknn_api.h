#ifndef _RKNN_API_H_
#define _RKNN_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t rknn_context;

typedef enum _rknn_tensor_type {
    RKNN_TENSOR_FLOAT32 = 0,
    RKNN_TENSOR_FLOAT16,
    RKNN_TENSOR_INT8,
    RKNN_TENSOR_UINT8,
    RKNN_TENSOR_INT16,
    RKNN_TENSOR_TYPE_MAX
} rknn_tensor_type;

typedef enum _rknn_tensor_format {
    RKNN_TENSOR_NCHW = 0,
    RKNN_TENSOR_NHWC,
    RKNN_TENSOR_FORMAT_MAX
} rknn_tensor_format;

typedef struct _rknn_input {
    uint32_t index;
    void* buf;
    uint32_t size;
    uint8_t pass_through;
    rknn_tensor_type type;
    rknn_tensor_format fmt;
} rknn_input;

typedef struct _rknn_output {
    uint32_t want_float;
    uint32_t is_prealloc;
    uint32_t index;
    void* buf;
    uint32_t size;
} rknn_output;

typedef struct _rknn_input_output_num {
    uint32_t n_input;
    uint32_t n_output;
} rknn_input_output_num;

typedef enum _rknn_query_cmd {
    RKNN_QUERY_IN_OUT_NUM = 0,
    RKNN_QUERY_INPUT_ATTR,
    RKVV_QUERY_OUTPUT_ATTR,
    RKNN_QUERY_SDK_VERSION,
} rknn_query_cmd;

// 核心函数满血声明
int rknn_init(rknn_context* context, void* model, uint32_t size, uint32_t flag, void* rknn_init_extend);
int rknn_destroy(rknn_context context);
int rknn_query(rknn_context context, rknn_query_cmd cmd, void* info, uint32_t size);
int rknn_inputs_set(rknn_context context, uint32_t n_inputs, rknn_input inputs[]);
int rknn_run(rknn_context context, void* extend);
int rknn_outputs_get(rknn_context context, uint32_t n_outputs, rknn_output outputs[], void* extend);
int rknn_outputs_release(rknn_context context, uint32_t n_outputs, rknn_output outputs[]);

#ifdef __cplusplus
}
#endif

#endif // _RKNN_API_H_