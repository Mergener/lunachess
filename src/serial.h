#ifndef LUNA_SERIAL_H
#define LUNA_SERIAL_H

#include <variant>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <istream>
#include <string>
#include <string_view>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <initializer_list>

#include "types.h"

namespace lunachess {

enum class SerialValueType {
    Int,
    List,
    Object
};

class SerialValue;
class SerialObject;
using SerialList = std::vector<SerialValue>;

template <typename T>
inline constexpr SerialValueType getMatchingType() {
    constexpr bool IS_INT = std::is_assignable<T&, i64>::value;
    constexpr bool IS_LST = std::is_assignable<T&, SerialList>::value;
    constexpr bool IS_OBJ = std::is_assignable<T&, SerialObject>::value;

    static_assert(IS_INT || IS_LST || IS_OBJ,
                  "Unsupported type T.");

    if constexpr (IS_INT) {
        return SerialValueType::Int;
    }
    if constexpr (IS_LST) {
        return SerialValueType::List;
    }
    return SerialValueType::Object;
}

class SerialValue {
    friend std::istream& operator>>(std::istream& stream, SerialValue& obj);
    friend std::ostream& operator<<(std::ostream& stream, const SerialValue& obj);

public:
    inline SerialValueType getType() const { return m_Type; }

    inline void set(i32 i) {
        disposeObjectIfExists();
        m_Value = i;
        m_Type = SerialValueType::Int;
    }

    inline void set(const SerialList& l) {
        set(l.begin(), l.end());
    }

    template <typename ITType>
    inline void set(ITType begin,
                    ITType end) {
        disposeObjectIfExists();
        m_Value = SerialList(begin, end);
        m_Type = SerialValueType::List;
    }

    void set(const SerialObject& object);

    template <typename T>
    using MappedType = typename std::conditional<std::is_assignable_v<T&, i64>, i64,
                        typename std::conditional<std::is_assignable_v<T&, SerialList>,
                        SerialList&, SerialObject&>::type>::type;

    template <typename T>
    inline MappedType<T> get() {
        constexpr bool IS_INT = std::is_assignable<T&, i32>::value;
        constexpr bool IS_LST = std::is_assignable<T&, SerialList>::value;
        constexpr bool IS_OBJ = std::is_assignable<T&, SerialObject>::value;

        static_assert(IS_INT || IS_LST || IS_OBJ,
                "Invalid type T for get() method.");

        if constexpr (IS_INT) {
            return std::get<0>(m_Value);
        }
        if constexpr (IS_LST) {
            return std::get<1>(m_Value);
        }
        if constexpr (IS_OBJ) {
            return *std::get<2>(m_Value);
        }
    }

    template <typename T>
    inline const MappedType<T> get() const {
        return const_cast<SerialValue*>(this)->get<T>();
    }

    inline SerialValue(i64 i = 0)
        : m_Value(i), m_Type(SerialValueType::Int) { }

    SerialValue(const SerialObject& object);

    inline SerialValue(const SerialList& list) {
        set(list);
    }

    template <typename ITType>
    inline SerialValue(ITType begin, ITType end) {
        set(begin, end);
    }

    inline SerialValue(const std::initializer_list<SerialValue>& lst) {
        set(lst.begin(), lst.end());
    }

    ~SerialValue();

private:
    std::variant<i64, SerialList, SerialObject*> m_Value;
    SerialValueType m_Type;

    void disposeObjectIfExists();
};

class SerialObject {
    friend std::istream& operator>>(std::istream& stream, SerialObject& obj);
    friend std::ostream& operator<<(std::ostream& stream, const SerialObject& obj);

public:
    inline SerialValue& operator[](const std::string& s) { return m_Values[s]; }
    inline const SerialValue& operator[](const std::string& s) const { return m_Values.at(s); }

    template <typename T>
    bool tryGet(const std::string& key, T& x) {
        constexpr SerialValueType SVT = getMatchingType<T>();

        auto it = m_Values.find(key);

        if (it == m_Values.end()) {
            return false;
        }

        auto& v = it->second;
        if (v.getType() != SVT) {
            return false;
        }

        x = v.get<T>();
        return true;
    }

    inline void clear() { m_Values.clear(); }

private:
    std::unordered_map<std::string, SerialValue> m_Values;
};

class BadSerial : std::runtime_error {
public:
    inline BadSerial(std::string_view sv)
        : std::runtime_error(std::string(sv)) { }
};

std::istream& operator>>(std::istream& stream, SerialValueType& type);
std::ostream& operator<<(std::ostream& stream, const SerialValueType& obj);

std::istream& operator>>(std::istream& stream, SerialValue& obj);
std::ostream& operator<<(std::ostream& stream, const SerialValue& obj);

std::istream& operator>>(std::istream& stream, SerialObject& obj);
std::ostream& operator<<(std::ostream& stream, const SerialObject& obj);

}

#endif // LUNA_SERIAL_H
