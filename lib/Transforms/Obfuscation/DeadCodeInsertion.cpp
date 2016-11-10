#include "llvm/Transforms/Obfuscation/DeadCodeInsertion.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"
#include <iostream>

#define DEBUG_TYPE "dci"
#define NUMBER_DCI 1


namespace {

	struct ModifiedDefinition {
		Value* modifiedValue;
		int increment;	
	};
	
	struct DeadCodeInsertion : public FunctionPass {
		static char ID; // Pass identification, replacement for typeid	
		void (DeadCodeInsertion::*funcDeadCodeInsert[NUMBER_DCI])(BinaryOperator *bo);
		Instruction::BinaryOps opcodeList[5];
		Instruction::BinaryOps fopcodeList[5];
		bool flag;		
		DeadCodeInsertion() : FunctionPass(ID) {}
		DeadCodeInsertion(bool flag) : FunctionPass(ID) {
			this->flag = flag;
			funcDeadCodeInsert[0] = &DeadCodeInsertion::addUnusedInstruction;
			opcodeList[0] = Instruction::Add;
			opcodeList[1] = Instruction::Sub;
			opcodeList[2] = Instruction::Mul;
			opcodeList[3] = Instruction::SDiv;
			opcodeList[4] = Instruction::SRem;	
			fopcodeList[0] = Instruction::FAdd;
			fopcodeList[1] = Instruction::FSub;
			fopcodeList[2] = Instruction::FMul;
			fopcodeList[3] = Instruction::FDiv;
			fopcodeList[4] = Instruction::FRem;
		}
		bool runOnFunction(Function &F);
		void insertUsedDeadCode(Function *F);
		void insertUsedDeadCodeIntoBlock(BasicBlock * bb);
		void insertUnusedDeadCode(Function *F);
		void addUnusedInstruction(BinaryOperator *bo);
		BasicBlock* createAlteredBasicBlock(BasicBlock*, const Twine&, Function*);
		Constant* getConstant(Type *ty);
		Instruction::BinaryOps getRandomOpcode(Type *ty);
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
		insertUsedDeadCode(tmp);
		insertUnusedDeadCode(tmp);
  	}
	return false;
}


void DeadCodeInsertion::insertUsedDeadCode(Function *F) {
	Function *tmp = F;
	for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
		insertUsedDeadCodeIntoBlock(bb);		
	}	
}

void DeadCodeInsertion::insertUsedDeadCodeIntoBlock(BasicBlock * bb) {
	//original def
	vector<Value*> originalDefinitions = std::vector<Value*>();
	//new value
	vector<ModifiedDefinition*> modifiedDefinitions = std::vector<ModifiedDefinition*>();
	for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
		//check uses
		//choose one of the values in modifiedDefinitions randomly to modify again
		//do not always randomise for each instruction
		//1/5 chance of not inserting in dead code
		int prob_insert_dead_code = llvm::cryptoutils->get_range(5);
		if (!isa<TerminatorInst>(inst) && modifiedDefinitions.size() > 0 && prob_insert_dead_code) {
			unsigned int random = llvm::cryptoutils->get_range(modifiedDefinitions.size());
			ModifiedDefinition *md = modifiedDefinitions.at(random);
			Type *int_type = Type::getInt32Ty(bb->getContext());
			//quantity to add
			unsigned int random_increment = llvm::cryptoutils->get_range(100);
			ConstantInt *c1 = (ConstantInt *)ConstantInt::get(int_type, random_increment);
			Instruction::BinaryOps opcode = opcodeList[llvm::cryptoutils->get_range(2)];
			BinaryOperator *new_inst =  BinaryOperator::Create(opcode, md->modifiedValue, c1, "scam", inst);				
			ModifiedDefinition *mdNew = new ModifiedDefinition();
			mdNew->modifiedValue = new_inst;
			if (opcode == Instruction::Add) {
				mdNew->increment = md->increment + random_increment;
			} else {
				mdNew->increment = md->increment - random_increment;
			}
			modifiedDefinitions.at(random) = mdNew;
		}
		for (unsigned int i = 0; i < inst->getNumOperands(); i++) {
			Value *v = inst->getOperand(i);
			//if present in original Defn
			unsigned int pos = find(originalDefinitions.begin(), originalDefinitions.end(), v) - originalDefinitions.begin();
			if (originalDefinitions.size() != 0 && pos < originalDefinitions.size()) {
				//check corresponding modified definition, 
				ModifiedDefinition* modifiedDef = modifiedDefinitions.at(pos);
				//add instruction that negates modified definition before the inst
				Value* latestValue = modifiedDef->modifiedValue;
				int increment = modifiedDef->increment;
				Instruction::BinaryOps opcode;
				if (increment == 0) {
					continue;				
				} else if (increment < 0) {
					//do addition to get it back
					opcode = BinaryOperator::Add;		
				} else {
					//do subtraction to get it back 
					opcode = BinaryOperator::Sub;				
				}
				unsigned int final_increment = abs(increment);
				Type *int_type = Type::getInt32Ty(bb->getContext());
				//break correction into 2 steps
				unsigned int step = llvm::cryptoutils->get_range(final_increment);
				ConstantInt *c1 = (ConstantInt *)ConstantInt::get(int_type, step);
				ConstantInt *c2 = (ConstantInt *)ConstantInt::get(int_type, final_increment - step);
				BinaryOperator *new_inst =  BinaryOperator::Create(opcode, latestValue, c1, "correct-", inst);
				BinaryOperator *new_inst_2 =  BinaryOperator::Create(opcode, new_inst, c2, "correct2-", inst);
				//replace uses
				inst->setOperand(i, new_inst_2);
				//reset modified definitions
				modifiedDef->modifiedValue = new_inst_2;
				modifiedDef->increment = 0;
									
				//replace all uses
				//v->replaceAllUsesWith(new_inst);  
				//remove entry from original and modified definitions
				//originalDefinitions.erase(originalDefinitions.begin() + pos);
				//modifiedDefinitions.erase(modifiedDefinitions.begin() + pos);
			}
		}
		//if it's a load or a binary operator
		//store Value into original definitions and modified Definitions so that we can modify next time
		if ((isa<LoadInst>(inst) || isa<BinaryOperator>(inst)) && inst->getType()->isIntegerTy()) {
			originalDefinitions.push_back(inst);
			ModifiedDefinition* md = new ModifiedDefinition();
			md->modifiedValue = inst;
			md->increment = 0;
			modifiedDefinitions.push_back(md);		
		}
	
	}
}

