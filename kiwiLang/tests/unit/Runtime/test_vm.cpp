#include "kiwiLang/internal/Runtime/VM.h"
#include "kiwiLang/internal/Runtime/Value.h"
#include "kiwiLang/internal/Runtime/Object.h"
#include "kiwiLang/internal/Runtime/String.h"
#include "kiwiLang/internal/Runtime/Array.h"
#include "kiwiLang/internal/Runtime/GarbageCollector.h"
#include <gtest/gtest.h>
#include <memory>

using namespace kiwiLang;
using namespace kiwiLang::internal::runtime;

class VMTest : public ::testing::Test {
protected:
    void SetUp() override {
        vm_ = std::make_unique<VM>();
        vm_->initialize();
    }
    
    void TearDown() override {
        vm_->shutdown();
    }
    
    std::unique_ptr<VM> vm_;
};

TEST_F(VMTest, ValueCreation) {
    Value nil = Value::nil();
    EXPECT_TRUE(nil.isNil());
    
    Value boolean = Value::boolean(true);
    EXPECT_TRUE(boolean.isBoolean());
    EXPECT_TRUE(boolean.asBoolean());
    
    Value number = Value::number(42.5);
    EXPECT_TRUE(number.isNumber());
    EXPECT_DOUBLE_EQ(number.asNumber(), 42.5);
    
    Value integer = Value::integer(42);
    EXPECT_TRUE(integer.isInteger());
    EXPECT_EQ(integer.asInteger(), 42);
}

TEST_F(VMTest, StringOperations) {
    String* str = String::create(*vm_, "Hello, World!");
    Value stringValue = Value::object(str);
    
    EXPECT_TRUE(stringValue.isObject());
    EXPECT_EQ(stringValue.asObject()->type(), ObjectType::STRING);
    
    String* stringObj = static_cast<String*>(stringValue.asObject());
    EXPECT_EQ(stringObj->length(), 13);
    EXPECT_EQ(stringObj->toString(), "Hello, World!");
}

TEST_F(VMTest, StringConcatenation) {
    String* str1 = String::create(*vm_, "Hello, ");
    String* str2 = String::create(*vm_, "World!");
    
    String* result = str1->concat(*vm_, str2);
    EXPECT_EQ(result->toString(), "Hello, World!");
}

TEST_F(VMTest, ArrayOperations) {
    Array* arr = Array::create(*vm_, 5);
    
    EXPECT_EQ(arr->size(), 0);
    EXPECT_EQ(arr->capacity(), 8); // Default capacity
    
    arr->push(Value::integer(1));
    arr->push(Value::integer(2));
    arr->push(Value::integer(3));
    
    EXPECT_EQ(arr->size(), 3);
    EXPECT_EQ(arr->get(0).asInteger(), 1);
    EXPECT_EQ(arr->get(2).asInteger(), 3);
    
    Value popped = arr->pop();
    EXPECT_EQ(popped.asInteger(), 3);
    EXPECT_EQ(arr->size(), 2);
}

TEST_F(VMTest, ArrayIndexing) {
    Array* arr = Array::create(*vm_, 3);
    
    arr->push(Value::integer(10));
    arr->push(Value::integer(20));
    arr->push(Value::integer(30));
    
    EXPECT_EQ((*arr)[0].asInteger(), 10);
    EXPECT_EQ((*arr)[2].asInteger(), 30);
    
    (*arr)[1] = Value::integer(99);
    EXPECT_EQ(arr->get(1).asInteger(), 99);
}

TEST_F(VMTest, ArraySlice) {
    Array* arr = Array::create(*vm_, 5);
    
    for (int i = 0; i < 5; ++i) {
        arr->push(Value::integer(i * 10));
    }
    
    Array* slice = arr->slice(*vm_, 1, 4);
    EXPECT_EQ(slice->size(), 3);
    EXPECT_EQ(slice->get(0).asInteger(), 10);
    EXPECT_EQ(slice->get(2).asInteger(), 30);
}

