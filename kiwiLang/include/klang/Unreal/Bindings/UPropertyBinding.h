#pragma once

#include "kiwiLang/AST/Type.h"
#include "kiwiLang/Unreal/UETypes.h"
#include "kiwiLang/Runtime/Value.h"
#include <memory>
#include <string>
#include <functional>
#include <type_traits>

namespace kiwiLang {
namespace Unreal {

class UObjectRegistry;
class UETypeConverter;
struct UEBindingContext;

enum class UEPropertyAccessType {
    Direct,
    GetterSetter,
    FieldPtr,
    Virtual,
    Blueprint
};

struct UEPropertyBindingParams {
    std::string propertyName;
    AST::TypePtr kiwiType;
    UEPropertyInfo uePropertyInfo;
    UEPropertyAccessType accessType;
    std::uint32_t offset;
    std::uint32_t size;
    std::uint32_t arrayDim;
    bool isTransient;
    bool isConfig;
    bool isBlueprintVisible;
    bool isEditable;
    std::string category;
    std::string metaData;
    
    bool validate() const;
    std::string generateCPPType(const UETypeConverter& converter) const;
    std::string generateUHTMetaData() const;
};

class KIWI_UNREAL_API UPropertyBindingBase {
public:
    explicit UPropertyBindingBase(const UEPropertyBindingParams& params);
    virtual ~UPropertyBindingBase();
    
    const UEPropertyBindingParams& getParams() const { return params_; }
    
    virtual Runtime::Value getValue(void* object) const = 0;
    virtual bool setValue(void* object, const Runtime::Value& value) const = 0;
    virtual void* getRawPtr(void* object) const = 0;
    
    virtual bool bindToNative(void* nativeProperty) = 0;
    virtual bool bindToBlueprint(void* blueprintProperty) = 0;
    
    virtual std::string generateAccessorCode() const = 0;
    virtual std::string generateSerializerCode() const = 0;
    virtual std::string generateReplicationCode() const = 0;
    
    bool isBound() const { return isBound_; }
    bool isValid() const { return isValid_; }
    std::uint32_t getMemorySize() const { return params_.size; }
    
protected:
    UEPropertyBindingParams params_;
    bool isBound_;
    bool isValid_;
    
    virtual bool validateTypeCompatibility() = 0;
    virtual bool generateDefaultValue() = 0;
};

template<typename PropertyType>
class UPropertyBinding : public UPropertyBindingBase {
public:
    using Getter = std::function<PropertyType(void*)>;
    using Setter = std::function<void(void*, PropertyType)>;
    using RawPtr = PropertyType*(*)(void*);
    
    explicit UPropertyBinding(const UEPropertyBindingParams& params,
                             Getter getter = nullptr,
                             Setter setter = nullptr)
        : UPropertyBindingBase(params)
        , getter_(std::move(getter))
        , setter_(std::move(setter))
        , rawPtr_(nullptr)
        , defaultValue_() {
        initialize();
    }
    
    explicit UPropertyBinding(const UEPropertyBindingParams& params,
                             RawPtr rawPtr)
        : UPropertyBindingBase(params)
        , getter_(nullptr)
        , setter_(nullptr)
        , rawPtr_(rawPtr)
        , defaultValue_() {
        initialize();
    }
    
    Runtime::Value getValue(void* object) const override {
        if (!object) return Runtime::Value::null();
        
        if (rawPtr_) {
            PropertyType* ptr = rawPtr_(object);
            if (ptr) {
                return convertToRuntimeValue(*ptr);
            }
        } else if (getter_) {
            PropertyType value = getter_(object);
            return convertToRuntimeValue(value);
        }
        
        return Runtime::Value::null();
    }
    
    bool setValue(void* object, const Runtime::Value& value) const override {
        if (!object || !setter_) return false;
        
        auto converted = convertFromRuntimeValue(value);
        if (!converted) return false;
        
        setter_(object, *converted);
        return true;
    }
    
    void* getRawPtr(void* object) const override {
        if (!object) return nullptr;
        
        if (rawPtr_) {
            return rawPtr_(object);
        }
        
        return nullptr;
    }
    
    bool bindToNative(void* nativeProperty) override {
        if (!nativeProperty) return false;
        
        auto* uProperty = static_cast<UProperty*>(nativeProperty);
        return bindToUProperty(uProperty);
    }
    
    bool bindToBlueprint(void* blueprintProperty) override {
        if (!blueprintProperty) return false;
        
        return bindToBlueprintVariable(blueprintProperty);
    }
    
    std::string generateAccessorCode() const override {
        return generateCPPAccessors();
    }
    
    std::string generateSerializerCode() const override {
        return generateSerializationCode();
    }
    
    std::string generateReplicationCode() const override {
        return generateReplicationFunctions();
    }
    
