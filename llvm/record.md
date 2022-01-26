

# Introduction to the LLVM

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-02-47-25-image.png)

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-02-47-46-image.png)

SPEC

**The Standard Performance Evaluation Corporation (SPEC)** is a non-profit corporation formed to establish, maintain and endorse standardized benchmarks and tools to evaluate performance and energy efficiency for the newest generation of computing systems. SPEC develops benchmark suites and also reviews and publishes submitted results from our [member organizations](https://www.spec.org/consortium/) and other benchmark licensees.

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-02-43-39-image.png)

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-02-46-59-image.png)



# The Architecture of Open Source Applications_ LLVM

 The name "LLVM" was once an acronym, but is now just a brand for the umbrella project.



## Classical Compiler Design

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-02-59-04-image.png)

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-03-00-49-image.png)

With this design, porting the compiler to support a new source language (e.g., Algol or BASIC) requires implementing a new front end, but the existing optimizer and back end can be reused.

 serves a broader set of programmers than it would if it only supported one source language and one target.

the skills required to implement a front end are different than those required for the optimizer and back end.





While the benefits of a three-phase design are compelling and well-documented in compiler textbooks, in practice it is almost **never** fully realized.

```c
unsigned add1(unsigned a, unsigned b) {
  return a+b;
}

// Perhaps not the most efficient way to add two numbers.
unsigned add2(unsigned a, unsigned b) {
  if (a == 0) return b;
  return add2(a-1, b+1);
}
```

```ll
define i32 @add1(i32 %a, i32 %b) {
entry:
  %tmp1 = add i32 %a, %b
  ret i32 %tmp1
}

define i32 @add2(i32 %a, i32 %b) {
entry:
  %tmp1 = icmp eq i32 %a, 0
  br i1 %tmp1, label %done, label %recurse

recurse:
  %tmp2 = sub i32 %a, 1
  %tmp3 = add i32 %b, 1
  %tmp4 = call i32 @add2(i32 %tmp2, i32 %tmp3)
  ret i32 %tmp4

done:
  ret i32 %b
}
```

instructions are in three address form

LLVM is strongly typed with a simple type system

LLVM IR doesn't use a fixed set of named registers, it uses an infinite set of temporaries named with a % character.



Beyond being implemented as a language, LLVM IR is actually defined in three isomorphic forms: the textual format above, an in-memory data structure inspected and modified by optimizations themselves, and an efficient and dense on-disk binary "bitcode" format.



`llvm-as` assembles the textual `.ll` file into a `.bc` file containing the bitcode goop and `llvm-dis` turns a `.bc` file into a `.ll` file.



### IR Optimization

```ll
⋮    ⋮    ⋮
%example1 = sub i32 %a, %a
⋮    ⋮    ⋮
%example2 = sub i32 %b, 0
⋮    ⋮    ⋮
%tmp = mul i32 %c, 2
%example3 = sub i32 %tmp, %c
⋮    ⋮    ⋮
```

```cpp
// X - 0 -> X
if (match(Op1, m_Zero()))
  return Op0;

// X - X -> 0
if (Op0 == Op1)
  return Constant::getNullValue(Op0->getType());

// (X*2) - X -> X
if (match(Op0, m_Mul(m_Specific(Op1), m_ConstantInt<2>())))
  return Op1;

…

return 0;  // Nothing matched, return null to indicate no transformation.
```

```cpp
for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
  if (Value *V = SimplifyInstruction(I))
    I->replaceAllUsesWith(V);
```

This code simply loops over each instruction in a block, checking to see if any of them simplify. If so (because `SimplifyInstruction` returns non-null), it uses the `replaceAllUsesWith` method to update anything in the code using the simplifiable operation with the simpler form.





In an LLVM-based compiler, a front end is responsible for parsing, validating and diagnosing errors in the input code, then translating the parsed code into LLVM IR (usually, but not always, by building an AST and then converting the AST to LLVM IR). This IR is optionally fed through a series of analysis and optimization passes which improve the code, then is sent into a code generator to produce native machine code

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-03-42-40-image.png)



