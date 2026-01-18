src/internal/Runtime/String.cpp
#include "klang/internal/Runtime/String.h"
#include "klang/internal/Runtime/VM.h"
#include "klang/internal/Runtime/GC.h"
#include <algorithm>
#include <cstring>
#include <xxhash.h>

namespace klang {
namespace internal {
namespace runtime {

String::String(std::unique_ptr<char[]> data, std::size_t length, std::size_t hash)
    : data_(std::move(data)), length_(length), hash_(hash) {}

String* String::create(VM& vm, std::string_view str) {
    return create(vm, str.data(), str.length());
}

String* String::create(VM& vm, const char* data, std::size_t length) {
    auto buffer = std::make_unique<char[]>(length + 1);
    std::memcpy(buffer.get(), data, length);
    buffer[length] = '\0';
    
    auto hash = computeHash({data, length});
    auto* str = vm.gc().allocate<String>(std::move(buffer), length, hash);
    return str;
}

bool String::equals(const String* other) const {
    if (this == other) return true;
    if (hash_ != other->hash_ || length_ != other->length_) return false;
    return std::memcmp(data_.get(), other->data_.get(), length_) == 0;
}

bool String::equals(std::string_view other) const {
    if (length_ != other.length()) return false;
    return std::memcmp(data_.get(), other.data(), length_) == 0;
}

String* String::concat(VM& vm, const String* other) {
    std::size_t newLength = length_ + other->length_;
    auto buffer = std::make_unique<char[]>(newLength + 1);
    
    std::memcpy(buffer.get(), data_.get(), length_);
    std::memcpy(buffer.get() + length_, other->data_.get(), other->length_);
    buffer[newLength] = '\0';
    
    auto hash = XXH3_64bits(buffer.get(), newLength);
    return vm.gc().allocate<String>(std::move(buffer), newLength, hash);
}

String* String::substring(VM& vm, std::size_t start, std::size_t end) {
    if (start > end || end > length_) {
        start = 0;
        end = length_;
    }
    
    std::size_t subLength = end - start;
    return create(vm, data_.get() + start, subLength);
}

Value String::charAt(std::size_t index) const {
    if (index >= length_) return Value::nil();
    return Value::number(static_cast<double>(static_cast<unsigned char>(data_[index])));
}

std::size_t String::indexOf(const String* needle, std::size_t from) const {
    if (needle->length_ == 0) return from <= length_ ? from : length_;
    if (needle->length_ > length_ || from > length_ - needle->length_) return length_;
    
    const char* pos = std::search(
        data_.get() + from, data_.get() + length_,
        needle->data_.get(), needle->data_.get() + needle->length_
    );
    
    return pos == data_.get() + length_ ? length_ : pos - data_.get();
}

std::size_t String::computeHash(std::string_view str) {
    return XXH3_64bits(str.data(), str.length());
}

void String::visit(GCVisitor& visitor) {
    // Strings have no references to other objects
}

std::size_t String::size() const {
    return sizeof(String) + length_ + 1;
}

String* StringTable::intern(VM& vm, std::string_view str) {
    if (auto it = std::find_if(table_.begin(), table_.end(),
        [&](String* s) { return s->equals(str); }); it != table_.end()) {
        return *it;
    }
    
    auto* newStr = String::create(vm, str);
    table_.insert(newStr);
    return newStr;
}

String* StringTable::get(std::string_view str) const {
    for (auto* s : table_) {
        if (s->equals(str)) return s;
    }
    return nullptr;
}

void StringTable::sweep() {
    for (auto it = table_.begin(); it != table_.end();) {
        if (!(*it)->marked) {
            it = table_.erase(it);
        } else {
            ++it;
        }
    }
}

void StringTable::clear() {
    table_.clear();
}

} // namespace runtime
} // namespace internal
} // namespace klang