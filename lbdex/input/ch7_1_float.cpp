// clang -target mips-unknown-linux-gnu -S ch7_1_float.cpp -emit-llvm -o ch7_1_float.ll
// ~/llvm/test/build/bin/llc -march=cpu0 -mcpu=cpu032I -relocation-model=pic -filetype=asm ch7_1_float.ll -o -

/// start
float test_float()
{
  float a = 3.1;
  float b = 2.2;
  float c = a + b;

  return c;
}

