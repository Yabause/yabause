#include "ygl.hh"

int * YglCache::isCached(unsigned long addr) {
	if (_cache.count(addr) > 0) {
		return _cache[addr];
	}
	return NULL;
}

void YglCache::cache(unsigned long addr, int * val) {
	_cache[addr] = val;
}

void YglCache::reset(void) {
	_cache.clear();
}
