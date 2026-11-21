# export.asm - save the picture as a PNG (step 7, section 11.2).
# Translation of c/src/export_png.c (same function names, same bytes):
#   crc32_bytes, adler32_bytes, put_be32, export_png, do_save.
#
# The file is built in file_buf (main.asm, 52 KB) with byte stores, then
# one Open / Write / Close (syscalls 1024, 64, 57). The file name is a
# string constant here; RARS writes it into its working directory.
#
# Layout (section 11.2), n x n cells at EXPORT_SCALE 8 pixels each:
#   signature | IHDR | IDAT (78 01 | 01 | LEN | ~LEN | raw | adler32) | IEND
#   raw row = filter byte 0, then R G B per pixel; EMPTY is white.

.include "constants.asm"

        .data
export_name:    .asciz "pixel_painter.png"
msg_saved:      .asciz "\nSaved: pixel_painter.png\n"
msg_save_fail:  .asciz "\nSave failed\n"

        .text
        .globl  crc32_bytes, adler32_bytes, put_be32, export_png, do_save

# crc32_bytes(a0 = address, a1 = length) -> a0 = CRC-32
# Bitwise, reflected polynomial 0xEDB88320, init and final XOR 0xFFFFFFFF.
# Uses: a0, a1, t0-t4.  Calls: none (leaf).  Saves: nothing.
crc32_bytes:
        li      t0, -1                  # crc = 0xFFFFFFFF
        li      t3, 0xEDB88320          # polynomial
crc32_byte:
        beqz    a1, crc32_done
        lbu     t1, 0(a0)
        xor     t0, t0, t1
        li      t2, 8
crc32_bit:
        andi    t4, t0, 1               # low bit before the shift
        srli    t0, t0, 1
        beqz    t4, crc32_no_poly
        xor     t0, t0, t3
crc32_no_poly:
        addi    t2, t2, -1
        bnez    t2, crc32_bit
        addi    a0, a0, 1
        addi    a1, a1, -1
        j       crc32_byte
crc32_done:
        not     a0, t0                  # final XOR 0xFFFFFFFF
        ret

# adler32_bytes(a0 = address, a1 = length) -> a0 = (b << 16) | a
# a = 1 + sum of bytes, b = sum of a, both mod 65521 (section 11.2).
# Uses: a0, a1, t0-t3.  Calls: none (leaf).  Saves: nothing.
adler32_bytes:
        li      t0, 1                   # a
        li      t1, 0                   # b
        li      t3, 65521
adler32_byte:
        beqz    a1, adler32_done
        lbu     t2, 0(a0)
        add     t0, t0, t2
        remu    t0, t0, t3
        add     t1, t1, t0
        remu    t1, t1, t3
        addi    a0, a0, 1
        addi    a1, a1, -1
        j       adler32_byte
adler32_done:
        slli    t1, t1, 16
        or      a0, t1, t0
        ret

# put_be32(a0 = value, a1 = destination)
# Stores 4 bytes, most significant first. The caller moves its pointer.
# Uses: t0.  Calls: none (leaf).  Saves: nothing.
put_be32:
        srli    t0, a0, 24
        sb      t0, 0(a1)
        srli    t0, a0, 16
        sb      t0, 1(a1)
        srli    t0, a0, 8
        sb      t0, 2(a1)
        sb      a0, 3(a1)
        ret

