#pragma once

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

template <typename T>
struct DeferDoodad
{
	T lambda;
	DeferDoodad(T lambda) : lambda(lambda) {}
	~DeferDoodad() { lambda(); }
};

struct DeferDoodadHelp
{
	template <typename T>
	DeferDoodad<T> operator + (T t) { return t; }
};

#define DEFER_PASTE_(a, b) a##b
#define DEFER_PASTE(a, b) DEFER_PASTE_(a, b)
#define defer const auto DEFER_PASTE(defer_, __LINE__) = DeferDoodadHelp() + [&]()

typedef unsigned long long uintptr_t;

template <typename T, size_t n>
constexpr size_t arrayCount(T const (&)[n]) noexcept
{
    return n;
}

constexpr inline uintptr_t AlignAddress(uintptr_t value, size_t align)
{
	size_t mask = align - 1;
	return (value + mask) & ~mask;
}

template <typename T>
constexpr inline T* AlignPointer(T* ptr, size_t align)
{
	uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);
	intptr = AlignAddress(intptr, align);
	return reinterpret_cast<T*>(intptr);
}

template <typename T>
constexpr inline T*
CastAlignPointer(void* address)
{
	char* at = static_cast<char*>(address);
	at = AlignPointer(at, alignof(T));
	return reinterpret_cast<T*>(at);
}

template <typename T>
constexpr inline void
ConstructArrayInPlace(size_t count, T* base)
{
	for (size_t i = 0; i < count; i += 1)
	{
		new (base + i) T{};
	}
}

template <typename T>
constexpr inline void zeroStruct(T* ptr)
{
	memset(ptr, 0, sizeof(T));
}

template <typename T>
constexpr inline void zeroArray(size_t count, T* ptr)
{
	memset(ptr, 0, count * sizeof(T));
}