TEST_F(VMTest, GarbageCollection) {
    GCConfig config;
    config.heapSize = 1024 * 1024; // 1MB
    
    vm_->gc().configure(config);
    
    // Allocate many objects to trigger GC
    std::vector<Value> values;
    
    for (int i = 0; i < 10000; ++i) {
        String* str = String::create(*vm_, "Test String " + std::to_string(i));
        values.push_back(Value::object(str));
        
        Array* arr = Array::create(*vm_, 10);
        for (int j = 0; j < 10; ++j) {
            arr->push(Value::integer(j));
        }
        values.push_back(Value::object(arr));
    }
    
    // Force GC
    vm_->gc().collect();
    
    // Objects should still be alive
    EXPECT_GT(vm_->gc().allocatedBytes(), 0);
}

TEST_F(VMTest, FunctionCall) {
    // Create a simple bytecode function
    std::vector<uint8_t> bytecode = {
        // Load constant 0 (42)
        0x01, 0x00,
        // Return
        0x0F
    };
    
    std::vector<Value> constants = {
        Value::integer(42)
    };
    
    Function* func = Function::create(*vm_, 
                                     "test",
                                     0, // arity
                                     bytecode,
                                     constants,
                                     {});
    
    Value result = vm_->callFunction(func, {});
    EXPECT_TRUE(result.isInteger());
    EXPECT_EQ(result.asInteger(), 42);
}

TEST_F(VMTest, NativeFunction) {
    auto nativeFunc = [](VM& vm, const std::vector<Value>& args) -> Value {
        if (args.size() == 2 && args[0].isInteger() && args[1].isInteger()) {
            return Value::integer(args[0].asInteger() + args[1].asInteger());
        }
        return Value::nil();
    };
    
    NativeFunction* func = NativeFunction::create(*vm_, 
                                                 "add",
                                                 2,
                                                 nativeFunc);
    
    std::vector<Value> args = {
        Value::integer(10),
        Value::integer(32)
    };
    
    Value result = vm_->callNative(func, args);
    EXPECT_TRUE(result.isInteger());
    EXPECT_EQ(result.asInteger(), 42);
}

TEST_F(VMTest, Closure) {
    std::vector<uint8_t> bytecode = {
        // Load upvalue 0
        0x10, 0x00,
        // Return
        0x0F
    };
    
    std::vector<Value> constants;
    std::vector<Value> upvalues = {
        Value::integer(42)
    };
    
    Function* func = Function::create(*vm_,
                                     "closure",
                                     0,
                                     bytecode,
                                     constants,
                                     {});
    
    Closure* closure = Closure::create(*vm_, func, upvalues);
    
    Value result = vm_->callClosure(closure, {});
    EXPECT_TRUE(result.isInteger());
    EXPECT_EQ(result.asInteger(), 42);
}

TEST_F(VMTest, Coroutine) {
    std::vector<uint8_t> bytecode = {
        // Load constant 0 (1)
        0x01, 0x00,
        // Yield
        0x11,
        // Load constant 1 (2)
        0x01, 0x01,
        // Yield
        0x11,
        // Load constant 2 (3)
        0x01, 0x02,
        // Return
        0x0F
    };
    
    std::vector<Value> constants = {
        Value::integer(1),
        Value::integer(2),
        Value::integer(3)
    };
    
    Function* func = Function::create(*vm_,
                                     "coroutine",
                                     0,
                                     bytecode,
                                     constants,
                                     {});
    
    Coroutine* coroutine = Coroutine::create(*vm_, func);
    
    // First yield
    Value result1 = vm_->resumeCoroutine(coroutine, {});
    EXPECT_TRUE(result1.isInteger());
    EXPECT_EQ(result1.asInteger(), 1);
    EXPECT_FALSE(coroutine->isFinished());
    
    // Second yield
    Value result2 = vm_->resumeCoroutine(coroutine, {});
    EXPECT_TRUE(result2.isInteger());
    EXPECT_EQ(result2.asInteger(), 2);
    EXPECT_FALSE(coroutine->isFinished());
    
    // Final return
    Value result3 = vm_->resumeCoroutine(coroutine, {});
    EXPECT_TRUE(result3.isInteger());
    EXPECT_EQ(result3.asInteger(), 3);
    EXPECT_TRUE(coroutine->isFinished());
}

