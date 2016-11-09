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
		void insertRedundantInstIntoBlock(BasicBlock * bb);
		void insertZero(Function *F);
		void insertJumps(Function *f);
		void addZero(BinaryOperator *bo);
		void subZero(BinaryOperator *bo);
		BasicBlock* createAlteredBasicBlock(BasicBlock*, const Twine&, Function*);
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
		//insertZero(tmp);
		//insertJumps(tmp);
  	}
	return false;
}


void DeadCodeInsertion::insertDeadCode(Function *F) {
	Function *tmp = F;
	for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
		insertRedundantInstIntoBlock(bb);
	}	
}

void DeadCodeInsertion::insertRedundantInstIntoBlock(BasicBlock * bb) {
	//original def
	std::vector<Value*> originalDefinitions = std::vector<Value*>();
	//new value
	std:vector<ModifiedDefinition*> modifiedDefinitions = std::vector<ModifiedDefinition*>();
	int count = -1;
	for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
		count++;
		//if terminating instruction
		if (isa<TerminatorInst>(inst)) {
			std::vector<Value*>::iterator od = originalDefinitions.begin();
			std::vector<ModifiedDefinition*>::iterator md = modifiedDefinitions.begin();
			for(; od != originalDefinitions.end() && md!= modifiedDefinitions.end(); ++od, ++md) {
				//insert instruction to negate modified definition
				Value* latestValue = (*md)->modifiedValue;
				int increment = (*md)->increment;
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
				ConstantInt *c1 = (ConstantInt *)ConstantInt::get(int_type, final_increment);
				BinaryOperator *new_inst =  BinaryOperator::Create(opcode, latestValue, c1, "dummy", inst);
				//replace all uses
				(*od)->replaceAllUsesWith(new_inst);  	
			}
			//clean up
			od = originalDefinitions.begin();
			modifiedDefinitions = std::vector<ModifiedDefinition*>();
		//continue
		} else {
			//check uses
			for (Use &U : inst->operands()) {
  				Value *v = U.get();
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
					ConstantInt *c1 = (ConstantInt *)ConstantInt::get(int_type, final_increment);
					BinaryOperator *new_inst =  BinaryOperator::Create(opcode, latestValue, c1, "scam", inst);
					//replace all uses
					v->replaceAllUsesWith(new_inst);  
					//remove entry from original and modified definitions
					originalDefinitions.erase(originalDefinitions.begin() + pos);
					modifiedDefinitions.erase(modifiedDefinitions.begin() + pos);
					errs() << "Hello 3: "<< count << inst->getOpcodeName() << "\n";
				}
			}
			//choose one of the values in modifiedDefinitions randomly to modify again
			if (modifiedDefinitions.size() > 0) {
				ModifiedDefinition *md = modifiedDefinitions.at(0);
				Type *int_type = Type::getInt32Ty(bb->getContext());
				ConstantInt *c1 = (ConstantInt *)ConstantInt::get(int_type, 5);
				BinaryOperator *new_inst =  BinaryOperator::Create(BinaryOperator::Add, md->modifiedValue, c1, "scamscam", inst);				
				ModifiedDefinition *mdNew = new ModifiedDefinition();
				mdNew->modifiedValue = new_inst;
				mdNew->increment += 5;
				modifiedDefinitions.at(0) = mdNew;
				errs() << "Hello 2: "<< count << inst->getOpcodeName() << "\n";
				return;
			}
			//if it's a load or a binary operator
			//store Value into original definitions and modified Definitions so that we can modify next time
			if (isa<LoadInst>(inst) || isa<BinaryOperator>(inst)) {
				originalDefinitions.push_back(inst);
				ModifiedDefinition* md = new ModifiedDefinition();
				md->modifiedValue = inst;
				md->increment = 0;
				modifiedDefinitions.push_back(md);
				errs() << "Hello: "<< count << inst->getOpcodeName() << "\n";
				
			}
			
		}
	
	}
}

