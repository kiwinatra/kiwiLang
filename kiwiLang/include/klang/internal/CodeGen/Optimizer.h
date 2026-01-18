include/kiwiLang/internal/CodeGen/Optimizer.h
#pragma once
#include "IR.h"
#include "Analysis.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace kiwiLang {
namespace internal {
namespace codegen {

class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    
    virtual std::string name() const = 0;
    virtual bool run(FunctionIR& function, OptimizationContext& context) = 0;
    virtual bool isAnalysis() const { return false; }
};

class OptimizationPipeline {
public:
    OptimizationPipeline();
    
    void addPass(std::unique_ptr<OptimizationPass> pass);
    void addPass(std::string_view passName);
    
    bool run(FunctionIR& function, OptimizationContext& context);
    bool run(ModuleIR& module, OptimizationContext& context);
    
    const std::vector<std::unique_ptr<OptimizationPass>>& passes() const { return passes_; }
    
    static std::unique_ptr<OptimizationPipeline> createDefaultPipeline();
    static std::unique_ptr<OptimizationPipeline> createAggressivePipeline();
    static std::unique_ptr<OptimizationPipeline> createSizeOptimizedPipeline();

private:
    std::vector<std::unique_ptr<OptimizationPass>> passes_;
    std::unordered_map<std::string, std::unique_ptr<OptimizationPass>> passRegistry_;
    
