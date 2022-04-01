#include "serial.h"

namespace lunachess {

void SerialValue::disposeObjectIfExists() {
    if (m_Type != SerialValueType::Object) {
        return;
    }

    auto& obj = std::get<SerialObject*>(m_Value);

    if (obj != nullptr) {
        delete obj;
        obj = nullptr;
    }
}

void SerialValue::set(const SerialObject& object) {
    SerialObject* obj = new SerialObject(object);
    m_Value = obj;
    m_Type = SerialValueType::Object;
}

SerialValue::~SerialValue() {
    if (m_Type == SerialValueType::Object) {
        delete std::get<SerialObject*>(m_Value);
    }
}

std::istream& operator>>(std::istream& stream, SerialValueType& type) {
    std::string typeStr;
    stream >> typeStr;

    if (typeStr == "int32") {
        type = SerialValueType::Int;
    }
    else if (typeStr == "list") {
        type = SerialValueType::List;
    }
    else if (typeStr == "object") {
        type = SerialValueType::Object;
    }
    else {
        throw BadSerial("Invalid type name.");
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SerialValueType& type) {
    switch (type) {
        case SerialValueType::Int:
            stream << "int32";
            break;

        case SerialValueType::List:
            stream << "list";
            break;

        case SerialValueType::Object:
            stream << "object";
            break;

        default:
            throw BadSerial("Invalid type name.");
    }
    return stream;
}

std::istream& operator>>(std::istream& stream, SerialObject& obj) {
    int nElems;

    stream >> nElems;

    for (int i = 0; i < nElems; ++i) {
        std::string name;

        stream >> name;

        SerialValue val;
        stream >> val;

        obj[name] = val;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SerialObject& obj) {
    stream << obj.m_Values.size() << std::endl;
    for (const auto& kv : obj.m_Values) {
        stream << kv.first << " " << kv.second << std::endl;
    }
    stream << std::endl;
    return stream;
}

std::istream& operator>>(std::istream& stream, SerialValue& v) {
    stream >> v.m_Type;

    switch (v.m_Type) {
        case SerialValueType::Int: {
            i32 val;
            stream >> val;
            v.set(val);
            break;
        }

        case SerialValueType::List: {
            v.disposeObjectIfExists();
            i32 nElems;

            // Get list size
            stream >> nElems;

            // Initialize list
            v.m_Value = SerialList();

            // Add elements
            auto& list = std::get<SerialList>(v.m_Value);
            for (i32 i = 0; i < nElems; ++i) {
                SerialValue value;
                stream >> value;
                list.push_back(value);
            }
            break;
        }

        case SerialValueType::Object: {
            v.disposeObjectIfExists();
            SerialObject *object = new SerialObject();
            v.m_Value = object;
            stream >> *object;
            break;
        }

        default:
            throw BadSerial("Invalid type");
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SerialValue& v) {
    stream << v.m_Type << ' ';

    switch (v.m_Type) {
        case SerialValueType::Int: {
            stream << v.get<i64>();
            break;
        }

        case SerialValueType::List: {
            const auto& list = std::get<SerialList>(v.m_Value);

            // Get list size
            stream << static_cast<i32>(list.size()) << std::endl;

            // Add elements
            for (i32 i = 0; i < list.size(); ++i) {
                stream << list[i] << std::endl;
            }
            break;
        }

        case SerialValueType::Object: {
            stream << *std::get<SerialObject*>(v.m_Value);
            break;
        }

        default:
            throw BadSerial("Invalid type");
    }
    return stream;
}


}