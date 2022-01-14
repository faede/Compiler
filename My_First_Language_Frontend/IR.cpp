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


Value * IfExprAST::codegen(){
	Value * CondV = Cond->codegen();
	if(!CondV)
		return nullptr;

	// Convert to bool by compare non-equal to 0.0 
	CondV = Builder.CreateFCmpONE(CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");

	Function * TheFunction = Builder.GetInsertBlock()->getParent();

	// create blocks for then and else
	// insert then block at the end of function
	BasicBlock * ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
	BasicBlock * ElseBB = BasicBlock::Create(TheContext, "else");
	BasicBlock * MergeBB = BasicBlock::Create(TheContext, "ifcont");

	Builder.CreateCondBr(CondV, ThenBB, ElseBB);

	// emit then value
	Builder.SetInsertPoint(ThenBB);

	Value * ThenV = Then->codegen();
	if(!ThenV)
		return nullptr;

	Builder.CreateBr(MergeBB);
	// codegen of then can change the cunrrehnt block,
	// update ThenBB for the PHI
	ThenBB = Builder.GetInsertBlock();


	// emit else block
	TheFunction->getBasicBlockList().push_back(ElseBB);
	Builder.SetInsertPoint(ElseBB);

	Value * ElseV = Else->codegen();
	if(!ElseV)
		return nullptr;

	Builder.CreateBr(MergeBB);
	// because codegen of 'Else' can change the current block, 
	// update ElseBB for the PHI
	ElseBB = Builder.GetInsertBlock();

	// Emit merge block
	TheFunction->getBasicBlockList().push_back(MergeBB);
	Builder.SetInsertPoint(MergeBB);
	PHINode * PN = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");

	PN->addIncoming(ThenV, ThenBB);
	PN->addIncoming(ElseV, ElseBB);

	// returned value will feed into the code for the top-level function,
	// which will create the return instruction.
	return PN;

}


Value * ForExprAST::codegen(){
	// emit the start code first, without 'variable' in scope
	Value * StartVal = Start->codegen();
	if(!StartVal)
		return nullptr;

	// make the new basic block for loop header, inserting after current block
	Function * TheFunction = Builder.GetInsertBlock()->getParent();
	BasicBlock * PreheaderBB = Builder.GetInsertBlock();
	BasicBlock * LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);

	// inset an explicit fall through from the current block to LoopBB
	Builder.CreateBr(LoopBB);

	// start insertion in LoopBB
	Builder.SetInsertPoint(LoopBB);

	// start the PHI node with an entry for start
	PHINode * Variable = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, 
								VarName.c_str());
	Variable->addIncoming(StartVal, PreheaderBB);

	// within the loop, the varibale is defined equal to the PHI node
	// if it shadows an existing varibale, we have to restore it,
	// so save it now
	// (redefine value name in for ,ex.  i = 5  for i = 1 ...)
	Value * OldVal = NamedValues[VarName];
	NamedValues[VarName] = Variable;

	// emit the body of the loop
	// can change the current BB
	// Note that we ignore the value computed by the body, but don't allow an error
	if(!Body->codegen())
		return nullptr;

	// Emit the step Value.
	Value * StepVal = nullptr;
	if(Step){
		StepVal = Step->codegen();
		if(!StepVal)
			return nullptr;
	}else{
		// if not specified, use 1.0
		StepVal = ConstantFP::get(TheContext, APFloat(1.0));
	}

	Value * NextVar = Builder.CreateFAdd(Variable, StepVal, "nextvar");


	// compute the end condition
	Value * EndCond = End->codegen();
	if(!EndCond)
		return nullptr;

	// covert condition to a bool by comparing non-eqyal to 0.0
	EndCond = Builder.CreateFCmpONE(EndCond, ConstantFP::get(TheContext, APFloat(0.0)), 
					"loopcond");

	// create the "after loop" blcok and insert it
	BasicBlock * LoopEndBB = Builder.GetInsertBlock();
	BasicBlock * AfterBB = BasicBlock::Create(TheContext, "afterloop", TheFunction);

	// inset the conditional branch into the end of LoopEndBB
	Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

	// any new code will be inserted in AfterBB
	Builder.SetInsertPoint(AfterBB);

	// add a new entry to the PHI node for the backedge
	Variable->addIncoming(NextVar, LoopEndBB);

	// restore the unshadowed variable
	if(OldVal)
		NamedValues[VarName] = OldVal;
	else
		NamedValues.erase(VarName);

	// for expr always return 0.0
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
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






