void DeadCodeInsertion::insertZero(Function *f) {
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
	for (std::vector<BasicBlock *>::iterator I = origBB.begin(), IE = origBB.end(); I != IE; ++I) {
		BasicBlock *bb = *I;
		//for each block
		// No need to split a 1 inst bb
		// Or ones containing a PHI node
		// Split
		BasicBlock *toSplit = bb;
		int toContinue;
		BasicBlock::iterator inst;
		std::vector<BasicBlock*> basicBlockList = vector<BasicBlock*>();
		std::vector<BasicBlock*> unusedBlockList = vector<BasicBlock*>();
		do {		
			int range;			
			if (toSplit->size() < 2 ) {
				break;
			} else if (toSplit->size() > 5) {
				range = toSplit->size()/2;
			} else {
				range = toSplit->size();
			}
			BasicBlock::iterator inst = std::next(toSplit->begin(), llvm::cryptoutils->get_range(range));
			while (isa<PHINode>(inst)) {
				++inst;
			}
			BasicBlock *newBlock = toSplit->splitBasicBlock(inst, "original");
			basicBlockList.push_back(toSplit);
			toSplit = newBlock;
			toContinue = llvm::cryptoutils->get_range(8);
		} while (toContinue);
/*
		basicBlockList.push_back(toSplit);
		for (std::vector<BasicBlock *>::iterator I = basicBlockList.begin(), IE = basicBlockList.end(); I != IE; ++I) {
			BasicBlock *bb = *I;			
			unusedBlockList.push_back(createAlteredBasicBlock(bb, "unused", f));
		}

		//TODO: update phi nodes
		for (std::vector<BasicBlock *>::iterator I = unusedBlockList.begin(), IE = unusedBlockList.end(); I != IE; ++I) {
			BasicBlock *bb = *I;
			TerminatorInst* tInst = bb->getTerminator();
			//if terminator instruction contains phi node, skip
			BasicBlock *successor;
			for (unsigned int id = 0; id < tInst->getNumSuccessors(); id++) {
				BasicBlock *originalSucc = tInst->getSuccessor(id);		
				unsigned int randomInt = llvm::cryptoutils->get_range(unusedBlockList.size() + basicBlockList.size());
				if (randomInt < basicBlockList.size()) {
					successor = basicBlockList[randomInt];
				} else {
					successor = unusedBlockList[randomInt-basicBlockList.size()];				
				}
				tInst->setSuccessor(id, successor);
			}
		} */
		
	}
}