TEST_F(VMTest, ExceptionHandling) {
    std::vector<uint8_t> bytecode = {
        // Load constant 0 (error message)
        0x01, 0x00,
        // Throw exception
        0x12
    };
    
    std::vector<Value> constants = {
        Value::object(String::create(*vm_, "Test error"))
    };
    
    Function* func = Function::create(*vm_,
                                     "thrower",
                                     0,
                                     bytecode,
                                     constants,
                                     {});
    
    try {
        vm_->callFunction(func, {});
        FAIL() << "Expected exception";
    } catch (const VMException& e) {
        EXPECT_EQ(e.what(), std::string("Test error"));
    }
}

TEST_F(VMTest, TryCatch) {
    std::vector<uint8_t> bytecode = {
        // Begin try block
        0x13, 0x05, // try, catch offset = 5
        
        // Try block: throw error
        0x01, 0x00, // Load constant 0
        0x12,       // Throw
        
        // Catch block
        0x01, 0x01, // Load constant 1 (fallback)
        0x0F        // Return
    };
    
    std::vector<Value> constants = {
        Value::object(String::create(*vm_, "Error")),
        Value::integer(42)
    };
    
    Function* func = Function::create(*vm_,
                                     "trycatch",
                                     0,
                                     bytecode,
                                     constants,
                                     {});
    
    Value result = vm_->callFunction(func, {});
    EXPECT_TRUE(result.isInteger());
    EXPECT_EQ(result.asInteger(), 42);
}

TEST_F(VMTest, MemoryAllocation) {
    std::size_t initialBytes = vm_->gc().allocatedBytes();
    
    // Allocate some objects
    for (int i = 0; i < 1000; ++i) {
        String::create(*vm_, "String " + std::to_string(i));
        Array::create(*vm_, 10);
    }
    
    std::size_t afterAllocBytes = vm_->gc().allocatedBytes();
    EXPECT_GT(afterAllocBytes, initialBytes);
    
    // Force GC
    vm_->gc().collect();
    
    std::size_t afterGCBytes = vm_->gc().allocatedBytes();
    EXPECT_LT(afterGCBytes, afterAllocBytes);
}

TEST_F(VMTest, ObjectIdentity) {
    String* str1 = String::create(*vm_, "Hello");
    String* str2 = String::create(*vm_, "Hello");
    String* str3 = String::create(*vm_, "World");
    
    // Different objects should have different identities
    EXPECT_NE(str1, str2);
    
    // But equal strings should compare equal
    EXPECT_TRUE(str1->equals(str2));
    EXPECT_FALSE(str1->equals(str3));
}

TEST_F(VMTest, TypeChecking) {
    Value nil = Value::nil();
    Value boolean = Value::boolean(true);
    Value number = Value::number(3.14);
    Value integer = Value::integer(42);
    Value string = Value::object(String::create(*vm_, "test"));
    Value array = Value::object(Array::create(*vm_, 0));
    
    EXPECT_TRUE(nil.isNil());
    EXPECT_TRUE(boolean.isBoolean());
    EXPECT_TRUE(number.isNumber());
    EXPECT_TRUE(integer.isInteger());
    EXPECT_TRUE(string.isObject());
    EXPECT_TRUE(array.isObject());
    
    EXPECT_FALSE(nil.isBoolean());
    EXPECT_FALSE(boolean.isNumber());
    EXPECT_FALSE(number.isObject());
    EXPECT_FALSE(integer.isNil());
}

