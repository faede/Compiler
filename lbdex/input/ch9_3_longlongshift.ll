; ModuleID = 'ch9_3_longlongshift.cpp'
source_filename = "ch9_3_longlongshift.cpp"
target datalayout = "E-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64"
target triple = "cpu0-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone mustprogress
define dso_local i64 @_Z20test_longlong_shift1v() #0 {
entry:
  %a = alloca i64, align 8
  %b = alloca i64, align 8
  %c = alloca i64, align 8
  %d = alloca i64, align 8
  %e = alloca i64, align 8
  store i64 4, i64* %a, align 8
  store i64 18, i64* %b, align 8
  %0 = load i64, i64* %b, align 8
  %1 = load i64, i64* %a, align 8
  %shr = ashr i64 %0, %1
  store i64 %shr, i64* %c, align 8
  %2 = load i64, i64* %b, align 8
  %3 = load i64, i64* %a, align 8
  %shl = shl i64 %2, %3
  store i64 %shl, i64* %d, align 8
  store i64 0, i64* %e, align 8
  %4 = load i64, i64* %c, align 8
  %5 = load i64, i64* %d, align 8
  %add = add nsw i64 %4, %5
  %6 = load i64, i64* %e, align 8
  %add1 = add nsw i64 %add, %6
  ret i64 %add1
}

; Function Attrs: noinline nounwind optnone mustprogress
define dso_local i64 @_Z20test_longlong_shift2v() #0 {
entry:
  %a = alloca i64, align 8
  %b = alloca i64, align 8
  %c = alloca i64, align 8
  store i64 48, i64* %a, align 8
  store i64 6305037760331786, i64* %b, align 8
  %0 = load i64, i64* %b, align 8
  %1 = load i64, i64* %a, align 8
  %shr = ashr i64 %0, %1
  store i64 %shr, i64* %c, align 8
  %2 = load i64, i64* %c, align 8
  ret i64 %2
}

attributes #0 = { noinline nounwind optnone mustprogress "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cpu032II" "target-features"="+HasCmp,+HasSlt" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 12.0.1 (https://github.com/llvm/llvm-project.git e8a397203c67adbeae04763ce25c6a5ae76af52c)"}
