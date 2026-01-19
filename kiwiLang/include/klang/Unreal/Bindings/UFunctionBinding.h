#pragma once

#include "kiwiLang/AST/Decl.h"
#include "kiwiLang/AST/Expr.h"
#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <type_traits>

namespace kiwiLang {
namespace Unreal {

class UObjectRegistry;
class UETypeConverter;
struct UEBindingContext;

enum class UEFunctionCallType {
    Static,
    Instance,
    Delegate,
    Multicast,
    RPC_Server,
    RPC_Client,
    RPC_NetMulticast
};

struct UEFunctionBindingParams {
    std::string functionName;
    AST::FunctionDecl* kiwiFunction;
    UEFunctionInfo ueFunctionInfo;
    UEFunctionCallType callType;
    bool isConst;
    bool isVirtual;
    bool isBlueprintNativeEvent;
    std::uint32_t replicationFlags;
    std::vector<std::string> paramNames;
    std::vector<AST::TypePtr> paramTypes;
    
    bool validate() const;
    std::string generateSignature(const UETypeConverter& converter) const;
};

class KIWI_UNREAL_API UFunctionBindingBase {
public:
    explicit UFunctionBindingBase(const UEFunctionBindingParams& params);
    virtual ~UFunctionBindingBase();
    
    const UEFunctionBindingParams& getParams() const { return params_; }
    
    virtual void* invoke(void* object, void** args) const = 0;
    virtual bool bindToNative(void* nativeFunction) = 0;
    virtual bool bindToBlueprint(void* blueprintFunction) = 0;
    
    virtual std::string generateWrapperCode() const = 0;
    virtual std::string generateThunkCode() const = 0;
    virtual std::string generateDispatcherCode() const = 0;
    
    bool isBound() const { return isBound_; }
    bool isValid() const { return isValid_; }
    
protected:
    UEFunctionBindingParams params_;
    bool isBound_;
    bool isValid_;
    
    virtual bool validateParameters() = 0;
    virtual bool generateSignatureHash() = 0;
};

template<typename ReturnType, typename... Args>
class UFunctionBinding : public UFunctionBindingBase {
public:
    using FunctionPtr = ReturnType(*)(void*, Args...);
    using MemberFunctionPtr = ReturnType(*)(void*, void*, Args...);
    using NativeFunctionPtr = std::function<ReturnType(Args...)>;
    
    explicit UFunctionBinding(const UEFunctionBindingParams& params,
                             FunctionPtr functionPtr = nullptr)
        : UFunctionBindingBase(params)
        , functionPtr_(functionPtr)
        , memberFunctionPtr_(nullptr)
        , nativeFunction_() {
        initialize();
    }
    
    explicit UFunctionBinding(const UEFunctionBindingParams& params,
                             MemberFunctionPtr memberPtr,
                             void* objectInstance)
        : UFunctionBindingBase(params)
        , functionPtr_(nullptr)
        , memberFunctionPtr_(memberPtr)
        , objectInstance_(objectInstance)
        , nativeFunction_() {
        initialize();
    }
    
    explicit UFunctionBinding(const UEFunctionBindingParams& params,
                             NativeFunctionPtr nativeFunc)
        : UFunctionBindingBase(params)
        , functionPtr_(nullptr)
        , memberFunctionPtr_(nullptr)
        , nativeFunction_(std::move(nativeFunc)) {
        initialize();
    }
    
    void* invoke(void* object, void** args) const override {
        if constexpr (std::is_same_v<ReturnType, void>) {
            invokeImpl(object, args);
            return nullptr;
        } else {
            return invokeImpl(object, args);
        }
    }
    
    bool bindToNative(void* nativeFunction) override {
        if (!nativeFunction) return false;
        
        auto* uFunction = static_cast<UFunction*>(nativeFunction);
        return bindToUFunction(uFunction);
    }
    
    bool bindToBlueprint(void* blueprintFunction) override {
        if (!blueprintFunction) return false;
        
        return bindToBlueprintGraph(blueprintFunction);
    }
    
    std::string generateWrapperCode() const override {
        return generateCPPWrapper();
    }
    
    std::string generateThunkCode() const override {
        return generateNativeThunk();
    }
    