In particular, LLVM IR is both well specified and the *only* interface to the optimizer. This property means that all you need to know to write a front end for LLVM is what LLVM IR is, how it works, and the invariants it expects. Since LLVM IR has a first-class textual form, it is both possible and reasonable to build a front end that outputs LLVM IR as text, then uses Unix pipes to send it through the optimizer sequence and code generator of your choice.





### LLVM is a Collection of Libraries

 LLVM is an infrastructure, a collection of useful compiler technology that can be brought to bear on specific problems (like building a C compiler, or an optimizer in a special effects pipeline). While one of its most powerful features, it is also one of its least understood design points.

In LLVM (as in many other compilers) the optimizer is organized as a pipeline of distinct optimization passes each of which is run on the input and has a chance to do something

Depending on the optimization level, different passes are run: for example at -O0 (no optimization) the Clang compiler runs no passes, at -O3 it runs a series of 67 passes in its optimizer (as of LLVM 2.8).

Each LLVM pass is written as a C++ class that derives (indirectly) from the `Pass` class. Most passes are written in a single `.cpp` file, and their subclass of the `Pass` class is defined in an anonymous namespace (which makes it completely private to the defining file). In order for the pass to be useful, code outside the file has to be able to get it, so a **single function** (to create the pass) is exported from the file. Here is a slightly simplified example of a pass to make things concrete.[6](http://www.aosabook.org/en/llvm.html#footnote-6)

```cpp
namespace {
  class Hello : public FunctionPass {
  public:
    // Print out the names of functions in the LLVM IR being optimized.
    virtual bool runOnFunction(Function &F) {
      cerr << "Hello: " << F.getName() << "\n";
      return false;
    }
  };
}

FunctionPass *createHelloPass() { return new Hello(); }
```

As mentioned, the LLVM optimizer provides dozens of different passes, each of which are written in a similar style. These passes are compiled into one or more `.o` files, which are then built into a series of archive libraries (`.a` files on Unix systems). These libraries provide all sorts of analysis and transformation capabilities, and the passes are as loosely coupled as possible: they are expected to stand on their own, or explicitly declare their dependencies among other passes if they depend on some other analysis to do their job. When given a series of passes to run, the LLVM PassManager uses the explicit dependency information to satisfy these dependencies and optimize the execution of passes.



![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-15-33-49-image.png)

 the implementer is free to implement their own language-specific passes to cover for deficiencies in the LLVM optimizer or to explicit language-specific optimization opportunities.



## Design of the Retargetable LLVM Code Generator



Similar to the approach in the optimizer, LLVM's code generator splits the code generation problem into individual passes—instruction selection, register allocation, scheduling, code layout optimization, and assembly emission—and provides many builtin passes that are run by default.



### LLVM Target Description Files

The "mix and match" approach allows target authors to choose what makes sense for their architecture and permits a large amount of code reuse across different targets.each shared component needs to be able to reason about target specific properties in a generic way.LLVM's solution to this is for each target to provide a target description in a declarative domain-specific language (a set of `.td` files) processed by the tblgen tool. The (simplified) build process for the x86 target is shown in [Figure 11.5](http://www.aosabook.org/en/llvm.html#fig.llvm.x86).

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-15-40-24-image.png)



 different subsystems supported by the `.td` files allow target authors to build up the different pieces of their target.

```td
def GR32 : RegisterClass<[i32], 32,
  [EAX, ECX, EDX, ESI, EDI, EBX, EBP, ESP,
   R8D, R9D, R10D, R11D, R14D, R15D, R12D, R13D]> { … }
```

This definition says that registers in this class can hold 32-bit integer values ("i32"), prefer to be 32-bit aligned, have the specified 16 registers (which are defined elsewhere in the `.td` files) and have some more information to specify preferred allocation order and other things. Given this definition, specific instructions can refer to this, using it as an operand. For example, the "complement a 32-bit register" instruction is defined as:

```td
let Constraints = "$src = $dst" in
def NOT32r : I<0xF7, MRM2r,
               (outs GR32:$dst), (ins GR32:$src),
               "not{l}\t$dst",
               [(set GR32:$dst, (not GR32:$src))]>;
```

