#ifndef LUNA_DEFS_H
#define LUNA_DEFS_H

#define C64(n) n##ULL

#define BITWISE_ENUM_CLASS(name, baseType, ...) \
	enum class name : baseType { __VA_ARGS__ }; \
	IMPLEMENT_BITOPS_FOR_ENUM(name, baseType)

#define IMPLEMENT_BITOPS_FOR_ENUM(name, baseType) \
	inline name operator&(name a, name b) { return static_cast<name>(static_cast<baseType>(a) & static_cast<baseType>(b)); } \
	inline name operator|(name a, name b) { return static_cast<name>(static_cast<baseType>(a) | static_cast<baseType>(b)); } \
	inline name operator^(name a, name b) { return static_cast<name>(static_cast<baseType>(a) ^ static_cast<baseType>(b)); } \
	inline name operator+(name a, name b) { return static_cast<name>(static_cast<baseType>(a) + static_cast<baseType>(b)); } \
	inline name& operator|=(name& a, name b) { a = a | b; return a; } \
	inline name& operator&=(name& a, name b) { a = a & b; return a; } \
	inline name& operator^=(name& a, name b) { a = a ^ b; return a; } \
	inline name& operator+=(name& a, name b) { a = a + b; return a; } \
	inline name operator&(name a, baseType b) { return static_cast<name>(static_cast<baseType>(a) & b); } \
	inline name operator|(name a, baseType b) { return static_cast<name>(static_cast<baseType>(a) | b); } \
	inline name operator^(name a, baseType b) { return static_cast<name>(static_cast<baseType>(a) ^ b); } \
	inline name operator+(name a, baseType b) { return static_cast<name>(static_cast<baseType>(a) + b); } \
	inline name& operator|=(name& a, baseType b) { a = a | b; return a; } \
	inline name& operator&=(name& a, baseType b) { a = a & b; return a; } \
	inline name& operator^=(name& a, baseType b) { a = a ^ b; return a; } \
	inline name& operator+=(name& a, baseType b) { a = a + b; return a; } 

#endif // LUNA_DEFS_H