    const PropertyType& getDefaultValue() const { return defaultValue_; }
    
private:
    Getter getter_;
    Setter setter_;
    RawPtr rawPtr_;
    PropertyType defaultValue_;
    
    void initialize() {
        isValid_ = validateTypeCompatibility() && generateDefaultValue();
        isBound_ = getter_ || setter_ || rawPtr_;
    }
    
    bool validateTypeCompatibility() override {
        return isSupportedType<PropertyType>();
    }
    
    bool generateDefaultValue() override {
        if constexpr (std::is_default_constructible_v<PropertyType>) {
            defaultValue_ = PropertyType();
            return true;
        } else {
            return false;
        }
    }
    
    Runtime::Value convertToRuntimeValue(const PropertyType& value) const {
        if constexpr (std::is_same_v<PropertyType, bool>) {
            return Runtime::Value::boolean(value);
        } else if constexpr (std::is_integral_v<PropertyType>) {
            if constexpr (std::is_signed_v<PropertyType>) {
                return Runtime::Value::integer(static_cast<std::int64_t>(value));
            } else {
                return Runtime::Value::uinteger(static_cast<std::uint64_t>(value));
            }
        } else if constexpr (std::is_floating_point_v<PropertyType>) {
            return Runtime::Value::floating(static_cast<double>(value));
        } else if constexpr (std::is_same_v<PropertyType, std::string>) {
            return Runtime::Value::string(value);
        } else if constexpr (std::is_pointer_v<PropertyType>) {
            return Runtime::Value::pointer(value);
        } else {
            return Runtime::Value::null();
        }
    }
    
    std::optional<PropertyType> convertFromRuntimeValue(const Runtime::Value& value) const {
        if constexpr (std::is_same_v<PropertyType, bool>) {
            if (value.isBoolean()) {
                return value.asBoolean();
            }
        } else if constexpr (std::is_integral_v<PropertyType>) {
            if (value.isInteger()) {
                return static_cast<PropertyType>(value.asInteger());
            } else if (value.isUInteger()) {
                return static_cast<PropertyType>(value.asUInteger());
            }
        } else if constexpr (std::is_floating_point_v<PropertyType>) {
            if (value.isFloating()) {
                return static_cast<PropertyType>(value.asFloating());
            }
        } else if constexpr (std::is_same_v<PropertyType, std::string>) {
            if (value.isString()) {
                return value.asString();
            }
        } else if constexpr (std::is_pointer_v<PropertyType>) {
            if (value.isPointer()) {
                return static_cast<PropertyType>(value.asPointer());
            }
        }
        
        return std::nullopt;
    }
    
    bool bindToUProperty(UProperty* uProperty);
    bool bindToBlueprintVariable(void* blueprintProperty);
    
    std::string generateCPPAccessors() const;
    std::string generateSerializationCode() const;
    std::string generateReplicationFunctions() const;
    
    template<typename T>
    static constexpr bool isSupportedType() {
        return std::is_fundamental_v<T> ||
               std::is_same_v<T, std::string> ||
               std::is_same_v<T, FString> ||
               std::is_same_v<T, FName> ||
               std::is_same_v<T, FText> ||
               std::is_same_v<T, TArray<T>> ||
               std::is_same_v<T, TMap<typename T::KeyType, typename T::ValueType>> ||
               std::is_same_v<T, TSet<typename T::ElementType>>;
    }
};

class KIWI_UNREAL_API UPropertyBindingFactory {
public:
    static std::unique_ptr<UPropertyBindingBase> createBinding(
        const UEPropertyBindingParams& params,
        const UEBindingContext& context);
    
    template<typename PropertyType>
    static std::unique_ptr<UPropertyBinding<PropertyType>>
    createTypedBinding(const UEPropertyBindingParams& params,
                      typename UPropertyBinding<PropertyType>::Getter getter,
                      typename UPropertyBinding<PropertyType>::Setter setter,
                      const UEBindingContext& context) {
        auto binding = std::make_unique<UPropertyBinding<PropertyType>>(
            params, std::move(getter), std::move(setter));
        configureTypedBinding(binding.get(), context);
        return binding;
    }
    
    static bool validatePropertyCompatibility(const UEPropertyBindingParams& params,
                                             AST::TypePtr kiwiType,
                                             const UETypeConverter& converter);
    
    static std::uint32_t calculatePropertySize(const UEPropertyBindingParams& params,
                                              const UETypeConverter& converter);
    
private:
    static void configureTypedBinding(UPropertyBindingBase* binding,
                                     const UEBindingContext& context);
    
    static AST::TypePtr determineKiwiType(const UEPropertyBindingParams& params,
                                         const UETypeConverter& converter);
    static UEPropertyInfo createPropertyInfoFromKiwi(AST::TypePtr kiwiType,
                                                    const std::string& name,
                                                    const UETypeConverter& converter);
};

} // namespace Unreal
} // namespace kiwiLang