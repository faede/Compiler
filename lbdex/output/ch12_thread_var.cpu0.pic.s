  .text
  .section .mdebug.abiO32
  .previous
  .file  "ch12_thread_var.cpp"
  .globl  _Z15test_thread_varv            # -- Begin function _Z15test_thread_va
rv
  .p2align  1
  .type  _Z15test_thread_varv,@function
  .ent  _Z15test_thread_varv            # @_Z15test_thread_varv
_Z15test_thread_varv:
  .frame  $fp,16,$lr
  .mask   0x00005000,-4
  .set  noreorder
  .cpload  $t9
  .set  nomacro
# %bb.0:                                # %entry
  addiu  $sp, $sp, -16
  st  $lr, 12($sp)                    # 4-byte Folded Spill
  st  $fp, 8($sp)                     # 4-byte Folded Spill
  move  $fp, $sp
  .cprestore  8
  ori  $4, $gp, %tlsldm(a)
  ld  $t9, %call16(__tls_get_addr)($gp)
  jalr  $t9
  nop
  ld  $gp, 8($fp)
  lui  $3, %dtp_hi(a)
  addu  $2, $3, $2
  ori  $2, $2, %dtp_lo(a)
  addiu  $3, $zero, 2
  st  $3, 0($2)
  ld  $2, 0($2)
  move  $sp, $fp
  ld  $fp, 8($sp)                     # 4-byte Folded Reload
  ld  $lr, 12($sp)                    # 4-byte Folded Reload
  addiu  $sp, $sp, 16
  ret  $lr
  nop
  .set  macro
  .set  reorder
  .end  _Z15test_thread_varv
$func_end0:
  .size  _Z15test_thread_varv, ($func_end0)-_Z15test_thread_varv
                                        # -- End function
  .globl  _Z17test_thread_var_2v          # -- Begin function _Z17test_thread_va
r_2v
  .p2align  1
  .type  _Z17test_thread_var_2v,@function
  .ent  _Z17test_thread_var_2v          # @_Z17test_thread_var_2v
_Z17test_thread_var_2v:
  .frame  $fp,16,$lr
  .mask   0x00005000,-4
  .set  noreorder
  .cpload  $t9
  .set  nomacro
# %bb.0:                                # %entry
  addiu  $sp, $sp, -16
  st  $lr, 12($sp)                    # 4-byte Folded Spill
  st  $fp, 8($sp)                     # 4-byte Folded Spill
  move  $fp, $sp
  .cprestore  8
  ori  $4, $gp, %tlsldm(b)
  ld  $t9, %call16(__tls_get_addr)($gp)
  jalr  $t9
  nop
  ld  $gp, 8($fp)
  lui  $3, %dtp_hi(b)
  addu  $2, $3, $2
  ori  $2, $2, %dtp_lo(b)
  addiu  $3, $zero, 3
  st  $3, 0($2)
  ld  $2, 0($2)
  move  $sp, $fp
  ld  $fp, 8($sp)                     # 4-byte Folded Reload
  ld  $lr, 12($sp)                    # 4-byte Folded Reload
  addiu  $sp, $sp, 16
  ret  $lr
  nop
  .set  macro
  .set  reorder
  .end  _Z17test_thread_var_2v
$func_end1:
  .size  _Z17test_thread_var_2v, ($func_end1)-_Z17test_thread_var_2v
                                        # -- End function
  .section  .text._ZTW1b,"axG",@progbits,_ZTW1b,comdat
  .hidden  _ZTW1b                          # -- Begin function _ZTW1b
  .weak  _ZTW1b
  .p2align  1
  .type  _ZTW1b,@function
  .ent  _ZTW1b                          # @_ZTW1b
_ZTW1b:
  .cfi_startproc
  .frame  $fp,16,$lr
  .mask   0x00005000,-4
  .set  noreorder
  .cpload  $t9
  .set  nomacro
# %bb.0:
  addiu  $sp, $sp, -16
  .cfi_def_cfa_offset 16
  st  $lr, 12($sp)                    # 4-byte Folded Spill
  st  $fp, 8($sp)                     # 4-byte Folded Spill
  .cfi_offset 14, -4
  .cfi_offset 12, -8
  move  $fp, $sp
  .cfi_def_cfa_register 12
  .cprestore  8
  ld  $t9, %call16(__tls_get_addr)($gp)
  ori  $4, $gp, %tlsldm(b)
  jalr  $t9
  nop
  ld  $gp, 8($fp)
  lui  $3, %dtp_hi(b)
  addu  $2, $3, $2
  ori  $2, $2, %dtp_lo(b)
  move  $sp, $fp
  ld  $fp, 8($sp)                     # 4-byte Folded Reload
  ld  $lr, 12($sp)                    # 4-byte Folded Reload
  addiu  $sp, $sp, 16
  ret  $lr
  nop
  .set  macro
  .set  reorder
  .end  _ZTW1b
$func_end2:
  .size  _ZTW1b, ($func_end2)-_ZTW1b
  .cfi_endproc
                                        # -- End function
  .type  a,@object                       # @a
  .section  .tbss,"awT",@nobits
  .globl  a
  .p2align  2
a:
$a$local:
  .4byte  0                               # 0x0
  .size  a, 4

  .type  b,@object                       # @b
  .globl  b
  .p2align  2
b:
$b$local:
  .4byte  0                               # 0x0
  .size  b, 4

  .ident  "clang version 12.0.1 (https://github.com/llvm/llvm-project.git e8a397
203c67adbeae04763ce25c6a5ae76af52c)"
  .section  ".note.GNU-stack","",@progbits
