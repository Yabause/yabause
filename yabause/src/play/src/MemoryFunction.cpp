#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include "MemoryFunction.h"

#ifdef WIN32

#include <windows.h>

#elif defined(__APPLE__)

#include "TargetConditionals.h"
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <libkern/OSCacheControl.h>

#elif defined(__ANDROID__) || defined(__linux__) || defined(__FreeBSD__)

#include <sys/mman.h>

#endif

CMemoryFunction::CMemoryFunction()
: m_code(nullptr)
, m_size(0)
{

}

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
: m_code(nullptr)
{
#ifdef WIN32
	m_size = size;
	m_code = malloc(size);
	memcpy(m_code, code, size);
	
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_code, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	assert(result == TRUE);
#elif defined(__APPLE__)
	vm_size_t page_size = 0;
	host_page_size(mach_task_self(), &page_size);
	unsigned int allocSize = ((size + page_size - 1) / page_size) * page_size;
	vm_allocate(mach_task_self(), reinterpret_cast<vm_address_t*>(&m_code), allocSize, TRUE); 
	memcpy(m_code, code, size);
	sys_icache_invalidate(m_code, size);
	kern_return_t result = vm_protect(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), size, 0, VM_PROT_READ | VM_PROT_EXECUTE);
	assert(result == 0);
	m_size = allocSize;
#elif defined(__ANDROID__) || defined(__linux__) || defined(__FreeBSD__)
	m_size = size;
	m_code = mmap(nullptr, size, PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(m_code != MAP_FAILED);
	memcpy(m_code, code, size);
#if defined(__arm__) || defined(__aarch64__)
	__clear_cache(m_code, reinterpret_cast<uint8*>(m_code) + size);
#endif
#endif
}

CMemoryFunction::~CMemoryFunction()
{
	Reset();
}

void CMemoryFunction::Reset()
{
	if(m_code != nullptr)
	{
#ifdef WIN32
		free(m_code);
#elif defined(__APPLE__)
		vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), m_size);
#elif defined(__ANDROID__) || defined(__linux__) || defined(__FreeBSD__)
		munmap(m_code, m_size);
#endif
	}
	m_code = nullptr;
	m_size = 0;
}

bool CMemoryFunction::IsEmpty() const
{
	return m_code == nullptr;
}

CMemoryFunction& CMemoryFunction::operator =(CMemoryFunction&& rhs)
{
	Reset();
	std::swap(m_code, rhs.m_code);
	std::swap(m_size, rhs.m_size);
	return (*this);
}

void CMemoryFunction::operator()(void* context)
{
	typedef void (*FctType)(void*);
	auto fct = reinterpret_cast<FctType>(m_code);
	fct(context);
}

void* CMemoryFunction::GetCode() const
{
	return m_code;
}

size_t CMemoryFunction::GetSize() const
{
	return m_size;
}
