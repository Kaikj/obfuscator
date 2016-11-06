#include "llvm/Transforms/Obfuscation/DeadCodeInsertion.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Intrinsics.h"

#define DEBUG_TYPE "dci"
#define NUMBER_DCI 2

namespace {
	struct DeadCodeInsertion : public FunctionPass {
		static char ID; // Pass identification, replacement for typeid	
		void (DeadCodeInsertion::*funcDeadCodeInsert[NUMBER_DCI])(BinaryOperator *bo);
		bool flag;		
		DeadCodeInsertion() : FunctionPass(ID) {}
		DeadCodeInsertion(bool flag) : FunctionPass(ID) {

			this->flag = flag;
			funcDeadCodeInsert[0] = &DeadCodeInsertion::addZero;
			funcDeadCodeInsert[1] = &DeadCodeInsertion::subZero;	
		}
		bool runOnFunction(Function &F);
		void insertDeadCode(Function *F);
		void addZero(BinaryOperator *bo);
		void subZero(BinaryOperator *bo);
	};
}

char DeadCodeInsertion::ID = 0;
static RegisterPass<DeadCodeInsertion> X("dci", "Dead Code Insertion Pass");

Pass *llvm::createDeadCodeInsertion(bool flag) {
  return new DeadCodeInsertion(flag);
}

bool DeadCodeInsertion::runOnFunction(Function &F) {
	Function *tmp = &F;
	if (toObfuscate(flag, tmp, "dci")) {
		insertDeadCode(tmp);
  	}
	return false;
}

void DeadCodeInsertion::insertDeadCode(Function *f) {
	Function *tmp = f;
	for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
	      for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
			if (inst->isBinaryOp()) {
				  switch (inst->getOpcode()) {
				  case BinaryOperator::Add:
				    // case BinaryOperator::FAdd:
				    // Substitute with random add operation
				    (this->*funcDeadCodeInsert[llvm::cryptoutils->get_range
					(NUMBER_DCI)])(cast<BinaryOperator>(inst));
				    break;
				  case BinaryOperator::Sub:
				    // case BinaryOperator::FSub:
				    // Substitute with random sub operation
				    (this->*funcDeadCodeInsert[llvm::cryptoutils->get_range
					(NUMBER_DCI)])(cast<BinaryOperator>(inst));
				    break;
				  default:
				    break;
				  }              // End switch
			}                // End isBinaryOp
		}                  // End for basickblock
	}                    // End for Function
}

void DeadCodeInsertion::addZero(BinaryOperator *bo){
	BinaryOperator *op = NULL;
	Type *ty = bo->getType();
	ConstantInt *c1 = (ConstantInt *)ConstantInt::get(ty, 0);
	op = BinaryOperator::Create(Instruction::Add, bo->getOperand(0), c1, "", bo);
	op = BinaryOperator::Create(bo->getOpcode(), op, bo->getOperand(1), "", bo);
	// Check signed wrap
	op->setHasNoSignedWrap(bo->hasNoSignedWrap());
	op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
	bo->replaceAllUsesWith(op);
}

void DeadCodeInsertion::subZero(BinaryOperator *bo){
	BinaryOperator *op = NULL;
	Type *ty = bo->getType();
	ConstantInt *c1 = (ConstantInt *)ConstantInt::get(ty, 0);
	op = BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), c1, "", bo);
	op = BinaryOperator::Create(bo->getOpcode(), op, bo->getOperand(1), "", bo);
	// Check signed wrap
	op->setHasNoSignedWrap(bo->hasNoSignedWrap());
	op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
	bo->replaceAllUsesWith(op);
}


