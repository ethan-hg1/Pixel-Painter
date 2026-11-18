# history.asm - undo/redo ring buffer (section 10).
# Translation of c/src/history.c (same function names). The storage is in
# main.asm's .data: hist_records, hist_snaps, hist_start, hist_len, hist_pos.
#
# Record (32 bytes): +0 type (1 PAINT, 2 CLEAR, 3 RESIZE), +4 row, +8 col,
# +12 old_color, +16 new_color, +20 old_n, +24 new_n, +28 unused.
# Snapshot slot i is 256 words (1 KB) at hist_snaps + i * 1024; it holds
# the whole grid (stride 16) taken BEFORE a CLEAR or RESIZE.
#
# Reads globals: s0 (GRID_BASE, for the snapshot copy in history_push).

.include "constants.asm"

        .text
        .globl  history_clear, history_slot, history_push
        .globl  history_undo_index, history_redo_index

# history_clear()
# Empties the history: no records, nothing to undo or redo.
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
history_clear:
        la      t0, hist_start
        sw      zero, 0(t0)
        la      t0, hist_len
        sw      zero, 0(t0)
        la      t0, hist_pos
        sw      zero, 0(t0)
        ret

# history_slot(a0 = i) -> a0 = (hist_start + i) & 63
# Uses: a0, t0.  Calls: none (leaf).  Saves: nothing.
history_slot:
        la      t0, hist_start
        lw      t0, 0(t0)
        add     a0, a0, t0
        andi    a0, a0, 63              # HISTORY_DEPTH - 1
        ret

# history_push(a0 = record address) -> nothing
# Adds the 32-byte record at a0 as the newest one. Drops the redo tail; if
# the ring is full, forgets the oldest record. For CLEAR and RESIZE, copies
# the grid (s0) into the slot's snapshot, so call this BEFORE the change.
# WRITES GLOBALS: none (hist_* only).
# Uses: a0-a2, t0-t6.  Calls: history_slot.  Saves: ra, s8-s10.
history_push:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        sw      s8, 8(sp)
        sw      s9, 4(sp)
        sw      s10, 0(sp)
        mv      s8, a0                  # s8 = record address

        la      t0, hist_pos
        lw      t1, 0(t0)               # t1 = pos
        la      t2, hist_len
        sw      t1, 0(t2)               # len = pos (drop the redo tail)
        li      t3, 64                  # HISTORY_DEPTH
        bne     t1, t3, hp_room
        # ring full: forget the oldest record
        la      t4, hist_start
        lw      t5, 0(t4)
        addi    t5, t5, 1
        andi    t5, t5, 63
        sw      t5, 0(t4)               # start = (start + 1) & 63
        addi    t1, t1, -1
        sw      t1, 0(t2)               # len = 63
        sw      t1, 0(t0)               # pos = 63
hp_room:
        mv      a0, t1                  # slot(pos)
        jal     history_slot
        mv      s9, a0                  # s9 = slot

        # hist_records[slot] = *record (8 words, the unused one included)
        la      t0, hist_records
        slli    t1, s9, 5
        add     s10, t0, t1             # s10 = destination record
        li      t2, 0
hp_copy:
        add     t3, s8, t2
        lw      t4, 0(t3)
        add     t5, s10, t2
        sw      t4, 0(t5)
        addi    t2, t2, 4
        li      t6, 32
        blt     t2, t6, hp_copy

        # CLEAR and RESIZE keep a full grid snapshot (PAINT does not)
        lw      t0, 0(s8)               # type
        li      t1, 1                   # PAINT
        beq     t0, t1, hp_no_snap
        la      t2, hist_snaps
        slli    t3, s9, 10
        add     t2, t2, t3              # t2 = destination snapshot
        mv      t3, s0                  # t3 = grid (source)
        li      t4, 256                 # MAX_N * MAX_N words
hp_snap:
        lw      t5, 0(t3)
        sw      t5, 0(t2)
        addi    t3, t3, 4
        addi    t2, t2, 4
        addi    t4, t4, -1
        bnez    t4, hp_snap
hp_no_snap:
        la      t0, hist_pos            # pos++, len = pos
        lw      t1, 0(t0)
        addi    t1, t1, 1
        sw      t1, 0(t0)
        la      t0, hist_len
        sw      t1, 0(t0)

        lw      s10, 0(sp)
        lw      s9, 4(sp)
        lw      s8, 8(sp)
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret

# history_undo_index() -> a0 = ring slot of the record to undo, or -1
# Moves pos back one record. -1 (and no change) when pos is 0.
# Uses: a0, t0, t1.  Calls: history_slot (tail jump).  Saves: nothing.
history_undo_index:
        la      t0, hist_pos
        lw      t1, 0(t0)
        beqz    t1, hu_none
        addi    t1, t1, -1
        sw      t1, 0(t0)
        mv      a0, t1
        j       history_slot            # returns to our caller
hu_none:
        li      a0, -1
        ret

# history_redo_index() -> a0 = ring slot of the record to redo, or -1
# Returns the record at pos and moves pos forward. -1 when pos == len.
# Uses: a0-a1, t0-t3.  Calls: history_slot (tail jump).  Saves: nothing.
history_redo_index:
        la      t0, hist_pos
        lw      t1, 0(t0)
        la      t2, hist_len
        lw      t3, 0(t2)
        beq     t1, t3, hr_none
        mv      a0, t1                  # slot(pos) before the step
        addi    t1, t1, 1
        sw      t1, 0(t0)
        j       history_slot            # returns to our caller
hr_none:
        li      a0, -1
        ret
