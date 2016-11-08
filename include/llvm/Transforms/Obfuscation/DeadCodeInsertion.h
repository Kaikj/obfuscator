//===- SubstitutionIncludes.h - Substitution Obfuscation pass-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains includes and defines for the substitution pass
//
//===----------------------------------------------------------------------===//

#ifndef _DCI_H_
#define _DCI_H_


// LLVM include
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/CryptoUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// Namespace
using namespace llvm;
using namespace std;

namespace llvm {
	Pass *createDeadCodeInsertion ();
	Pass *createDeadCodeInsertion (bool flag);
}

#endif

