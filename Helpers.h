#pragma once

#define WIN32_LEAN_AND_MEAN
#include <exception>
#include <Windows.h>

inline void ThrowIfFailed(HRESULT hr)
{
	if(FAILED(hr))
	{
		throw std::exception();
	}
}