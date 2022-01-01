#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
using namespace llvm;

static std::unique_ptr<LLVMContext> TheContext; // owns a lot of core LLVM data structures
static std::unique_ptr<IRBuilder<>> Builder; // a helper object that makes it easy to generate LLVM instructions
static std::unique_ptr<Module> TheModule; // contains functions and global variables
// keeps track of which values are defined in the current scope and what their LLVM representation is
static std::map<std::string, Value *> NamedValues; 

