// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "ckernel.h"

// Debug bus and register array dump

namespace ckernel
{

struct dbg_array_id
{
    constexpr static uint32_t SRCA_B0 = 0; // SrcA Bank0 
    constexpr static uint32_t SRCA_B1 = 8; // SrcA Bank1
    constexpr static uint32_t SRCB_B0 = 1; // SrcB Bank0
    constexpr static uint32_t SRCB_B1 = 9; // SrcB Bank1
    constexpr static uint32_t DEST = 2; // Dest acc
};

struct dbg_daisy_id
{
    constexpr static uint32_t INSTR_ISSUE_0 = 4; 
    constexpr static uint32_t INSTR_ISSUE_1 = 5; 
};

typedef struct
{
    uint32_t sig_sel : 16;
    uint32_t daisy_sel : 8;
    uint32_t rd_sel : 4;
    uint32_t reserved_0: 1;
    uint32_t en: 1;
    uint32_t reserved_1: 2;
} dbg_bus_cntl_t;

typedef union
{
    uint32_t val;
    dbg_bus_cntl_t f;
} dbg_bus_cntl_u;;

typedef struct
{
    uint32_t en : 1;
    uint32_t reserved : 31;
} dbg_array_rd_en_t;

typedef union
{
    uint32_t val;
    dbg_array_rd_en_t f;
} dbg_array_rd_en_u;

typedef struct
{
    uint32_t row_addr : 16;
    uint32_t array_id : 4;
    uint32_t reserved : 12;
} dbg_array_rd_cmd_t;

typedef union
{
    uint32_t val;
    dbg_array_rd_cmd_t f;
} dbg_array_rd_cmd_u; 

typedef struct
{
    uint32_t unp : 2;
    uint32_t pack : 4;
    uint32_t reserved : 26;
} dbg_soft_reset_t;

typedef union
{
    uint32_t val;
    dbg_soft_reset_t f;
} dbg_soft_reset_u;;

template <ThreadId thread_id> 
inline void dbg_thread_halt() {
    static_assert((thread_id == ThreadId::MathThreadId) || (thread_id == ThreadId::UnpackThreadId) || (thread_id == ThreadId::PackThreadId), "Invalid thread id set in dbg_wait_for_thread_idle(...)");

    if constexpr (thread_id == ThreadId::UnpackThreadId) {
        // Wait for all instructions on the running thread to complete
        tensix_sync();
        // Notify math thread that unpack thread is idle
        mailbox_write(ThreadId::MathThreadId, 1);
        // Wait for math thread to complete debug dump 
        volatile uint32_t temp = mailbox_read(ThreadId::MathThreadId);
    } else if constexpr (thread_id == ThreadId::MathThreadId) {
        // Wait for all instructions on the running thread to complete
        tensix_sync();
        // Wait for unpack thread to complete
        volatile uint32_t temp = mailbox_read(ThreadId::UnpackThreadId);
        // Wait for previous packs to finish 
        while (semaphore_read(semaphore::MATH_PACK) > 0) { }; 
    }
}

template <ThreadId thread_id> 
inline void dbg_thread_unhalt() {
    static_assert((thread_id == ThreadId::MathThreadId) || (thread_id == ThreadId::UnpackThreadId) || (thread_id == ThreadId::PackThreadId), "Invalid thread id set in dbg_wait_for_thread_idle(...)");

    if constexpr (thread_id == ThreadId::MathThreadId) {
        // Reset pack 0 (workaround)
        dbg_soft_reset_u dbg_soft_reset;
        dbg_soft_reset.val = 0;
        dbg_soft_reset.f.pack = 1;
        reg_write(RISCV_DEBUG_REG_SOFT_RESET_0, dbg_soft_reset.val);
        wait(5);
        dbg_soft_reset.val = 0;
        reg_write(RISCV_DEBUG_REG_SOFT_RESET_0, dbg_soft_reset.val);

        // Unhalt unpack thread
        mailbox_write(ThreadId::UnpackThreadId, 1);
    }
}

inline void dbg_get_array_row(const uint32_t array_id, const uint32_t row_addr, uint32_t *rd_data) {
    dbg_array_rd_en_u dbg_array_rd_en;
    dbg_array_rd_en.val = 0;
    dbg_array_rd_en.f.en = 0x1;
    reg_write(RISCV_DEBUG_REG_DBG_ARRAY_RD_EN, dbg_array_rd_en.val);

    dbg_array_rd_cmd_u dbg_array_rd_cmd;
    dbg_array_rd_cmd.val = 0;
    dbg_array_rd_cmd.f.row_addr = row_addr;
    dbg_array_rd_cmd.f.array_id = array_id;
    reg_write(RISCV_DEBUG_REG_DBG_ARRAY_RD_CMD, dbg_array_rd_cmd.val);

    dbg_bus_cntl_u dbg_bus_cntl;
    dbg_bus_cntl.val = 0;
    dbg_bus_cntl.f.sig_sel = 0x0;
    dbg_bus_cntl.f.daisy_sel = dbg_daisy_id::INSTR_ISSUE_0;
    dbg_bus_cntl.f.en = 1;

    for (uint32_t i=0; i<8; i++) {
       dbg_bus_cntl.f.rd_sel = i<<1; // Sel 16-bit
       reg_write(RISCV_DEBUG_REG_DBG_BUS_CNTL_REG, dbg_bus_cntl.val);
       wait (5); // Wait for value to get stable
       rd_data[i] = reg_read(RISCV_DEBUG_REG_DBG_RD_DATA);
    }

    // Disable debug control
    dbg_bus_cntl.val = 0;
    reg_write(RISCV_DEBUG_REG_DBG_BUS_CNTL_REG, dbg_bus_cntl.val);
    dbg_array_rd_cmd.val = 0;
    reg_write(RISCV_DEBUG_REG_DBG_ARRAY_RD_CMD, dbg_array_rd_cmd.val);
    dbg_array_rd_en.val = 0;
    reg_write(RISCV_DEBUG_REG_DBG_ARRAY_RD_EN, dbg_array_rd_en.val);

}
  
} // namespace ckernel
