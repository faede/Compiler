//#include "KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
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
#include "llvm/Transforms/Utils.h"
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

// create an alloca instrcution in the entry block of the function
// this is used for mutable variables etc
static AllocaInst * CreateEntryBlockAlloca(Function * TheFunction,
										   const std::string &VarName){
	// creates an IRBuilder object that is pointing at the first instruction
	IRBuilder<> TmpB(&TheFunction->getEntryBlock(),	
						TheFunction->getEntryBlock().begin());

	// creates an alloca with the expected name and returns it
	return TmpB.CreateAlloca(Type::getDoubleTy(TheContext), nullptr, VarName.c_str());
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
	
	// load the value
	return Builder.CreateLoad(V, Name.c_str());
}

Value * UnaryExprAST::codegen(){
	Value * OperandV = Operand->codegen();
	if(!OperandV)
		return nullptr;

	Function * F = getFunction(std::string("unary") + Opcode);
	if(!F)
		return LogErrorV("Unknown unary operator");

	return Builder.CreateCall(F, OperandV, "unop");
 }


Value * BinaryExprAST::codegen(){
	// Special case '=' , beacuse we don't want to emit the LHS as an expression
	if(Op == '='){
		// Assignment requires the LHS to be an identifier
		// assume we're build LLVM with RTTI this can be changed to a 
		// dynamic_cast for automatic error checking
		VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
		if(!LHSE)
			return LogErrorV("destination of '=' must be a variable");

		// codegen the RHS
		Value * Val = RHS->codegen();
		if(!Val)
			return nullptr;

		// Look up the name
		Value * Variable = NamedValues[LHSE->getName()];
		if(!Variable)
			return LogErrorV("Unknown variable name");
		Builder.CreateStore(Val, Variable);
		return Val;
	}
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
			break;
	}

	// if it wasn't a builin binary operator, it must be a user defined one
	// emit a call to it
	Function * F = getFunction(std::string("binary") + Op);
	assert(F && "binary operator not found!");

	Value * Ops[] = {L, R};
	return Builder.CreateCall(F, Ops, "binop");
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



// Output for-loop as:
//   var = alloca double
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:
Value * ForExprAST::codegen(){
	Function * TheFunction = Builder.GetInsertBlock()->getParent();

	// create an alloca for the variable in the entry block
	AllocaInst * Alloca = CreateEntryBlockAlloca(TheFunction, VarName);

	// emit the start code first, without 'variable' in scope
	Value * StartVal = Start->codegen();
	if(!StartVal)
		return nullptr;

	// Store the value into the alloca
	Builder.CreateStore(StartVal, Alloca);

	// make the new basic block for loop header, inserting after current block
	BasicBlock * LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);

	// insert an explicit fall through from the current block to LoopBB
	Builder.CreateBr(LoopBB);

	// start insertion in LoopBB
	Builder.SetInsertPoint(LoopBB);

	// within the loop, the varibale is defined equal to the PHI node
	// if it shadows an existing varibale, we have to restore it,
	// so save it now
	// (redefine value name in for ,ex.  i = 5  for i = 1 ...)
	AllocaInst * OldVal = NamedValues[VarName];
	NamedValues[VarName] = Alloca;

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


	// compute the end condition
	Value * EndCond = End->codegen();
	if(!EndCond)
		return nullptr;

	// Reload, increment and restore
	Value * CurVal = Builder.CreateLoad(Alloca, Alloca, VarName.c_str());
	Value * NextVar = Builder.CreateFAdd(CurVal, StepVal, "nextvar");
	Builder.CreateStore(NextVar, Alloca);

	// covert condition to a bool by comparing non-eqyal to 0.0
	EndCond = Builder.CreateFCmpONE(EndCond, ConstantFP::get(TheContext, APFloat(0.0)), 
					"loopcond");

	// create the "after loop" blcok and insert it
	BasicBlock * AfterBB = BasicBlock::Create(TheContext, "afterloop", TheFunction);

	// inset the conditional branch into the end of LoopEndBB
	Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

	// any new code will be inserted in AfterBB
	Builder.SetInsertPoint(AfterBB);

	// restore the unshadowed variable
	if(OldVal)
		NamedValues[VarName] = OldVal;
	else
		NamedValues.erase(VarName);

	// for expr always return 0.0
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}



Value * VarExprAST::codegen(){
	std::vector<AllocaInst *> OldBindings;

	Function * TheFunction = Builder.GetInsertBlock()->getParent();

	// register all varibales and emit their initializer
	for(unsigned i = 0, e = VarNames.size(); i != e; ++i){
		const std::string & VarName = VarNames[i].first;
		ExprAST * Init = VarNames[i].second.get();

		// emit the initializer before adding the variable to scope, 
		// this prevents the initializer from referencing the varibale itself,
		// and permits stuff like this:
		// var a = 1 in
		// 	 var a = a in ... # refers to outer 'a'
		Value * InitVal;
		if(Init){
			InitVal = Init->codegen();
			if(!InitVal)
				return nullptr;
		}else{ 
			// if not specified, use 0.0
			InitVal = ConstantFP::get(TheContext, APFloat(0.0));
		}

		AllocaInst * Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
		Builder.CreateStore(InitVal, Alloca);

		// remeber the old varibale binding so that we can restore the binding
		// when we unrecurse
		OldBindings.push_back(NamedValues[VarName]);

		// remeber this binding
		NamedValues[VarName] = Alloca;
	}

	// codegen the body, now that all vars are in scope
	Value * BodyVal = Body->codegen();
	if(!BodyVal)
		return nullptr;

	// pop all our variables from scope
	for(unsigned i = 0, e = VarNames.size(); i != e; ++i){
		NamedValues[VarNames[i].first] = OldBindings[i];
	}

	// return the body computation
	return BodyVal;
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

	if(!TheFunction)
		return nullptr;

	// If this is an operator, install it
	if(P.isBinaryOp())
		BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

	// Create a new basic block to start
	BasicBlock * BB = BasicBlock::Create(TheContext, "entry", TheFunction);
	Builder.SetInsertPoint(BB);

	// Record arguments
	NamedValues.clear();
	for(auto & Arg : TheFunction->args()){
		// create an alloca for this variable
		AllocaInst * Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());

		// store the initial value into the alloca
		Builder.CreateStore(&Arg, Alloca);

		// Add arguments to variable symbol table
		NamedValues[Arg.getName()] = Alloca;
	}

	if(Value * RetVal = Body->codegen()){
		// finish
		Builder.CreateRet(RetVal);

		// This function does a variety of consistency checks on the generated code, 
		// to determine if our compiler is doing everything right
		// it can catch a lot of bugs
		verifyFunction(*TheFunction);

		return TheFunction;
	}

	// error body, remove function
	TheFunction->eraseFromParent();

	if (P.isBinaryOp())
    	BinopPrecedence.erase(P.getOperatorName());
  	return nullptr;
	// TODO:
	// an earlier ‘extern’ declaration will take precedence over the function 
	// definition’s signature, which can cause codegen to fail, for instance if the function
	// arguments are named differently
}
