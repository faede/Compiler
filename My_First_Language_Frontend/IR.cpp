#include "KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "IR.hpp"
#include "Paser.hpp"



//using namespace llvm::legacy;
using namespace llvm;
using namespace llvm::orc;


Value * LogErrorV(const char *  Str){
	LogErrorV(Str);
	return nullptr;
}




Function *getFunction(std::string Name){
	// check exist
	if(auto *F = TheModule->getFunction(Name))
		return F;
	// check whether we can codegen the declaration from some existing prototype
	auto FI = FunctionProtos.find(Name);
	if(FI != FunctionProtos.end())
		return FI->second->codegen();

	// no exist 
	return nullptr;
}



// in the LLVM IR that constants are all uniqued together and shared
// So APU use get() rather than new
Value * NumberExprAST::codegen(){
	return ConstantFP::get(TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  Value *V = NamedValues[Name];
  if (!V)
    return LogErrorV("Unknown variable name");
  return V;
}


Value * BinaryExprAST::codegen(){
	Value * L = LHS->codegen();
	Value * R = RHS->codegen();
	if(!L || !R)
		return nullptr;

	switch(Op){
		case '+':
			return Builder.CreateFAdd(L, R, "addtmp");
		case '-':
			return Builder.CreateFSub(L, R, "subtmp");
		case '*':
			return Builder.CreateFMul(L, R, "multmp");
		case '<':
			L = Builder.CreateFCmpULT(L, R, "cmptmp");
			// convert bool to double 0.0 or 1.0 by treat input as unsigned value
			return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
		default:
			return LogErrorV("Invalid binary operator");
	}
}


// LLVM uses the native C calling conventions by default, 
// allowing these calls to also call into standard library functions,
// with no additional effort
Value * CallExprAST::codegen(){
	// find name in globe module table
	Function * CalleeF = getFunction(Callee);
	if(!CalleeF)
		return LogErrorV("Unknown function referenced");

	if(CalleeF->arg_size() != Args.size())
		return LogErrorV("Incorrect # arguments passed");

	std::vector<Value *> ArgsV;
	for(unsigned i = 0, e = Args.size(); i != e; ++i){
		ArgsV.push_back(Args[i]->codegen());
		if(!ArgsV.back())
			return nullptr;
	}

	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}


Function * PrototypeAST::codegen(){
	// type: double (double , double) etc
	// There is not vararg (the false parameter indicates this)
	std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));

	FunctionType * FT = 
		FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

	// “external linkage” means that the function may be defined outside the current
	//  module and/or that it is callable by functions outside the module
	Function * F =
		Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

	// Set namesfor all arguments
	unsigned Idx = 0;
	for(auto &Arg : F->args())
		Arg.setName(Args[Idx++]);

	return F;
}


Function * FunctionAST::codegen(){
	// Transfer ownership of the prototype to the FunctionProtos map, but keep a
    // reference to it for use below
	auto & P = *Proto;
	FunctionProtos[Proto->getName()] = std::move(Proto); 

	// First, check for an existing function from a previous 'extern' declaration.
	Function * TheFunction = getFunction(P.getName());

	// If Module::getFunction returns null then no previous version exists
	// ??? I think shouldn't be deleted
	if(!TheFunction)
		TheFunction = Proto->codegen();

	if(!TheFunction)
		return nullptr;

	// Create a new basic block to start
	BasicBlock * BB = BasicBlock::Create(TheContext, "entry", TheFunction);
	Builder.SetInsertPoint(BB);

	// Record arguments
	NamedValues.clear();
	for(auto & Arg : TheFunction->args())
		NamedValues[std::string(Arg.getName())] = &Arg;

	if(Value * RetVal = Body->codegen()){
		// finish
		Builder.CreateRet(RetVal);

		// This function does a variety of consistency checks on the generated code, 
		// to determine if our compiler is doing everything right
		// it can catch a lot of bugs
		verifyFunction(*TheFunction);

		// Optimize the function
		TheFPM->run(*TheFunction);

		return TheFunction;
	}else{
		// error body, remove function
		TheFunction->eraseFromParent();
		return nullptr;
	}

	// TODO:
	// an earlier ‘extern’ declaration will take precedence over the function 
	// definition’s signature, which can cause codegen to fail, for instance if the function
	// arguments are named differently
}






