    void initializeRegistry();
};

class ConstantFoldingPass final : public OptimizationPass {
public:
    std::string name() const override { return "constant-folding"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool foldConstants(BasicBlock& block, OptimizationContext& context);
    std::optional<Value> tryFoldBinaryOp(Instruction& inst);
    std::optional<Value> tryFoldUnaryOp(Instruction& inst);
    std::optional<Value> tryFoldCompare(Instruction& inst);
    std::optional<Value> tryFoldCast(Instruction& inst);
    std::optional<Value> tryFoldLoad(Instruction& inst);
    std::optional<Value> tryFoldGEP(Instruction& inst);
};

class DeadCodeEliminationPass final : public OptimizationPass {
public:
    std::string name() const override { return "dce"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool eliminateDeadCode(FunctionIR& function);
    bool isDeadInstruction(Instruction& inst);
    void markLiveInstructions(FunctionIR& function);
    
    std::unordered_set<Instruction*> liveInstructions_;
};

class CommonSubexpressionEliminationPass final : public OptimizationPass {
public:
    std::string name() const override { return "cse"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    struct ExpressionHash {
        std::size_t operator()(const Instruction& inst) const;
    };
    
    struct ExpressionEqual {
        bool operator()(const Instruction& a, const Instruction& b) const;
    };
    
    std::unordered_map<Instruction, Value, ExpressionHash, ExpressionEqual> expressionTable_;
    
    Value findCommonExpression(Instruction& inst);
    void invalidateMemoryDependentExpressions();
};

class LoopInvariantCodeMotionPass final : public OptimizationPass {
public:
    std::string name() const override { return "licm"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool hoistInvariants(Loop& loop, OptimizationContext& context);
    bool isLoopInvariant(Instruction& inst, Loop& loop);
    bool canHoist(Instruction& inst, Loop& loop);
    
    std::unordered_set<Instruction*> hoistedInstructions_;
};

class FunctionInliningPass final : public OptimizationPass {
public:
    std::string name() const override { return "inlining"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
    void setMaxSize(std::size_t size) { maxInlinedSize_ = size; }
    void setThreshold(std::size_t threshold) { inliningThreshold_ = threshold; }

private:
    std::size_t maxInlinedSize_ = 100;
    std::size_t inliningThreshold_ = 10;
    
    bool shouldInline(FunctionIR& caller, FunctionIR& callee, std::size_t callsiteCount);
    std::unique_ptr<FunctionIR> inlineCall(FunctionIR& caller, CallInstruction& call, FunctionIR& callee);
    void remapValues(FunctionIR& inlined, 
                    const std::unordered_map<Value, Value>& valueMap);
};

class StrengthReductionPass final : public OptimizationPass {
public:
    std::string name() const override { return "strength-reduction"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool reduceStrength(BasicBlock& block);
    Value reduceBinaryOp(Instruction& inst);
    Value reduceCompare(Instruction& inst);
    Value reduceMemoryOp(Instruction& inst);
    
    std::unordered_map<Value, std::pair<Value, int>> inductionVariables_;
};

class MemoryOptimizationPass final : public OptimizationPass {
public:
    std::string name() const override { return "memory-opt"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool optimizeMemoryOps(FunctionIR& function);
    bool eliminateRedundantStores(FunctionIR& function);
    bool promoteAllocaToRegister(FunctionIR& function);
    bool mergeAdjacentStores(FunctionIR& function);
    
    std::unordered_set<Instruction*> eliminatedStores_;
};

class TailCallOptimizationPass final : public OptimizationPass {
public:
    std::string name() const override { return "tail-call-opt"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool optimizeTailCalls(FunctionIR& function);
    bool isTailCall(CallInstruction& call, BasicBlock& block);
    void transformToTailCall(CallInstruction& call, BasicBlock& block);
};

class PeepholeOptimizationPass final : public OptimizationPass {
public:
    std::string name() const override { return "peephole"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool optimizeBlock(BasicBlock& block);
    bool optimizeInstruction(Instruction& inst);
    bool optimizeBinaryOp(Instruction& inst);
    bool optimizeCompare(Instruction& inst);
    bool optimizeBranch(Instruction& inst);
    
    std::unordered_set<Instruction*> modifiedInstructions_;
};

class VectorizationPass final : public OptimizationPass {
public:
    std::string name() const override { return "vectorize"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
    void setVectorWidth(std::size_t width) { vectorWidth_ = width; }

private:
    std::size_t vectorWidth_ = 4;
    
    bool vectorizeLoop(Loop& loop, OptimizationContext& context);
    bool isVectorizableLoop(Loop& loop);
    bool isVectorizableOperation(Instruction& inst);
    
    std::vector<Instruction*> collectVectorizableInstructions(Loop& loop);
    void createVectorInstructions(const std::vector<Instruction*>& scalarInsts, 
                                 Loop& loop);
};

class InstructionCombiningPass final : public OptimizationPass {
public:
    std::string name() const override { return "instcombine"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool combineInstructions(FunctionIR& function);
    bool visitInstruction(Instruction& inst);
    bool combineBinaryOp(Instruction& inst);
    bool combineCompare(Instruction& inst);
    bool combineCast(Instruction& inst);
    bool combineMemoryOp(Instruction& inst);
    
    std::unordered_set<Instruction*> combinedInstructions_;
};

class GlobalValueNumberingPass final : public OptimizationPass {
public:
    std::string name() const override { return "gvn"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    struct ValueNumber {
        std::size_t number;
        Value value;
    };
    
    std::unordered_map<std::size_t, Value> valueTable_;
    std::unordered_map<Value, std::size_t> valueNumbers_;
    
    std::size_t computeValueNumber(Value value);
    std::size_t hashValue(Value value);
    bool tryReplaceWithValueNumber(Instruction& inst);
};

class ControlFlowSimplificationPass final : public OptimizationPass {
public:
    std::string name() const override { return "simplify-cfg"; }
    bool run(FunctionIR& function, OptimizationContext& context) override;
    
private:
    bool simplifyControlFlow(FunctionIR& function);
    bool mergeBlocks(FunctionIR& function);
    bool eliminateEmptyBlocks(FunctionIR& function);
    bool eliminateUnreachableBlocks(FunctionIR& function);
    bool simplifyBranchChains(FunctionIR& function);
    
    std::unordered_set<BasicBlock*> processedBlocks_;
};

class OptimizationManager {
public:
    OptimizationManager();
    
    void registerPass(std::string_view name, std::unique_ptr<OptimizationPass> pass);
    OptimizationPass* getPass(std::string_view name);
    
    bool runPass(std::string_view name, FunctionIR& function, OptimizationContext& context);
    
    void addAnalysis(std::unique_ptr<AnalysisPass> analysis);
    AnalysisPass* getAnalysis(std::string_view name);
    
    const AnalysisResult* getAnalysisResult(std::string_view name) const;
    
    static OptimizationManager& instance();

private:
    std::unordered_map<std::string, std::unique_ptr<OptimizationPass>> passes_;
    std::unordered_map<std::string, std::unique_ptr<AnalysisPass>> analyses_;
    std::unordered_map<std::string, std::unique_ptr<AnalysisResult>> analysisResults_;
    
    void runRequiredAnalyses(FunctionIR& function, OptimizationContext& context);
};

} // namespace codegen
} // namespace internal
} // namespace kiwiLang