void DeadCodeInsertion::insertUnusedDeadCode(Function *f) {
	Function *tmp = f;
	for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
	      for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
			if (inst->isBinaryOp()) {
				switch (inst->getOpcode()) {
				case BinaryOperator::Add:
				case BinaryOperator::Sub:
				case BinaryOperator::Mul:
				case BinaryOperator::SDiv:
				case BinaryOperator::SRem:	
				case BinaryOperator::FAdd:
				case BinaryOperator::FSub:
				case BinaryOperator::FMul:
				case BinaryOperator::FDiv:
				case BinaryOperator::FRem:
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

void DeadCodeInsertion::addUnusedInstruction(BinaryOperator *bo){
	BinaryOperator *op = NULL;
	Type *ty = bo->getType();
	Instruction::BinaryOps opcode = getRandomOpcode(ty);
	Constant *c1 = getConstant(ty);
	int index = llvm::cryptoutils->get_range(2);
	op = BinaryOperator::Create(opcode, bo->getOperand(index), c1, "unusedOne", bo);

	//to continue
	bool toContinue = llvm::cryptoutils->get_range(2);
	while (toContinue) {
		opcode = getRandomOpcode(ty);
		Value *operands[3];	
		operands[0] = bo->getOperand(0);
		operands[1] = bo->getOperand(1);
		operands[2] = op;
		Value *LHS_operand = operands[llvm::cryptoutils->get_range(3)];
		Value *RHS_operand = operands[llvm::cryptoutils->get_range(3)];
		op = BinaryOperator::Create(opcode, LHS_operand, RHS_operand, "unusedTwo", bo);
		toContinue = llvm::cryptoutils->get_range(2);
	
	}
	// Check signed wrap
	op->setHasNoSignedWrap(bo->hasNoSignedWrap());
	op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
	//bo->replaceAllUsesWith(op);
}

Instruction::BinaryOps DeadCodeInsertion::getRandomOpcode(Type *ty) {
	if (ty->isFloatingPointTy()) {
		return fopcodeList[llvm::cryptoutils->get_range(5)];
	} else {
		return opcodeList[llvm::cryptoutils->get_range(5)];	
	}
}

Constant* DeadCodeInsertion::getConstant(Type *ty){
	if (ty->isFloatingPointTy()) {
		return (ConstantFP *)ConstantFP::get(ty, (double) llvm::cryptoutils->get_uint64_t());
	} else {
		return (ConstantInt*) ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
	}
}

