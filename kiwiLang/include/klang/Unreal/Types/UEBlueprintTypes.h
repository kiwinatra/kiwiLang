#pragma once

#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace kiwiLang {
namespace Unreal {

enum class UEBlueprintNodeType {
    Event,
    Function,
    VariableGet,
    VariableSet,
    FlowControl,
    Branch,
    Sequence,
    ForLoop,
    WhileLoop,
    Break,
    Continue,
    MathExpression,
    Comparison,
    LogicOperation,
    TypeCast,
    Literal,
    MakeArray,
    MakeMap,
    MakeSet,
    GetArrayElement,
    SetArrayElement,
    MapGet,
    MapSet,
    SetContains,
    StructOperation,
    ClassOperation,
    EnumOperation,
    DelegateBinding,
    CustomEvent,
    Macro,
    Comment,
    RerouteNode,
    Unknown
};

enum class UEBlueprintPinDirection {
    Input,
    Output,
    Bidirectional
};

enum class UEBlueprintPinType {
    Exec,
    Boolean,
    Byte,
    Integer,
    Integer64,
    Float,
    Double,
    String,
    Text,
    Name,
    Vector,
    Rotator,
    Transform,
    Object,
    Class,
    Interface,
    Struct,
    Enum,
    Array,
    Map,
    Set,
    Delegate,
    Wildcard
};

struct UEBlueprintPinInfo {
    std::string pinName;
    std::string displayName;
    UEBlueprintPinType pinType;
    UEBlueprintPinDirection direction;
    std::string typeName;
    std::string defaultValue;
    bool isArray;
    bool isReference;
    bool isConst;
    bool isRequired;
    bool isAdvancedView;
    
    bool isValid() const;
    std::string getCPPTypeName() const;
};

struct UEBlueprintNodeInfo {
    std::string nodeId;
    std::string title;
    UEBlueprintNodeType nodeType;
    std::vector<UEBlueprintPinInfo> inputPins;
    std::vector<UEBlueprintPinInfo> outputPins;
    std::unordered_map<std::string, std::string> nodeData;
    std::uint32_t posX;
    std::uint32_t posY;
    std::uint32_t width;
    std::uint32_t height;
    bool isCollapsed;
    bool isEnabled;
    bool isPure;
    
    bool hasInputPin(const std::string& pinName) const;
    bool hasOutputPin(const std::string& pinName) const;
    const UEBlueprintPinInfo* findPin(const std::string& pinName) const;
};

struct UEBlueprintConnection {
    std::string connectionId;
    std::string fromNodeId;
    std::string fromPinName;
    std::string toNodeId;
    std::string toPinName;
    bool isExecutionFlow;
    
    bool isValid() const;
};

struct UEBlueprintVariable {
    std::string variableName;
    UEBlueprintPinType variableType;
    std::string typeName;
    std::string defaultValue;
    std::uint32_t flags;
    std::string category;
    bool isEditable;
    bool isBlueprintReadOnly;
    bool isBlueprintReadWrite;
    bool isExposedOnSpawn;
    bool isPrivate;
    
    bool isBlueprintVisible() const;
    std::string getCPPDeclaration() const;
};

struct UEBlueprintGraph {
    std::string graphName;
    std::string parentClassName;
    std::vector<UEBlueprintNodeInfo> nodes;
    std::vector<UEBlueprintConnection> connections;
    std::vector<UEBlueprintVariable> variables;
    std::unordered_map<std::string, std::string> graphProperties;
    std::uint32_t version;
    
    bool validate() const;
    bool hasCycles() const;
    std::vector<UEBlueprintNodeInfo> getRootNodes() const;
    std::vector<UEBlueprintNodeInfo> getExecutionOrder() const;
};

struct UEBlueprintClassInfo {
    std::string className;
    std::string parentClassName;
    std::vector<std::string> implementedInterfaces;
    std::vector<UEBlueprintGraph> graphs;
    std::vector<UEBlueprintVariable> memberVariables;
    std::vector<UEFunctionInfo> functions;
    std::unordered_map<std::string, std::string> classMetadata;
    bool isBlueprintGeneratedClass;
    bool isAbstract;
    bool isDeprecated;
    
    bool isValid() const;
    bool hasGraph(const std::string& graphName) const;
    const UEBlueprintGraph* findGraph(const std::string& graphName) const;
};

class KIWI_UNREAL_API UEBlueprintTypeConverter {
public:
    explicit UEBlueprintTypeConverter(UETypeConverter& baseConverter);
    
    UEBlueprintPinType convertToPinType(AST::TypePtr kiwiType) const;
    AST::TypePtr convertFromPinType(UEBlueprintPinType pinType, const std::string& typeName = "") const;
    
    std::string generatePinDefaultValue(UEBlueprintPinType pinType, const std::string& defaultValue) const;
    bool validatePinConnection(UEBlueprintPinType fromType, UEBlueprintPinType toType) const;
    
    UEBlueprintNodeInfo convertFunctionToNode(const UEFunctionInfo& functionInfo) const;
    UEFunctionInfo convertNodeToFunction(const UEBlueprintNodeInfo& nodeInfo) const;
    
    UEBlueprintVariable convertPropertyToVariable(const UEPropertyInfo& propertyInfo) const;
    UEPropertyInfo convertVariableToProperty(const UEBlueprintVariable& variable) const;
    
    bool generateBlueprintGraphCode(const UEBlueprintGraph& graph, const std::string& outputPath) const;
    bool generateBlueprintClassCode(const UEBlueprintClassInfo& classInfo, const std::string& outputPath) const;
    
    static std::string pinTypeToString(UEBlueprintPinType pinType);
    static UEBlueprintPinType stringToPinType(const std::string& typeString);
    
private:
    UETypeConverter& baseConverter_;
    
    std::unordered_map<UEBlueprintPinType, std::string> pinTypeNames_;
    std::unordered_map<std::string, UEBlueprintPinType> stringToPinType_;
    
    void initializePinTypeMappings();
    
    bool validateNodeConnections(const UEBlueprintNodeInfo& node,
                                const std::vector<UEBlueprintConnection>& connections) const;
    bool detectGraphCycles(const UEBlueprintGraph& graph,
                          const std::string& startNodeId,
                          std::unordered_set<std::string>& visited,
                          std::unordered_set<std::string>& recursionStack) const;
    
    std::string generateNodeCode(const UEBlueprintNodeInfo& node) const;
    std::string generateConnectionCode(const UEBlueprintConnection& connection) const;
    std::string generateVariableCode(const UEBlueprintVariable& variable) const;
    
    std::string generateEventNodeCode(const UEBlueprintNodeInfo& node) const;
    std::string generateFunctionNodeCode(const UEBlueprintNodeInfo& node) const;
    std::string generateFlowControlNodeCode(const UEBlueprintNodeInfo& node) const;
    std::string generateMathNodeCode(const UEBlueprintNodeInfo& node) const;
    std::string generateVariableNodeCode(const UEBlueprintNodeInfo& node) const;
    
    bool isPinTypeCompatible(UEBlueprintPinType type1, UEBlueprintPinType type2) const;
    bool canPromotePinType(UEBlueprintPinType fromType, UEBlueprintPinType toType) const;
    
    std::string getPinCPPType(UEBlueprintPinType pinType, const std::string& typeName) const;
    std::string getPinDefaultValueCPP(UEBlueprintPinType pinType, const std::string& defaultValue) const;
};

} // namespace Unreal
} // namespace kiwiLang