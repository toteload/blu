#include "toteload.hh"
#include <stdio.h>
#include <stdarg.h>

namespace ttld::os {
#ifdef _WIN32
#include <Windows.h>

b32 is_sys_info_initialized = false;
SYSTEM_INFO sys_info;
u32 page_size() {
  if (!is_sys_info_initialized) {
    GetSystemInfo(&sys_info);
  }

  return sys_info.dwPageSize;
}

void *mem_reserve(usize size) {
  return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
}

b32 mem_commit(void *p, usize size) {
  void *x = VirtualAlloc(p, size, MEM_COMMIT, PAGE_READWRITE);
  return x != nullptr;
}

void mem_release(void *p, usize size) {
  VirtualFree(p, size, MEM_RELEASE);
}
#endif

#ifdef __APPLE__
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

static u32 macos_page_size = 0;

u32 page_size() {
  if (macos_page_size == 0) {
    macos_page_size = getpagesize();
  }

  return macos_page_size;
}

void *mem_reserve(usize size) {
  return mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

b32 mem_commit(void *p, usize size) {
  return 0 != mprotect(p, size, PROT_READ | PROT_WRITE);
}

void mem_release(void *p, usize size) {
  munmap(p, size);
}
#endif
}

constexpr usize default_commit_growth_size = KiB(16);

void Arena::init(usize reserve_size) {
  void *p = ttld::os::mem_reserve(reserve_size);

  reserve_end = ptr_offset(p, reserve_size);
  commit_end = p;
  at = p;
  base = p;
}

void *Arena::raw_alloc(usize byte_size, u32 align) {
  void *aligned = ptr_forward_align(at, align);
  void *at_after_alloc = ptr_offset(aligned, byte_size);

  if (at_after_alloc <= commit_end) {
    at = at_after_alloc;
    return aligned;
  }

  if (at_after_alloc >= reserve_end) {
    // TODO OOM
    return NULL;
  }

  u32 page_size = ttld::os::page_size();
  usize commit_size_needed = ptr_diff(at_after_alloc, commit_end);
  usize x = max(default_commit_growth_size, commit_size_needed);
  usize commit_size = round_up_to_power_of_two<usize>(x, page_size);

  // TODO check for error? Can this even realistically happen?
  ttld::os::mem_commit(commit_end, commit_size);

  at = at_after_alloc;
  commit_end = ptr_offset(commit_end, commit_size);

  return aligned;
}

Str Arena::push_format_string(char const *format, ...) {
  va_list vl;
  va_start(vl, format);
  i32 len = vsnprintf(nullptr, 0, format, vl);
  char *s = alloc<char>(len+1);
  vsnprintf(s, len+1, format, vl);
  va_end(vl);
  return { s, cast<u32>(len), };
}

