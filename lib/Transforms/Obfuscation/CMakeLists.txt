add_llvm_library(LLVMObfuscation
  CryptoUtils.cpp
  Substitution.cpp
  BogusControlFlow.cpp
  Utils.cpp
  SplitBasicBlocks.cpp
  Flattening.cpp
  DeadCodeInsertion.cpp
  DataFlowTransformation.cpp
  DataTypeObfuscation.cpp
  )

add_dependencies(LLVMObfuscation intrinsics_gen)
