include/klang/internal/Runtime/Object.h
#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <memory>

namespace klang {
namespace internal {
namespace runtime {

class VM;
class GCVisitor;
class GC;

enum class ObjectType : uint8_t {
    STRING = 0,
    ARRAY,
    MAP,
    FUNCTION,
    CLOSURE,
    CLASS,
    INSTANCE,
    BOUND_METHOD,
    NATIVE,
    THREAD,
    MODULE,
    COROUTINE,
    UP_VALUE,
    WEAK_REF,
    USER_DATA
};

class HeapObject {
public:
    virtual ~HeapObject() = default;
    
    virtual ObjectType type() const = 0;
    virtual void visit(GCVisitor& visitor) = 0;
    virtual std::size_t size() const = 0;
    
    bool marked = false;
    HeapObject* next = nullptr;
    HeapObject* prev = nullptr;
};

class GCVisitor {
public:
    virtual ~GCVisitor() = default;
    virtual void visit(HeapObject* object) = 0;
    
    template<typename T>
    void visit(T* object) {
        if (object) visit(static_cast<HeapObject*>(object));
    }
    
    template<typename T>
    void visit(const std::unique_ptr<T>& object) {
        visit(object.get());
    }
    
    template<typename T>
    void visit(const T* object) {
        visit(const_cast<T*>(object));
    }
};

template<typename T>
class ObjectHandle {
public:
    ObjectHandle() : object_(nullptr) {}
    explicit ObjectHandle(T* object) : object_(object) {}
    
    T* get() const { return object_; }
    T* operator->() const { return object_; }
    T& operator*() const { return *object_; }
    
    explicit operator bool() const { return object_ != nullptr; }
    
    bool operator==(const ObjectHandle& other) const { return object_ == other.object_; }
    bool operator!=(const ObjectHandle& other) const { return object_ != other.object_; }
    
private:
    T* object_;
};

class ObjectHeader {
public:
    ObjectType type;
    uint8_t flags;
    uint16_t extra;
    
    static constexpr uint8_t MARKED_FLAG = 0x01;
    static constexpr uint8_t FINALIZED_FLAG = 0x02;
    static constexpr uint8_t WEAK_REF_FLAG = 0x04;
    
    bool isMarked() const { return flags & MARKED_FLAG; }
    void mark() { flags |= MARKED_FLAG; }
    void unmark() { flags &= ~MARKED_FLAG; }
};

constexpr std::size_t OBJECT_ALIGNMENT = alignof(std::max_align_t);

template<typename T, typename... Args>
T* allocateObject(GC& gc, Args&&... args);

class GCObject {
public:
    virtual ~GCObject() = default;
    
    ObjectHeader* header() { return reinterpret_cast<ObjectHeader*>(this); }
    const ObjectHeader* header() const { return reinterpret_cast<const ObjectHeader*>(this); }
    
    virtual void gcVisit(GCVisitor& visitor) = 0;
    virtual void gcFinalize() {}
    
    static void* operator new(std::size_t size, GC& gc);
    static void operator delete(void* ptr);
    
protected:
    GCObject() = default;
    
private:
    GCObject(const GCObject&) = delete;
    GCObject& operator=(const GCObject&) = delete;
};

template<typename T>
concept GCVisitable = std::is_base_of_v<GCObject, T>;

template<GCVisitable T>
class GCHandle {
public:
    GCHandle() : object_(nullptr) {}
    explicit GCHandle(T* object) : object_(object) {}
    
    T* get() const { return object_; }
    T* operator->() const { return object_; }
    T& operator*() const { return *object_; }
    
    explicit operator bool() const { return object_ != nullptr; }
    operator T*() const { return object_; }
    
    bool operator==(const GCHandle& other) const { return object_ == other.object_; }
    bool operator!=(const GCHandle& other) const { return object_ != other.object_; }
    
    void reset() { object_ = nullptr; }
    
private:
    T* object_;
};

template<typename T>
class GCArray {
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = T*;
    using const_iterator = const T*;
    
    GCArray() : data_(nullptr), size_(0), capacity_(0) {}
    
    iterator begin() { return data_; }
    iterator end() { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }
    
    size_type size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    T& operator[](size_type index) { return data_[index]; }
    const T& operator[](size_type index) const { return data_[index]; }
    
    void clear() { size_ = 0; }
    
private:
    T* data_;
    size_type size_;
    size_type capacity_;
};

} // namespace runtime
} // namespace internal
} // namespace klang