# export_png() -> a0 = bytes in file_buf (0 if n is out of range)
# Writes the whole PNG for the current grid into file_buf. Locals are on
# the stack: 0 raw_len, 4 raw_start, 8 IDAT type, 12 IHDR type,
# 16 IEND type, 20 IDAT length.
# WRITES GLOBALS: none (reads grid and n).
# Uses: a0-a2, t0-t4, s8-s11.  Calls: grid_get, crc32_bytes, adler32_bytes,
# put_be32.  Saves: ra, s8-s11.  Reads globals: s0, s1 (n).
export_png:
        addi    sp, sp, -48
        sw      ra, 44(sp)
        sw      s8, 40(sp)
        sw      s9, 36(sp)
        sw      s10, 32(sp)
        sw      s11, 28(sp)

        li      t0, MIN_N
        blt     s1, t0, export_png_fail
        li      t0, MAX_N
        bgt     s1, t0, export_png_fail

        la      s9, file_buf            # s9 = start of the file
        mv      s8, s9                  # s8 = write pointer

        # raw_len = w * (1 + 3 * w), w = n * EXPORT_SCALE (8)
        slli    t0, s1, 3               # t0 = w
        slli    t1, t0, 1
        add     t1, t1, t0              # 3w
        addi    t1, t1, 1               # 1 + 3w
        mul     t1, t0, t1              # raw_len
        sw      t1, 0(sp)

        # signature
        li      t0, 0x89
        sb      t0, 0(s8)
        li      t0, 0x50                # P
        sb      t0, 1(s8)
        li      t0, 0x4E                # N
        sb      t0, 2(s8)
        li      t0, 0x47                # G
        sb      t0, 3(s8)
        li      t0, 0x0D
        sb      t0, 4(s8)
        li      t0, 0x0A
        sb      t0, 5(s8)
        li      t0, 0x1A
        sb      t0, 6(s8)
        li      t0, 0x0A
        sb      t0, 7(s8)
        addi    s8, s8, 8

        # IHDR: length 13, "IHDR", width, height, 8, 2, 0, 0, 0, CRC
        li      a0, 13
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4
        sw      s8, 12(sp)              # IHDR type
        li      t0, 0x49                # I
        sb      t0, 0(s8)
        li      t0, 0x48                # H
        sb      t0, 1(s8)
        li      t0, 0x44                # D
        sb      t0, 2(s8)
        li      t0, 0x52                # R
        sb      t0, 3(s8)
        addi    s8, s8, 4
        slli    a0, s1, 3               # width = n * 8
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4
        slli    a0, s1, 3               # height = n * 8
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4
        li      t0, 8                   # bit depth
        sb      t0, 0(s8)
        li      t0, 2                   # color type RGB
        sb      t0, 1(s8)
        sb      zero, 2(s8)             # compression
        sb      zero, 3(s8)             # filter
        sb      zero, 4(s8)             # interlace
        addi    s8, s8, 5
        lw      a0, 12(sp)              # CRC over type + data = 17 bytes
        li      a1, 17
        jal     crc32_bytes
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4

        # IDAT: length = raw_len + 11 (zlib header 2, block header 5, adler 4)
        lw      t1, 0(sp)
        addi    t2, t1, 11
        sw      t2, 20(sp)
        mv      a0, t2
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4
        sw      s8, 8(sp)               # IDAT type
        li      t0, 0x49                # I
        sb      t0, 0(s8)
        li      t0, 0x44                # D
        sb      t0, 1(s8)
        li      t0, 0x41                # A
        sb      t0, 2(s8)
        li      t0, 0x54                # T
        sb      t0, 3(s8)
        addi    s8, s8, 4
        li      t0, 0x78                # zlib header 78 01
        sb      t0, 0(s8)
        li      t0, 0x01
        sb      t0, 1(s8)
        sb      t0, 2(s8)               # BFINAL = 1, BTYPE = 00
        lw      t1, 0(sp)               # raw_len
        andi    t0, t1, 0xFF
        sb      t0, 3(s8)               # LEN, little-endian
        srli    t0, t1, 8
        andi    t0, t0, 0xFF
        sb      t0, 4(s8)
        not     t2, t1                  # NLEN = ~raw_len
        andi    t0, t2, 0xFF
        sb      t0, 5(s8)
        srli    t0, t2, 8
        andi    t0, t0, 0xFF
        sb      t0, 6(s8)
        addi    s8, s8, 7
        sw      s8, 4(sp)               # raw_start

        # raw rows: filter byte 0, then R G B for each pixel
        li      s10, 0                  # s10 = y
