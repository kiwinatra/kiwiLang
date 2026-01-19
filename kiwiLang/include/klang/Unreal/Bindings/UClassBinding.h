#pragma once

#include "kiwiLang/AST/Decl.h"
#include "kiwiLang/Unreal/UETypes.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace kiwiLang {
namespace Unreal {

class UObjectRegistry;
class UERuntime;

struct UEBindingContext {
    UObjectRegistry* registry;
    UERuntime* runtime;
    DiagnosticEngine* diagnostics;
    
    void* userData;
    
    template<typename T>
    T* getUserData() const { return static_cast<T*>(userData); }
};

class KIWI_UNREAL_API UClassBindingBase {
public:
    explicit UClassBindingBase(const UEClassInfo& classInfo);
    virtual ~UClassBindingBase();
    
    const UEClassInfo& getClassInfo() const { return classInfo_; }
    virtual void* createInstance() const = 0;
    virtual void destroyInstance(void* object) const = 0;
    virtual bool initializeInstance(void* object) const = 0;
    
    virtual std::string generateWrapperCode() const = 0;
    virtual std::string generateHeaderCode() const = 0;
    
    bool hasProperty(const std::string& name) const;
    bool hasFunction(const std::string& name) const;
    
protected:
    UEClassInfo classInfo_;
    std::vector<const UEPropertyInfo*> properties_;
    std::vector<const UEFunctionInfo*> functions_;
    
    void collectProperties();
    void collectFunctions();
};

template<typename NativeClass, typename KiwiClass>
class UClassBinding : public UClassBindingBase {
public:
    using Constructor = std::function<NativeClass*()>;
    using Destructor = std::function<void(NativeClass*)>;
    using PropertyGetter = std::function<void*(NativeClass*, const std::string&)>;
    using PropertySetter = std::function<void(NativeClass*, const std::string&, void*)>;
    using FunctionInvoker = std::function<void*(NativeClass*, const std::string&, void**)>;
    
    explicit UClassBinding(const UEClassInfo& classInfo,
                          Constructor ctor = nullptr,
                          Destructor dtor = nullptr)
        : UClassBindingBase(classInfo)
        , constructor_(std::move(ctor))
        , destructor_(std::move(dtor)) {
        if (!constructor_) {
            constructor_ = []() -> NativeClass* { return new NativeClass(); };
        }
        if (!destructor_) {
            destructor_ = [](NativeClass* obj) { delete obj; };
        }
    }
    
    void* createInstance() const override {
        return constructor_();
    }
    
    void destroyInstance(void* object) const override {
        destructor_(static_cast<NativeClass*>(object));
    }
    
    bool initializeInstance(void* object) const override {
        auto* nativeObj = static_cast<NativeClass*>(object);
        return initializeNativeObject(nativeObj);
    }
    
    void registerProperty(const std::string& name,
                         PropertyGetter getter,
                         PropertySetter setter = nullptr) {
        propertyGetters_[name] = std::move(getter);
        if (setter) {
            propertySetters_[name] = std::move(setter);
        }
    }
    
    void registerFunction(const std::string& name,
                         FunctionInvoker invoker) {
        functionInvokers_[name] = std::move(invoker);
    }
    
    void* getProperty(void* object, const std::string& name) const {
        auto it = propertyGetters_.find(name);
        if (it != propertyGetters_.end()) {
            return it->second(static_cast<NativeClass*>(object), name);
        }
        return nullptr;
    }
    
    bool setProperty(void* object, const std::string& name, void* value) const {
        auto it = propertySetters_.find(name);
        if (it != propertySetters_.end()) {
            it->second(static_cast<NativeClass*>(object), name, value);
            return true;
        }
        return false;
    }
    
    void* invokeFunction(void* object, const std::string& name, void** args) const {
        auto it = functionInvokers_.find(name);
        if (it != functionInvokers_.end()) {
            return it->second(static_cast<NativeClass*>(object), name, args);
        }
        return nullptr;
    }
    
    std::string generateWrapperCode() const override {
        return generateNativeWrapper();
    }
    
    std::string generateHeaderCode() const override {
        return generateNativeHeader();
    }
    
private:
    Constructor constructor_;
    Destructor destructor_;
    std::unordered_map<std::string, PropertyGetter> propertyGetters_;
    std::unordered_map<std::string, PropertySetter> propertySetters_;
    std::unordered_map<std::string, FunctionInvoker> functionInvokers_;
    
    bool initializeNativeObject(NativeClass* obj) const {
        return true;
    }
    
    std::string generateNativeWrapper() const;
    std::string generateNativeHeader() const;
    
    static_assert(std::is_base_of_v<UObject, NativeClass>,
                  "NativeClass must inherit from UObject");
};

class KIWI_UNREAL_API UClassBindingFactory {
public:
    static std::unique_ptr<UClassBindingBase> createBinding(const UEClassInfo& classInfo,
                                                           const UEBindingContext& context);
    
    template<typename NativeClass, typename KiwiClass>
    static std::unique_ptr<UClassBinding<NativeClass, KiwiClass>>
    createTypedBinding(const UEClassInfo& classInfo,
                      const UEBindingContext& context) {
        auto binding = std::make_unique<UClassBinding<NativeClass, KiwiClass>>(classInfo);
        configureTypedBinding(binding.get(), context);
        return binding;
    }
    
private:
    static void configureTypedBinding(UClassBindingBase* binding,
                                     const UEBindingContext& context);
    
    static void generateDefaultPropertyAccessors(UClassBindingBase* binding,
                                                const UEBindingContext& context);
    static void generateDefaultFunctionInvokers(UClassBindingBase* binding,
                                               const UEBindingContext& context);
};

} // namespace Unreal
} // namespace kiwiLang