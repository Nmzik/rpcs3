#pragma once

#include <d3d12.h>
#include <cassert>
#include <wrl/client.h>
#include "Utilities/Log.h"
#include "Emu/Memory/vm.h"
#include "Emu/RSX/GCM.h"


using namespace Microsoft::WRL;

// From DX12 D3D11On12 Sample (MIT Licensed)
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

/**
 * Send data to dst pointer without polluting cache.
 * Usefull to write to mapped memory from upload heap.
 */
inline
void streamToBuffer(void* dst, void* src, size_t sizeInBytes)
{
	for (int i = 0; i < sizeInBytes / 16; i++)
	{
		const __m128i &srcPtr = _mm_loadu_si128((__m128i*) ((char*)src + i * 16));
		_mm_stream_si128((__m128i*)((char*)dst + i * 16), srcPtr);
	}
}

/**
* copy src to dst pointer without polluting cache.
* Usefull to write to mapped memory from upload heap.
*/
inline
void streamBuffer(void* dst, void* src, size_t sizeInBytes)
{
	// Assume 64 bytes cache line
	int offset = 0;
	bool isAligned = !((size_t)src & 15);
	for (offset = 0; offset < sizeInBytes - 64; offset += 64)
	{
		char *line = (char*)src + offset;
		char *dstline = (char*)dst + offset;
		// prefetch next line
		_mm_prefetch(line + 16, _MM_HINT_NTA);
		__m128i srcPtr = isAligned ? _mm_load_si128((__m128i *)line) : _mm_loadu_si128((__m128i *)line);
		_mm_stream_si128((__m128i*)dstline, srcPtr);
		srcPtr = isAligned ? _mm_load_si128((__m128i *)(line + 16)) : _mm_loadu_si128((__m128i *)(line + 16));
		_mm_stream_si128((__m128i*)(dstline + 16), srcPtr);
		srcPtr = isAligned ? _mm_load_si128((__m128i *)(line + 32)) : _mm_loadu_si128((__m128i *)(line + 32));
		_mm_stream_si128((__m128i*)(dstline + 32), srcPtr);
		srcPtr = isAligned ? _mm_load_si128((__m128i *)(line + 48)) : _mm_loadu_si128((__m128i *)(line + 48));
		_mm_stream_si128((__m128i*)(dstline + 48), srcPtr);
	}
	memcpy((char*)dst + offset, (char*)src + offset, sizeInBytes - offset);
}