    std::string generateDispatcherCode() const override {
        return generateBlueprintDispatcher();
    }
    
private:
    FunctionPtr functionPtr_;
    MemberFunctionPtr memberFunctionPtr_;
    void* objectInstance_;
    NativeFunctionPtr nativeFunction_;
    std::size_t signatureHash_;
    
    void initialize() {
        isValid_ = validateParameters() && generateSignatureHash();
        isBound_ = functionPtr_ != nullptr || 
                   memberFunctionPtr_ != nullptr || 
                   !nativeFunction_.target_type().operator bool();
    }
    
    bool validateParameters() override {
        return sizeof...(Args) == params_.paramTypes.size();
    }
    
    bool generateSignatureHash() override {
        std::hash<std::string> hasher;
        signatureHash_ = hasher(params_.functionName);
        return true;
    }
    
    template<std::size_t... Is>
    auto unpackArgs(void** args, std::index_sequence<Is...>) const {
        return std::make_tuple(static_cast<Args>(args[Is])...);
    }
    
    void* invokeImpl(void* object, void** args) const {
        if (functionPtr_) {
            if constexpr (std::is_same_v<ReturnType, void>) {
                functionPtr_(object, std::forward<Args>(static_cast<Args>(args[Is]))...);
                return nullptr;
            } else {
                return static_cast<void*>(new ReturnType(
                    functionPtr_(object, std::forward<Args>(static_cast<Args>(args[Is]))...)
                ));
            }
        } else if (memberFunctionPtr_ && objectInstance_) {
            if constexpr (std::is_same_v<ReturnType, void>) {
                memberFunctionPtr_(objectInstance_, object, 
                    std::forward<Args>(static_cast<Args>(args[Is]))...);
                return nullptr;
            } else {
                return static_cast<void*>(new ReturnType(
                    memberFunctionPtr_(objectInstance_, object,
                        std::forward<Args>(static_cast<Args>(args[Is]))...)
                ));
            }
        } else if (nativeFunction_) {
            if constexpr (std::is_same_v<ReturnType, void>) {
                nativeFunction_(std::forward<Args>(static_cast<Args>(args[Is]))...);
                return nullptr;
            } else {
                return static_cast<void*>(new ReturnType(
                    nativeFunction_(std::forward<Args>(static_cast<Args>(args[Is]))...)
                ));
            }
        }
        
        return nullptr;
    }
    
    bool bindToUFunction(UFunction* uFunction);
    bool bindToBlueprintGraph(void* blueprintFunction);
    
    std::string generateCPPWrapper() const;
    std::string generateNativeThunk() const;
    std::string generateBlueprintDispatcher() const;
    
    template<typename T>
    static constexpr bool isSupportedType() {
        return std::is_fundamental_v<T> ||
               std::is_pointer_v<T> ||
               std::is_reference_v<T> ||
               std::is_same_v<T, FString> ||
               std::is_same_v<T, FName> ||
               std::is_same_v<T, FText>;
    }
};

class KIWI_UNREAL_API UFunctionBindingFactory {
public:
    static std::unique_ptr<UFunctionBindingBase> createBinding(
        const UEFunctionBindingParams& params,
        const UEBindingContext& context);
    
    template<typename ReturnType, typename... Args>
    static std::unique_ptr<UFunctionBinding<ReturnType, Args...>>
    createTypedBinding(const UEFunctionBindingParams& params,
                      typename UFunctionBinding<ReturnType, Args...>::FunctionPtr funcPtr,
                      const UEBindingContext& context) {
        auto binding = std::make_unique<UFunctionBinding<ReturnType, Args...>>(params, funcPtr);
        configureTypedBinding(binding.get(), context);
        return binding;
    }
    
    static bool validateFunctionCompatibility(const UEFunctionBindingParams& params,
                                             AST::FunctionDecl* kiwiFunction,
                                             const UETypeConverter& converter);
    
private:
    static void configureTypedBinding(UFunctionBindingBase* binding,
                                     const UEBindingContext& context);
    
    static AST::TypePtr convertUETypeToKiwi(const std::string& ueTypeName,
                                           const UETypeConverter& converter);
    static std::string convertKiwiTypeToUE(AST::TypePtr kiwiType,
                                          const UETypeConverter& converter);
};

} // namespace Unreal
} // namespace kiwiLang