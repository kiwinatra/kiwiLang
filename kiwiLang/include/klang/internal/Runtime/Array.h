include/klang/internal/Runtime/Array.h
#pragma once
#include "Object.h"
#include "Value.h"
#include <cstddef>
#include <memory>
#include <vector>
#include <type_traits>

namespace klang {
namespace internal {
namespace runtime {

class VM;
class GCVisitor;

class Array final : public HeapObject {
public:
    static constexpr ObjectType TYPE = ObjectType::ARRAY;
    
    static Array* create(VM& vm, std::size_t capacity = 0);
    static Array* create(VM& vm, const Value* values, std::size_t count);
    
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    
    Value get(std::size_t index) const;
    bool set(std::size_t index, Value value);
    bool push(Value value);
    Value pop();
    
    void insert(std::size_t index, Value value);
    void remove(std::size_t index);
    void clear();
    
    Value* data() { return elements_.get(); }
    const Value* data() const { return elements_.get(); }
    
    Value& operator[](std::size_t index);
    const Value& operator[](std::size_t index) const;
    
    Array* slice(VM& vm, std::size_t start, std::size_t end) const;
    Array* concat(VM& vm, const Array* other) const;
    
    void sort(VM& vm);
    std::size_t indexOf(Value value, std::size_t fromIndex = 0) const;
    
    ObjectType type() const override { return TYPE; }
    void visit(GCVisitor& visitor) override;
    std::size_t size() const override;
    
    class Iterator {
    public:
        Iterator(Array* array, std::size_t index);
        
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
        
        Iterator& operator++();
        Value operator*() const;
        
    private:
        Array* array_;
        std::size_t index_;
    };
    
    Iterator begin();
    Iterator end();

private:
    Array(std::unique_ptr<Value[]> elements, std::size_t capacity);
    
    std::unique_ptr<Value[]> elements_;
    std::size_t size_;
    std::size_t capacity_;
    
    void grow();
    void ensureCapacity(std::size_t minCapacity);
    
    static constexpr std::size_t MIN_CAPACITY = 8;
    static constexpr double GROWTH_FACTOR = 1.5;
};

class ArrayBuffer final : public HeapObject {
public:
    static constexpr ObjectType TYPE = ObjectType::ARRAY_BUFFER;
    
    static ArrayBuffer* create(VM& vm, std::size_t byteLength);
    static ArrayBuffer* create(VM& vm, const std::byte* data, std::size_t byteLength);
    
    std::byte* data() { return data_.get(); }
    const std::byte* data() const { return data_.get(); }
    std::size_t byteLength() const { return byteLength_; }
    
    ArrayBuffer* slice(VM& vm, std::size_t begin, std::size_t end) const;
    
    ObjectType type() const override { return TYPE; }
    void visit(GCVisitor& visitor) override;
    std::size_t size() const override;

private:
    ArrayBuffer(std::unique_ptr<std::byte[]> data, std::size_t byteLength);
    
    std::unique_ptr<std::byte[]> data_;
    std::size_t byteLength_;
};

template<typename T>
class TypedArray : public HeapObject {
    static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
    
public:
    static constexpr ObjectType TYPE = ObjectType::ARRAY;
    
    static TypedArray<T>* create(VM& vm, std::size_t length);
    static TypedArray<T>* create(VM& vm, const T* data, std::size_t length);
    
    std::size_t length() const { return length_; }
    T* data() { return data_.get(); }
    const T* data() const { return data_.get(); }
    
    T get(std::size_t index) const;
    void set(std::size_t index, T value);
    
    TypedArray<T>* slice(VM& vm, std::size_t begin, std::size_t end) const;
    
    ObjectType type() const override { return TYPE; }
    void visit(GCVisitor& visitor) override;
    std::size_t size() const override;

private:
    TypedArray(std::unique_ptr<T[]> data, std::size_t length);
    
    std::unique_ptr<T[]> data_;
    std::size_t length_;
};

using Int8Array = TypedArray<int8_t>;
using Uint8Array = TypedArray<uint8_t>;
using Int16Array = TypedArray<int16_t>;
using Uint16Array = TypedArray<uint16_t>;
using Int32Array = TypedArray<int32_t>;
using Uint32Array = TypedArray<uint32_t>;
using Float32Array = TypedArray<float>;
using Float64Array = TypedArray<double>;

class ArrayIterator final : public HeapObject {
public:
    static constexpr ObjectType TYPE = ObjectType::ITERATOR;
    
    static ArrayIterator* create(VM& vm, Array* array);
    
    bool hasNext() const;
    Value next();
    
    ObjectType type() const override { return TYPE; }
    void visit(GCVisitor& visitor) override;
    std::size_t size() const override;

private:
    explicit ArrayIterator(Array* array);
    
    Array* array_;
    std::size_t index_;
};

} // namespace runtime
} // namespace internal
} // namespace klang