This definition says that NOT32r is an instruction (it uses the `I` tblgen class), specifies encoding information (`0xF7, MRM2r`), specifies that it defines an "output" 32-bit register `$dst` and has a 32-bit register "input" named `$src` (the `GR32` register class defined above defines which registers are valid for the operand), specifies the assembly syntax for the instruction (using the `{}` syntax to handle both AT&T and Intel syntax), specifies the effect of the instruction and provides the pattern that it should match on the last line. The "let" constraint on the first line tells the register allocator that the input and output register must be allocated to the same physical register.



While we aim to get as much target information as possible into the `.td` files in a nice declarative form, we still don't have everything. Instead, we require target authors to write some C++ code for various support routines and to implement any target specific passes they might need (like `X86FloatingPoint.cpp`, which handles the x87 floating point stack



## Interesting Capabilities Provided by a Modular Design

Besides being a generally elegant design, modularity provides clients of the LLVM libraries with several interesting capabilities. These capabilities stem from the fact that LLVM provides functionality, but lets the client decide most of the *policies* on how to use it.

### Choosing When and Where Each Phase Runs

Since LLVM IR is self-contained, and serialization is a lossless process, we can do part of compilation, save our progress to disk, then continue work at some point in the future.



This feature provides a number of interesting capabilities including support for link-time and install-time optimization, both of which delay code generation from "compile time".

Link-Time Optimization (LTO) addresses the problem where the compiler traditionally only sees one translation unit (e.g., a `.c` file with all its headers) at a time and therefore cannot do optimizations (like inlining) across file boundaries. LLVM compilers like Clang support this with the `-flto` or `-O4` command line option. This option instructs the compiler to emit LLVM bitcode to the `.o`file instead of writing out a native object file, and delays code generation to link time, shown in [Figure 11.6](http://www.aosabook.org/en/llvm.html#fig.llvm.lto).

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-15-58-14-image.png)

Since the optimizer can now see across a much larger portion of the code, it can inline, propagate constants, do more aggressive dead code elimination, and more across file boundaries. While many modern compilers support LTO, most of them (e.g., GCC, Open64, the Intel compiler, etc.) do so by having an expensive and slow serialization process. In LLVM, LTO falls out naturally from the design of the system, and works across different source languages (unlike many other compilers) because the IR is truly source language neutral.

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-16-04-49-image.png)

