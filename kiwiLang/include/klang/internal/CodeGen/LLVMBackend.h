include/klang/internal/CodeGen/LLVMBackend.h
#pragma once
#include "IR.h"
#include "Target.h"
#include <memory>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>

namespace klang {
namespace internal {
namespace codegen {

class LLVMCodeGenerator {
public:
    LLVMCodeGenerator(TargetInfo target);
    ~LLVMCodeGenerator();
    
    std::unique_ptr<llvm::Module> generateModule(const ModuleIR& module);
    bool compileToObjectFile(const std::string& filename);
    bool compileToAssembly(const std::string& filename);
    std::string compileToLLVMIR();
    
    void setOptimizationLevel(OptimizationLevel level);
    void setTargetCPU(std::string cpu);
    void setTargetFeatures(std::string features);
    
    const TargetInfo& targetInfo() const { return targetInfo_; }

private:
    TargetInfo targetInfo_;
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> llvmModule_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::TargetMachine> targetMachine_;
    OptimizationLevel optLevel_;
    
    std::unordered_map<Value, llvm::Value*> valueMap_;
    std::unordered_map<Type, llvm::Type*> typeMap_;
    std::unordered_map<FunctionIR*, llvm::Function*> functionMap_;
    
    llvm::Type* convertType(Type type);
    llvm::FunctionType* convertFunctionType(FunctionType type);
    llvm::Value* convertValue(Value value);
    llvm::Constant* convertConstant(Constant constant);
    
    llvm::Function* createFunction(FunctionIR& function);
    void generateFunctionBody(FunctionIR& function, llvm::Function* llvmFunc);
    void generateBasicBlock(BasicBlock& block, llvm::BasicBlock* llvmBlock);
    llvm::Value* generateInstruction(Instruction& inst);
    
    llvm::Value* generateBinaryOp(BinaryInstruction& inst);
    llvm::Value* generateUnaryOp(UnaryInstruction& inst);
    llvm::Value* generateCompare(CompareInstruction& inst);
    llvm::Value* generateCall(CallInstruction& inst);
    llvm::Value* generateLoad(LoadInstruction& inst);
    llvm::Value* generateStore(StoreInstruction& inst);
    llvm::Value* generateAlloca(AllocaInstruction& inst);
    llvm::Value* generateGEP(GEPInstruction& inst);
    llvm::Value* generateCast(CastInstruction& inst);
    llvm::Value* generatePhi(PhiInstruction& inst);
    llvm::Value* generateSelect(SelectInstruction& inst);
    llvm::Value* generateBranch(BranchInstruction& inst);
    llvm::Value* generateReturn(ReturnInstruction& inst);
    
    void setupTarget();
    void runOptimizations(llvm::Module& module);
    
    static llvm::LLVMContext& getGlobalContext();
};

class LLVMTargetLowering {
public:
    explicit LLVMTargetLowering(llvm::TargetMachine& targetMachine);
    
    llvm::SDValue lowerOperation(llvm::SDNode* node, llvm::SelectionDAG& dag);
    llvm::SDValue lowerCall(llvm::SDNode* node, llvm::SelectionDAG& dag);
    llvm::SDValue lowerMemoryOp(llvm::SDNode* node, llvm::SelectionDAG& dag);
    
    const llvm::TargetLowering& getTLI() const { return *tli_; }

private:
    llvm::TargetMachine& targetMachine_;
    const llvm::TargetLowering* tli_;
    
    llvm::SDValue lowerBinaryOp(llvm::SDNode* node, llvm::SelectionDAG& dag);
    llvm::SDValue lowerCompare(llvm::SDNode* node, llvm::SelectionDAG& dag);
    llvm::SDValue lowerCast(llvm::SDNode* node, llvm::SelectionDAG& dag);
};

class LLVMAssemblyEmitter {
public:
    explicit LLVMAssemblyEmitter(llvm::TargetMachine& targetMachine);
    
    bool emitAssembly(llvm::Module& module, llvm::raw_ostream& output);
    bool emitObject(llvm::Module& module, llvm::raw_ostream& output);
    
    void setOutputFormat(OutputFormat format);

private:
    llvm::TargetMachine& targetMachine_;
    OutputFormat outputFormat_;
    
    std::unique_ptr<llvm::legacy::PassManager> createPassManager();
    void addOptimizationPasses(llvm::legacy::PassManager& pm);
};

class LLVMMCJITCompiler {
public:
    LLVMMCJITCompiler();
    ~LLVMMCJITCompiler();
    
    void* compileFunction(FunctionIR& function);
    void* getFunctionPointer(std::string_view name);
    
    void addModule(std::unique_ptr<llvm::Module> module);
    void removeModule(llvm::Module* module);
    
    llvm::ExecutionEngine& getExecutionEngine() { return *executionEngine_; }

private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::ExecutionEngine> executionEngine_;
    
    static std::unique_ptr<llvm::Module> createModuleForJIT(const ModuleIR& module);
};

class LLVMDebugInfo {
public:
    LLVMDebugInfo(llvm::Module& module, std::string_view sourceFile);
    
    void emitFunctionDebugInfo(FunctionIR& function, llvm::Function* llvmFunc);
    void emitLocationDebugInfo(SourceLocation location);
    void emitVariableDebugInfo(Value variable, std::string_view name);
    
    void finalize();

private:
    llvm::Module& module_;
    llvm::DIBuilder dib_;
    llvm::DICompileUnit* compileUnit_;
    std::unordered_map<std::string, llvm::DIFile*> fileCache_;
    std::unordered_map<FunctionIR*, llvm::DISubprogram*> functionDebugInfo_;
    
    llvm::DIType* getDebugType(Type type);
    llvm::DIFile* getOrCreateFile(std::string_view filename);
    llvm::DILocation* getDebugLocation(SourceLocation location);
};

class LLVMBackend {
public:
    explicit LLVMBackend(TargetInfo target);
    
    BackendResult compileModule(const ModuleIR& module);
    BackendResult compileFunction(FunctionIR& function);
    
    void setOptimizationOptions(const OptimizationOptions& options);
    void setCodeGenOptions(const CodeGenOptions& options);
    
    const TargetInfo& target() const { return target_; }

private:
    TargetInfo target_;
    OptimizationOptions optOptions_;
    CodeGenOptions codeGenOptions_;
    std::unique_ptr<LLVMCodeGenerator> codeGen_;
    
    BackendResult compileToMemory(const ModuleIR& module);
    BackendResult compileToFile(const ModuleIR& module, std::string_view outputFile);
    
    void applyOptimizationPasses(llvm::Module& module);
    
    static llvm::CodeGenOpt::Level convertOptLevel(OptimizationLevel level);
};

class LLVMTargetInfo : public TargetInfo {
public:
    LLVMTargetInfo();
    
    std::vector<std::string> getSupportedFeatures() const override;
    bool isFeatureSupported(std::string_view feature) const override;
    
    std::unique_ptr<TargetLowering> createLowering() const override;
    std::unique_ptr<TargetCodeGen> createCodeGen() const override;
    
    static void initializeLLVMTargets();

private:
    static bool llvmInitialized_;
};

} // namespace codegen
} // namespace internal
} // namespace klang