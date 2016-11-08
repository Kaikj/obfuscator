//===- DataTypeObfuscation.h - Data Type Obfuscation pass------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains includes and defines for the data type obfuscation pass
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_DATATYPEOBFUSCATION_H
#define LLVM_DATATYPEOBFUSCATION_H

// LLVM include
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/CryptoUtils.h"

// Namespace
using namespace llvm;
using namespace std;

namespace llvm {
    Pass *createDataTypeObfuscation ();
    Pass *createDataTypeObfuscation (bool flag);
}

#endif //LLVM_DATATYPEOBFUSCATION_H
