
#pragma once

#include <cstdint>

//***
//  Unpack LLK param structs
//***

struct llk_unpack_A_params_t {
    std::uint32_t unpA_operand;
};

constexpr std::uint32_t default_tile_dims[2] = {32, 32};

struct llk_unpack_AB_matmul_params_t {
    std::uint32_t unpA_operand;
    std::uint32_t unpB_operand;
    std::uint32_t transpose_xy_srca;
    const std::uint32_t in0_tile_dims[2];
    const std::uint32_t in1_tile_dims[2];
};

struct llk_unpack_AB_params_t {
    std::uint32_t unpA_operand;
    std::uint32_t unpB_operand;
};

struct llk_unpack_reduce_params_t {
    std::uint32_t unpA_operand;
    // std::uint32_t unpB_operand;  // TODO: Should be removed when llk hw args are cleaned up
};

struct llk_unpack_tilize_params_t {
    std::uint32_t unpA_operand;
    std::uint32_t unpA_block_c_dim;
};

struct llk_unpack_untilize_params_t {
    std::uint32_t unpA_operand;
};

//***
//  Math LLK param structs
//***

struct llk_math_eltwise_binary_params_t {
    std::int32_t unused;
};

struct llk_math_eltwise_unary_params_t {
    std::int32_t sfpu_params[6];  // TODO: Fix how we assign this from hlkc
    std::int32_t unused;
};

struct llk_math_matmul_params_t {
    std::int32_t unused;
};

struct llk_math_reduce_params_t {
    std::int32_t unused;
};

//***
//  Pack LLK param structs
//***

struct llk_relu_config_t {
    std::uint32_t
        ApplyRelu : 16;  // 0 ? no relu, 1 ? val<0=>val=0, 2 ? val<threshold=>val=0, 3 - val>threshold=>val=threshold
    std::uint32_t Threshold : 16;  // fp16
};

union llk_relu_config_u {
    llk_relu_config_t f;
    std::uint32_t val;
};

struct llk_pack_params_t {
    std::uint32_t pack_output;
    llk_relu_config_u relu_config;
};

// TODO: nsmith move this to a common place where the hlk can include it
struct hlk_pack_shifted_params_t {
    std::uint32_t pack_output;
    llk_relu_config_u relu_config;
    int initial_padding;
    int row_shift_x;
    int original_x;
    int original_y;
    int stride;
    int stride_offset;
    int valid_row_count;
    int column_number;
    bool final_iteration;
    bool relu;
    bool reserved[2];
};

struct hlk_pack_shifted_state_t {
    int current_rd_ptr;
    int current_x;
    int current_y;
    int current_wr_ptr;
    bool partial_tile;
    bool reserved[3];
};

using llk_pack_shifted_params_t = hlk_pack_shifted_params_t;
using llk_pack_shifted_state_t = hlk_pack_shifted_state_t;