TEST_F(VMTest, ValueEquality) {
    Value v1 = Value::integer(42);
    Value v2 = Value::integer(42);
    Value v3 = Value::integer(43);
    Value v4 = Value::number(42.0);
    
    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_FALSE(v1 == v4); // Different types
    
    Value s1 = Value::object(String::create(*vm_, "hello"));
    Value s2 = Value::object(String::create(*vm_, "hello"));
    Value s3 = Value::object(String::create(*vm_, "world"));
    
    EXPECT_TRUE(s1 == s2); // String interning should make these equal
    EXPECT_FALSE(s1 == s3);
}

TEST_F(VMTest, StackOperations) {
    vm_->push(Value::integer(1));
    vm_->push(Value::integer(2));
    vm_->push(Value::integer(3));
    
    EXPECT_EQ(vm_->stackSize(), 3);
    
    Value top = vm_->peek(0);
    EXPECT_EQ(top.asInteger(), 3);
    
    Value second = vm_->peek(1);
    EXPECT_EQ(second.asInteger(), 2);
    
    vm_->pop(); // Remove 3
    EXPECT_EQ(vm_->stackSize(), 2);
    
    vm_->popN(2); // Remove remaining
    EXPECT_EQ(vm_->stackSize(), 0);
}

TEST_F(VMTest, GlobalVariables) {
    vm_->setGlobal("pi", Value::number(3.14159));
    vm_->setGlobal("answer", Value::integer(42));
    
    Value pi = vm_->getGlobal("pi");
    Value answer = vm_->getGlobal("answer");
    Value missing = vm_->getGlobal("nonexistent");
    
    EXPECT_TRUE(pi.isNumber());
    EXPECT_DOUBLE_EQ(pi.asNumber(), 3.14159);
    
    EXPECT_TRUE(answer.isInteger());
    EXPECT_EQ(answer.asInteger(), 42);
    
    EXPECT_TRUE(missing.isNil());
}

TEST_F(VMTest, ModuleSystem) {
    Module* module = Module::create(*vm_, "test_module");
    
    module->setExport("add", Value::object(NativeFunction::create(*vm_, "add", 2,
        [](VM& vm, const std::vector<Value>& args) -> Value {
            return Value::integer(args[0].asInteger() + args[1].asInteger());
        })));
    
    module->setExport("version", Value::integer(1));
    
    vm_->registerModule(module);
    
    Value addFunc = vm_->import("test_module", "add");
    Value version = vm_->import("test_module", "version");
    
    EXPECT_TRUE(addFunc.isObject());
    EXPECT_TRUE(version.isInteger());
    EXPECT_EQ(version.asInteger(), 1);
}

TEST_F(VMTest, PerformanceBenchmark) {
    // Simple performance test for arithmetic operations
    const int ITERATIONS = 1000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        Value a = Value::integer(i);
        Value b = Value::integer(i * 2);
        
        // Simulate some operation
        Value result = Value::integer(a.asInteger() + b.asInteger());
        (void)result; // Avoid unused warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000); // Should complete in under 1 second
}

TEST_F(VMTest, StressTest) {
    // Stress test with many objects and operations
    const int OBJECTS = 10000;
    const int OPERATIONS = 1000;
    
    std::vector<Value> objects;
    
    // Create many objects
    for (int i = 0; i < OBJECTS; ++i) {
        String* str = String::create(*vm_, "Object " + std::to_string(i));
        Array* arr = Array::create(*vm_, 10);
        
        for (int j = 0; j < 10; ++j) {
            arr->push(Value::integer(j));
        }
        
        objects.push_back(Value::object(str));
        objects.push_back(Value::object(arr));
    }
    
    // Perform many operations
    for (int op = 0; op < OPERATIONS; ++op) {
        for (std::size_t i = 0; i < objects.size(); i += 2) {
            if (objects[i].isObject() && objects[i].asObject()->type() == ObjectType::STRING) {
                String* str = static_cast<String*>(objects[i].asObject());
                String* newStr = str->concat(*vm_, str); // Double the string
                objects[i] = Value::object(newStr);
            }
        }
        
        // Force occasional GC
        if (op % 100 == 0) {
            vm_->gc().collect();
        }
    }
    
    // Verify objects are still accessible
    EXPECT_GT(objects.size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}