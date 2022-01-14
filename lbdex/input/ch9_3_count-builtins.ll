; ModuleID = 'ch9_3_count-builtins.c'
source_filename = "ch9_3_count-builtins.c"
target datalayout = "E-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64"
target triple = "cpu0-unknown-linux-gnu"

@leading = dso_local global i32 0, align 4
@trailing = dso_local global i32 0, align 4
@pop = dso_local global i32 0, align 4

; Function Attrs: noinline nounwind optnone
define dso_local void @test_i64(float %P) #0 {
entry:
  %P.addr = alloca float, align 4
  store float %P, float* %P.addr, align 4
  %0 = load float, float* %P.addr, align 4
  %conv = fptoui float %0 to i64
  %1 = call i64 @llvm.ctlz.i64(i64 %conv, i1 true)
  %cast = trunc i64 %1 to i32
  store i32 %cast, i32* @leading, align 4
  ret void
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare i64 @llvm.ctlz.i64(i64, i1 immarg) #1

attributes #0 = { noinline nounwind optnone "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cpu032II" "target-features"="+HasCmp,+HasSlt" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 12.0.1 (https://github.com/llvm/llvm-project.git e8a397203c67adbeae04763ce25c6a5ae76af52c)"}
