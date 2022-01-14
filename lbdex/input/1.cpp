// clang -target mips-unknown-linux-gnu -c ch4_2_logic.cpp -emit-llvm -o ch4_2_logic.bc
// ~/llvm/test/build/bin/llc -march=cpu0 -mcpu=cpu032I -relocation-model=pic -filetype=asm ch4_2_logic.bc -o -
// ~/llvm/test/build/bin/llc -march=cpu0 -mcpu=cpu032II -relocation-model=pic -filetype=asm ch4_2_logic.bc -o -


/// start
int test_not()
{
  int a = 0;
  int b = 0;

  b = ~a;
  
  return b;
}

