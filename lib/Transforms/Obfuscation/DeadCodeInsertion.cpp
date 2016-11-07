#include "llvm/Transforms/Obfuscation/DeadCodeInsertion.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Intrinsics.h"
#include <iostream>

#define DEBUG_TYPE "dci"
#define NUMBER_DCI 1

namespace {
	struct DeadCodeInsertion : public FunctionPass {
		static char ID; // Pass identification, replacement for typeid	
		void (DeadCodeInsertion::*funcDeadCodeInsert[NUMBER_DCI])(BinaryOperator *bo);
		Instruction::BinaryOps opcodeList[2];		
		bool flag;		
		DeadCodeInsertion() : FunctionPass(ID) {}
		DeadCodeInsertion(bool flag) : FunctionPass(ID) {
			this->flag = flag;
			funcDeadCodeInsert[0] = &DeadCodeInsertion::addZero;
			opcodeList[0] = Instruction::Add;
			opcodeList[1] = Instruction::Sub;
		}
		bool runOnFunction(Function &F);
		void insertDeadCode(Function *F);
		void insertJumps(Function *f);
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
		//insertDeadCode(tmp);
		insertJumps(tmp);
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
				  case BinaryOperator::Sub:
				    // case BinaryOperator::FAdd:
				    // Substitute with random add operation
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
	Instruction::BinaryOps opcode = opcodeList[llvm::cryptoutils->get_range(2)];
	ConstantInt *c1 = (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
	int index = llvm::cryptoutils->get_range(2);
	op = BinaryOperator::Create(opcode, bo->getOperand(index), c1, "", bo);

	//to continue
	bool toContinue = llvm::cryptoutils->get_range(2);
	while (toContinue) {
		opcode = opcodeList[llvm::cryptoutils->get_range(2)];
		Value *operands[3];	
		operands[0] = bo->getOperand(0);
		operands[1] = bo->getOperand(1);
		operands[2] = op;
		Value *LHS_operand = operands[llvm::cryptoutils->get_range(3)];
		Value *RHS_operand = operands[llvm::cryptoutils->get_range(3)];
		op = BinaryOperator::Create(opcode, LHS_operand, RHS_operand, "", bo);
		toContinue = llvm::cryptoutils->get_range(2);
	
	}
	// Check signed wrap
	op->setHasNoSignedWrap(bo->hasNoSignedWrap());
	op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
	//bo->replaceAllUsesWith(op);
}



void DeadCodeInsertion::insertJumps(Function *f){
	std::vector<BasicBlock *> origBB;

	// Save all basic blocks
	for (Function::iterator I = f->begin(), IE = f->end(); I != IE; ++I) {
		origBB.push_back(I);
	}
	IntegerType *int_type = Type::getInt32Ty(llvm::getGlobalContext());
	
	for (std::vector<BasicBlock *>::iterator I = origBB.begin(), IE = origBB.end(); I != IE; ++I) {
		BasicBlock *bb = *I;
		//for each block
		
		// No need to split a 1 inst bb
		// Or ones containing a PHI node
		// Split
		BasicBlock *toSplit = bb;
		int toContinue;
		do {
			if (toSplit->size() < 2 ) {
				break;
			}
			BasicBlock::iterator inst = std::next(toSplit->begin(), llvm::cryptoutils->get_range(toSplit->size()/2));
			toSplit = toSplit->splitBasicBlock(inst, "original");
			//Definitely insert unused block at first try
			int toInsertUnusedBlock = 1;
			if (toInsertUnusedBlock) {
				BasicBlock *unusedBlock = BasicBlock::Create(bb->getContext(), "unused", f, toSplit);
				//75% chance of inserting additional unused block
				toInsertUnusedBlock = llvm::cryptoutils->get_range(4);
				while (toInsertUnusedBlock) {
					unusedBlock = unusedBlock->splitBasicBlock(unusedBlock->begin(), "unused");			
					toInsertUnusedBlock = llvm::cryptoutils->get_range(2);	
				}			
			}
			//80% chance to continue
			toContinue = llvm::cryptoutils->get_range(5);
		} while (toContinue);

	}
}