export_png_row:
        slli    t0, s1, 3
        bge     s10, t0, export_png_rows_done
        sb      zero, 0(s8)             # filter type 0
        addi    s8, s8, 1
        li      s11, 0                  # s11 = x
export_png_col:
        slli    t0, s1, 3
        bge     s11, t0, export_png_row_next
        srli    a0, s10, 3              # row = y / 8
        srli    a1, s11, 3              # col = x / 8
        jal     grid_get                # a0 = cell color
        li      t0, EMPTY
        bne     a0, t0, export_png_put
        li      a0, 0x00FFFFFF          # EMPTY is white
export_png_put:
        srli    t0, a0, 16
        sb      t0, 0(s8)               # R
        srli    t0, a0, 8
        sb      t0, 1(s8)               # G
        sb      a0, 2(s8)               # B
        addi    s8, s8, 3
        addi    s11, s11, 1
        j       export_png_col
export_png_row_next:
        addi    s10, s10, 1
        j       export_png_row
export_png_rows_done:

        # adler-32 of the raw data, then the IDAT CRC (type + data)
        lw      a0, 4(sp)
        lw      a1, 0(sp)
        jal     adler32_bytes
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4
        lw      a0, 8(sp)               # IDAT type
        lw      t0, 20(sp)
        addi    a1, t0, 4               # 4 (type) + IDAT length
        jal     crc32_bytes
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4

        # IEND: length 0, "IEND", CRC
        li      a0, 0
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4
        sw      s8, 16(sp)              # IEND type
        li      t0, 0x49                # I
        sb      t0, 0(s8)
        li      t0, 0x45                # E
        sb      t0, 1(s8)
        li      t0, 0x4E                # N
        sb      t0, 2(s8)
        li      t0, 0x44                # D
        sb      t0, 3(s8)
        addi    s8, s8, 4
        lw      a0, 16(sp)
        li      a1, 4
        jal     crc32_bytes
        mv      a1, s8
        jal     put_be32
        addi    s8, s8, 4

        sub     a0, s8, s9              # total bytes
        j       export_png_ret
export_png_fail:
        li      a0, 0
export_png_ret:
        lw      s11, 28(sp)
        lw      s10, 32(sp)
        lw      s9, 36(sp)
        lw      s8, 40(sp)
        lw      ra, 44(sp)
        addi    sp, sp, 48
        ret

# do_save() -> a0 = 1 if the file was written, 0 if not
# Ctrl+S / key s: export_png, then Open (write, create), Write, Close, and
# print "Saved: pixel_painter.png" on the console.
# Uses: a0-a2, a7, t0.  Calls: export_png.  Saves: ra.
# Locals (stack): 0 byte count, 4 file descriptor.
do_save:
        addi    sp, sp, -16
        sw      ra, 12(sp)
        jal     export_png
        beqz    a0, do_save_fail
        sw      a0, 0(sp)
        la      a0, export_name         # Open(name, 1 = write, create)
        li      a1, 1
        li      a2, 0
        li      a7, SYS_OPEN
        ecall
        bltz    a0, do_save_fail
        sw      a0, 4(sp)
        lw      a0, 4(sp)               # Write(fd, file_buf, bytes)
        la      a1, file_buf
        lw      a2, 0(sp)
        li      a7, SYS_WRITE
        ecall
        lw      a0, 4(sp)               # Close(fd)
        li      a7, SYS_CLOSE
        ecall
        la      a0, msg_saved
        li      a7, SYS_PRINT_STRING
        ecall
        li      a0, 1
        j       do_save_ret
do_save_fail:
        la      a0, msg_save_fail
        li      a7, SYS_PRINT_STRING
        ecall
        li      a0, 0
do_save_ret:
        lw      ra, 12(sp)
        addi    sp, sp, 16
        ret