//Koped from BogusControlFlow
BasicBlock* DeadCodeInsertion::createAlteredBasicBlock(BasicBlock* basicBlock, const Twine &  Name = "unused", Function * F = 0){
      // Useful to remap the informations concerning instructions.
      ValueToValueMapTy VMap;
      BasicBlock * alteredBB = llvm::CloneBasicBlock (basicBlock, VMap, "unused", F);
      DEBUG_WITH_TYPE("gen", errs() << "bcf: Original basic block cloned\n");
      // Remap operands.
      BasicBlock::iterator ji = basicBlock->begin();
      for (BasicBlock::iterator i = alteredBB->begin(), e = alteredBB->end() ; i != e; ++i){
        // Loop over the operands of the instruction
        for(User::op_iterator opi = i->op_begin (), ope = i->op_end(); opi != ope; ++opi){
          // get the value for the operand
          Value *v = MapValue(*opi, VMap,  RF_None, 0);
          if (v != 0){
            *opi = v;
            DEBUG_WITH_TYPE("gen", errs() << "bcf: Value's operand has been setted\n");
          }
        }
        DEBUG_WITH_TYPE("gen", errs() << "bcf: Operands remapped\n");
        // Remap phi nodes' incoming blocks.
        if (PHINode *pn = dyn_cast<PHINode>(i)) {
          for (unsigned j = 0, e = pn->getNumIncomingValues(); j != e; ++j) {
            Value *v = MapValue(pn->getIncomingBlock(j), VMap, RF_None, 0);
            if (v != 0){
              pn->setIncomingBlock(j, cast<BasicBlock>(v));
            }
          }
        }
        DEBUG_WITH_TYPE("gen", errs() << "bcf: PHINodes remapped\n");
        // Remap attached metadata.
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        i->getAllMetadata(MDs);
        DEBUG_WITH_TYPE("gen", errs() << "bcf: Metadatas remapped\n");
        // important for compiling with DWARF, using option -g.
        i->setDebugLoc(ji->getDebugLoc());
        ji++;
        DEBUG_WITH_TYPE("gen", errs() << "bcf: Debug information location setted\n");

      } // The instructions' informations are now all correct

      DEBUG_WITH_TYPE("gen", errs() << "bcf: The cloned basic block is now correct\n");
      DEBUG_WITH_TYPE("gen",
          errs() << "bcf: Starting to add junk code in the cloned bloc...\n");

      // add random instruction in the middle of the bloc. This part can be improve
      for (BasicBlock::iterator i = alteredBB->begin(), e = alteredBB->end() ; i != e; ++i){
        // in the case we find binary operator, we modify slightly this part by randomly
        // insert some instructions
        if(i->isBinaryOp()){ // binary instructions
          unsigned opcode = i->getOpcode();
          BinaryOperator *op, *op1 = NULL;
          Twine *var = new Twine("_");
          // treat differently float or int
          // Binary int
          if(opcode == Instruction::Add || opcode == Instruction::Sub ||
              opcode == Instruction::Mul || opcode == Instruction::UDiv ||
              opcode == Instruction::SDiv || opcode == Instruction::URem ||
              opcode == Instruction::SRem || opcode == Instruction::Shl ||
              opcode == Instruction::LShr || opcode == Instruction::AShr ||
              opcode == Instruction::And || opcode == Instruction::Or ||
              opcode == Instruction::Xor){
            for(int random = (int)llvm::cryptoutils->get_range(10); random < 10; ++random){
              switch(llvm::cryptoutils->get_range(4)){ // to improve
                case 0: //do nothing
                  break;
                case 1: op = BinaryOperator::CreateNeg(i->getOperand(0),*var,i);
                        op1 = BinaryOperator::Create(Instruction::Add,op,
                            i->getOperand(1),"gen",i);
                        break;
                case 2: op1 = BinaryOperator::Create(Instruction::Sub,
                            i->getOperand(0),
                            i->getOperand(1),*var,i);
                        op = BinaryOperator::Create(Instruction::Mul,op1,
                            i->getOperand(1),"gen",i);
                        break;
                case 3: op = BinaryOperator::Create(Instruction::Shl,
                            i->getOperand(0),
                            i->getOperand(1),*var,i);
                        break;
              }
            }
          }
          // Binary float
          if(opcode == Instruction::FAdd || opcode == Instruction::FSub ||
              opcode == Instruction::FMul || opcode == Instruction::FDiv ||
              opcode == Instruction::FRem){
            for(int random = (int)llvm::cryptoutils->get_range(10); random < 10; ++random){
              switch(llvm::cryptoutils->get_range(3)){ // can be improved
                case 0: //do nothing
                  break;
                case 1: op = BinaryOperator::CreateFNeg(i->getOperand(0),*var,i);
                        op1 = BinaryOperator::Create(Instruction::FAdd,op,
                            i->getOperand(1),"gen",i);
                        break;
                case 2: op = BinaryOperator::Create(Instruction::FSub,
                            i->getOperand(0),
                            i->getOperand(1),*var,i);
                        op1 = BinaryOperator::Create(Instruction::FMul,op,
                            i->getOperand(1),"gen",i);
                        break;
              }
            }
          }
          if(opcode == Instruction::ICmp){ // Condition (with int)
            ICmpInst *currentI = (ICmpInst*)(&i);
            switch(llvm::cryptoutils->get_range(3)){ // must be improved
              case 0: //do nothing
                break;
              case 1: currentI->swapOperands();
                      break;
              case 2: // randomly change the predicate
                      switch(llvm::cryptoutils->get_range(10)){
                        case 0: currentI->setPredicate(ICmpInst::ICMP_EQ);
                                break; // equal
                        case 1: currentI->setPredicate(ICmpInst::ICMP_NE);
                                break; // not equal
                        case 2: currentI->setPredicate(ICmpInst::ICMP_UGT);
                                break; // unsigned greater than
                        case 3: currentI->setPredicate(ICmpInst::ICMP_UGE);
                                break; // unsigned greater or equal
                        case 4: currentI->setPredicate(ICmpInst::ICMP_ULT);
                                break; // unsigned less than
                        case 5: currentI->setPredicate(ICmpInst::ICMP_ULE);
                                break; // unsigned less or equal
                        case 6: currentI->setPredicate(ICmpInst::ICMP_SGT);
                                break; // signed greater than
                        case 7: currentI->setPredicate(ICmpInst::ICMP_SGE);
                                break; // signed greater or equal
                        case 8: currentI->setPredicate(ICmpInst::ICMP_SLT);
                                break; // signed less than
                        case 9: currentI->setPredicate(ICmpInst::ICMP_SLE);
                                break; // signed less or equal
                      }
                      break;
            }

          }
          if(opcode == Instruction::FCmp){ // Conditions (with float)
            FCmpInst *currentI = (FCmpInst*)(&i);
            switch(llvm::cryptoutils->get_range(3)){ // must be improved
              case 0: //do nothing
                break;
              case 1: currentI->swapOperands();
                      break;
              case 2: // randomly change the predicate
                      switch(llvm::cryptoutils->get_range(10)){
                        case 0: currentI->setPredicate(FCmpInst::FCMP_OEQ);
                                break; // ordered and equal
                        case 1: currentI->setPredicate(FCmpInst::FCMP_ONE);
                                break; // ordered and operands are unequal
                        case 2: currentI->setPredicate(FCmpInst::FCMP_UGT);
                                break; // unordered or greater than
                        case 3: currentI->setPredicate(FCmpInst::FCMP_UGE);
                                break; // unordered, or greater than, or equal
                        case 4: currentI->setPredicate(FCmpInst::FCMP_ULT);
                                break; // unordered or less than
                        case 5: currentI->setPredicate(FCmpInst::FCMP_ULE);
                                break; // unordered, or less than, or equal
                        case 6: currentI->setPredicate(FCmpInst::FCMP_OGT);
                                break; // ordered and greater than
                        case 7: currentI->setPredicate(FCmpInst::FCMP_OGE);
                                break; // ordered and greater than or equal
                        case 8: currentI->setPredicate(FCmpInst::FCMP_OLT);
                                break; // ordered and less than
                        case 9: currentI->setPredicate(FCmpInst::FCMP_OLE);
                                break; // ordered or less than, or equal
                      }
                      break;
            }
          }
        }
      }
      return alteredBB;
    } // end of createAlteredBasicBlock()