Install-time optimization is the idea of delaying code generation even later than link time, all the way to install time, as shown in [Figure 11.7](http://www.aosabook.org/en/llvm.html#fig.llvm.ito). Install time is a very interesting time (in cases when software is shipped in a box, downloaded, uploaded to a mobile device, etc.), because this is when you find out the specifics of the device you're targeting.



### Unit Testing the Optimizer

The traditional approach to testing this is to write a `.c` file (for example) that is run through the compiler, and to have a test harness that verifies that the compiler doesn't crash.

The problem with this approach is that the compiler consists of many different subsystems and even many different passes in the optimizer, all of which have the opportunity to change what the input code looks like by the time it gets to the previously buggy code in question. If something changes in the front end or an earlier optimizer, a test case can easily fail to test what it is supposed to be testing.

By using the textual form of LLVM IR with the modular optimizer, the LLVM test suite has highly focused regression tests that can load LLVM IR from disk, run it through exactly one optimization pass, and verify the expected behavior. Beyond crashing, a more complicated behavioral test wants to verify that an optimization is actually performed. Here is a simple test case that checks to see that the constant propagation pass is working with add instructions:

```cpp
; RUN: opt < %s -constprop -S | FileCheck %s
define i32 @test() {
  %A = add i32 4, 5
  ret i32 %A
  ; CHECK: @test()
  ; CHECK: ret i32 9
}
```

The `RUN` line specifies the command to execute: in this case, the `opt` and `FileCheck` command line tools. The `opt` program is a simple wrapper around the LLVM pass manager, which links in all the standard passes (and can dynamically load plugins containing other passes) and exposes them through to the command line. The `FileCheck` tool verifies that its standard input matches a series of `CHECK` directives. In this case, this simple test is verifying that the `constprop` pass is folding the `add` of 4 and 5 into 9.

While this might seem like a really trivial example, this is very difficult to test by writing .c files: front ends often do constant folding as they parse, so it is very difficult and fragile to write code that makes its way downstream to a constant folding optimization pass. Because we can load LLVM IR as text and send it through the specific optimization pass we're interested in, then dump out the result as another text file, it is really straightforward to test exactly what we want, both for regression and feature tests.



### Automatic Test Case Reduction with BugPoint

The LLVM BugPoint tool[7](http://www.aosabook.org/en/llvm.html#footnote-7) uses the IR serialization and modular design of LLVM to automate this process. For example, given an input `.ll` or `.bc` file along with a list of optimization passes that causes an optimizer crash, BugPoint reduces the input to a small test case and determines which optimizer is at fault. It then outputs the reduced test case and the `opt` command used to reproduce the failure. It finds this by using techniques similar to "delta debugging" to reduce the input and the optimizer pass list. Because it knows the structure of LLVM IR, BugPoint does not waste time generating invalid IR to input to the optimizer, unlike the standard "delta" command line tool.

In the more complex case of a miscompilation, you can specify the input, code generator information, the command line to pass to the executable, and a reference output. BugPoint will first determine if the problem is due to an optimizer or a code generator, and will then repeatedly partition the test case into two pieces: one that is sent into the "known good" component and one that is sent into the "known buggy" component. By iteratively moving more and more code out of the partition that is sent into the known buggy code generator, it reduces the test case.

## Retrospective and Future Directions

Looking forward, we would like to continue making LLVM more modular and easier to subset.

Despite its success so far, there is still a lot left to be done, as well as the ever-present risk that LLVM will become less nimble and more calcified as it ages. While there is no magic answer to this problem, I hope that the continued exposure to new problem domains, a willingness to reevaluate previous decisions, and to redesign and throw away code will help. After all, the goal isn't to be perfect, it is to keep getting better over time.











```shell
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
cmake -DLLVM_ENABLE_PROJECTS=clang -G "Visual Studio 17 2022" -A x64 -Thost=x64 ..\llvm
-Thost=x64

Open LLVM.sln in Visual Studio.
Build the "clang" project for just the compiler driver and front end, or the "ALL_BUILD" project to build everything, including tools.
```





![_images/vs_build_llvm.png](https://getting-started-with-llvm-core-libraries-zh-cn.readthedocs.io/zh_CN/latest/_images/vs_build_llvm.png)

需要以管理员方式运行

默认的存放位置

> D:\vs_community\SoftwareData\VC\Tools\Llvm\bin

输入

```shell
clang.exe -v
```

得到:

```shell
clang version 12.0.0
Target: i686-pc-windows-msvc
Thread model: posix
InstalledDir: D:\vs_community\SoftwareData\VC\Tools\Llvm\bin
```

并将/bin文件夹添加到Path系统变量中

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-22-07-41-image.png)

生成



![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-22-22-38-image.png)

生成

这一步有错误

[JSONTest.Integers是闪烁的 · llvm-project答疑](https://ddeevv.com/question/llvm-llvm-project-45815.html)

https://bugs.chromium.org/p/chromium/issues/detail?id=1099867#c4

微软的问题

========== Build: 478 succeeded, 28 failed, 54 up-to-date, 0 skipped ==========

工具 -> 获取工具和功能->语言包

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-23-02-07-image.png)



文件->账户选项->区域

![](C:\Users\zyy\OneDrive\Desktop\llvm\assets\2022-01-26-23-04-48-image.png)

新建一个hello.c

```c
#include <stdio.h>

int main() {
  printf("hello world\n");
  return 0;
}
```

.bc

```shell
clang -emit-llvm -c hello.c -o hello.bc
```

.ll

```shell
clang -O0 hello.c -emit-llvm -S -c -o hello.ll
```



compile

```shell
clang hello.c -o hello
clang -O0 hello.c -emit-llvm -S -c -o hello.ll
```


