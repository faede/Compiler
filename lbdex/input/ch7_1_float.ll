; ModuleID = 'ch7_1_float.cpp'
source_filename = "ch7_1_float.cpp"
target datalayout = "E-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64"
target triple = "mips-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone
define dso_local float @_Z10test_floatv() #0 {
  %1 = alloca float, align 4
  %2 = alloca float, align 4
  %3 = alloca float, align 4
  store float 0x4008CCCCC0000000, float* %1, align 4
  store float 0x40019999A0000000, float* %2, align 4
  %4 = load float, float* %1, align 4
  %5 = load float, float* %2, align 4
  %6 = fadd float %4, %5
  store float %6, float* %3, align 4
  %7 = load float, float* %3, align 4
  ret float %7
}

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="mips32r2" "target-features"="+mips32r2,-noabicalls" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"Apple clang version 12.0.5 (clang-1205.0.22.9)"}
