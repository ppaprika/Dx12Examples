#pragma once

#define WIN32_LEAN_AND_MEAN
#include <exception>
#include <iostream>
#include <Windows.h>
#include <wrl/client.h>
#include <D3DX12/d3d12.h>

inline void ThrowIfFailed(HRESULT hr)
{
	if(FAILED(hr))
	{
		throw std::exception();
	}
}

inline void ShowLastError()
{
	DWORD errorCode = GetLastError();
	std::cerr << "Error Code: " << errorCode << std::endl;

	LPWSTR messageBuffer;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&messageBuffer,
		0,
		NULL
	);

	std::cerr << "Error Message: " << (char*)messageBuffer << std::endl;
	LocalFree(messageBuffer);
}

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max)
{
	return val < min ? min : val > max ? max : val;
}

template<typename T>
T AlignUpWithMask(T value, size_t mask)
{
	return static_cast<T>((static_cast<size_t>(value) + mask) & ~mask);
}

template<typename T>
T AlignUp(T value, size_t align)
{
	return AlignUpWithMask(value, align == 0 ? 0 : align - 1);
}

template<typename T>
T AlignDownWithMask(T value, size_t mask)
{
	return static_cast<T>(static_cast<size_t>(value) & ~mask);
}

template<typename T>
T AlignDown(T value, size_t align)
{
	return AlignDownWithMask(value, align == 0 ? 0 : align - 1);
}