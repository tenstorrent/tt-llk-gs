#pragma once
#include "llk_param_structs.h"

#include "ckernel_include.h"
#include "ckernel_template.h"
#include <type_traits>

#include "cmath_common.h"
#include "llk_math_common.h"
#include "llk_format_conversions.h"
#include "ckernel_globals.h"
#include "ckernel_sfpu.h"

using namespace ckernel;
template <SfpuType sfpu_type>
void static_assert_sfpu_type_dependent() {
    static_assert(sfpu_type == SfpuType::unused, "sfpu_type exception");
}
// local function declarations
inline void eltwise_unary_sfpu_configure_addrmod();
inline void eltwise_unary_sfpu_configure_mop();

template <SfpuType sfpu_op, bool APPROXIMATE, DstSync Dst = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu(
    const uint operand,
    uint dst_index, 
    int vector_mode = (int)Dim::RC,
    uint param0 = 0,
    uint param1 = 0,
    uint param2 = 0,
    uint param3 = 0,
    uint param4 = 0,
    uint param5 = 0) {
    if constexpr ((Dst == DstSync::SyncTile16) || (Dst == DstSync::SyncTile2)) {
        math::set_dst_write_addr<DstTileLayout::Default, DstTileShape::Tile32x32>(math_sync_tile_dst_index);
    } else {
        math::set_dst_write_addr<DstTileLayout::Default, DstTileShape::Tile32x32>(dst_index);
    }
    if (vector_mode == (int)Dim::R) {
        // Do a row vector, Face0 + Face1 -- first iteration
        const int ITERATIONS = 1;
#pragma GCC unroll 0
        for (int face = 0; face < 2; face++) {
            sfpu::calculate_sfpu<sfpu_op, APPROXIMATE, 0, ITERATIONS>(param0, param1, param2, param3, param4, param5);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
        }
        // Skip the next 2 faces
        TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
        TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
        TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
        TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
    } else if (vector_mode == (int)Dim::C) {
        // Do a column vector, Face0 + Face2 -- full face
#pragma GCC unroll 0
        for (int face = 0; face < 2; face++) {
            sfpu::calculate_sfpu<sfpu_op, APPROXIMATE>(param0, param1, param2, param3, param4, param5);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
        }
    } else {
        // Do all four faces, and iterate through all 4 blocks of 4 rows each
#pragma GCC unroll 0
        for (int face = 0; face < 4; face++) {
            sfpu::calculate_sfpu<sfpu_op, APPROXIMATE>(param0, param1, param2, param3, param4, param5);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
            TTI_SETRWC(p_setrwc::CLR_NONE, p_setrwc::CR_D, 8, 0, 0, p_setrwc::SET_D);
        }
    }
    math::clear_dst_reg_addr();
}

template <SfpuType sfpu_op, bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_init(
    const uint operand, uint param0 = 0, uint param1 = 0, uint param2 = 0, uint param3 = 0, uint param4 = 0, uint param5 = 0) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_init<{}, {}>({}, {}, {}, {}, {}, {}, {})", sfpu_op, APPROXIMATE, operand, param0, param1, param2, param3, param4, param5);
    if constexpr (sfpu_op == SfpuType::dropout) {
        sfpu::sfpu_init<APPROXIMATE>(sfpu_op, param2);
    } else {
        sfpu::sfpu_init<APPROXIMATE>(sfpu_op);
    }
    math::reset_counters(p_setrwc::SET_ABD_F);
}

// New LLK SFPU APIs

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_exponential(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_exponential<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::exponential, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_exponential_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::exponential, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_sqrt(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_sqrt<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::sqrt, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_sqrt_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::sqrt, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_gelu(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_gelu<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::gelu, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_gelu_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::gelu, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_gelu_derivative(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_gelu_derivative<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::gelu_derivative, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_gelu_derivative_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::gelu_derivative, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_reciprocal(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_reciprocal<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::reciprocal, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_reciprocal_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::reciprocal, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_log(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_log<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::log, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_log_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::log, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_tanh(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_tanh<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::tanh, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_tanh_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::tanh, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_sigmoid(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_sigmoid<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::sigmoid, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_sigmoid_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::sigmoid, APPROXIMATE>(operand);
}

template <DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_dropout(const uint operand, uint dst_index, int vector_mode, int integer_dropout, int scale_factor) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_dropout<{}>({}, {}, {}, {})", dst_sync, dst_index, vector_mode, integer_dropout, scale_factor);
    constexpr bool dont_care = false;
    llk_math_eltwise_unary_sfpu<SfpuType::dropout, dont_care, dst_sync>(operand, dst_index, vector_mode, integer_dropout, scale_factor);
}

inline void llk_math_eltwise_unary_sfpu_dropout_init(const uint operand, uint seed = 0) {
    constexpr bool dont_care = false;
    constexpr uint dont_care_param = 0;

    llk_math_eltwise_unary_sfpu_init<SfpuType::dropout, dont_care>(operand, dont_care_param, dont_care_param, seed);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_max(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_max<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::max, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_max_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::max, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_square(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_square<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::square, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_square_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::square, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_power(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC, int pow = 0) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_power<{}, {}>({}, {}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode, pow);
    llk_math_eltwise_unary_sfpu<SfpuType::power, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode, pow);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_power_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::power, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_sine(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_sine<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::sine, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_sine_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::sine, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_cosine(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_cosine<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::cosine, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_cosine_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::cosine, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_lrelu(const uint operand, uint dst_index, int vector_mode, uint uint_slope) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_lrelu<{}, {}>({}, {}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode, uint_slope);
    llk_math_eltwise_unary_sfpu<SfpuType::lrelu, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode, uint_slope);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_lrelu_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::lrelu, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_relu_max(const uint operand, uint dst_index, int vector_mode, uint uint_threshold) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_relu_max<{}, {}>({}, {}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode, uint_threshold);
    llk_math_eltwise_unary_sfpu<SfpuType::relu_max, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode, uint_threshold);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_relu_max_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::relu_max, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_relu_min(const uint operand, uint dst_index, int vector_mode, uint uint_threshold) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_relu_min<{}, {}>({}, {}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode, uint_threshold);
    llk_math_eltwise_unary_sfpu<SfpuType::relu_min, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode, uint_threshold);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_relu_min_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::relu_min, APPROXIMATE>(operand);
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_abs(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    TT_LLK_DUMP("llk_math_eltwise_unary_sfpu_abs<{}, {}>({}, {})", APPROXIMATE, dst_sync, dst_index, vector_mode);
    llk_math_eltwise_unary_sfpu<SfpuType::abs, APPROXIMATE, dst_sync>(operand, dst_index, vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_abs_init(const uint operand) {
    llk_math_eltwise_unary_sfpu_init<SfpuType::abs, APPROXIMATE>(operand);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_cast_fp32_to_fp16a_init(const uint operand) {
    // Do nothing, as this llk does not exist on grayskull.
    // An empty definition is added here in order to match LLK apis between GS/WH_B0
}

template <bool APPROXIMATE, DstSync dst_sync = DstSync::SyncFull>
inline void llk_math_eltwise_unary_sfpu_cast_fp32_to_fp16a(const uint operand, uint dst_index, int vector_mode = (int)Dim::RC) {
    // Do nothing, as this llk does not exist on grayskull.
    // An empty defintion is added here in order to match LLK apis between GS/WH_B0
}