// Clip Library
// Copyright (C) 2015-2020  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "../stdafx.h"
#include "clip.h"
#include "clip_lock_impl.h"
#include <vector>

#ifndef LCS_WINDOWS_COLOR_SPACE
#define LCS_WINDOWS_COLOR_SPACE 'Win '
#endif

#ifndef CF_DIBV5
#define CF_DIBV5            17
#endif

namespace clip {

namespace {

// Data type used as header for custom formats to indicate the exact
// size of the user custom data. This is necessary because it looks
// like GlobalSize() might not return the exact size, but a greater
// value.
typedef uint64_t CustomSizeT;

unsigned long get_shift_from_mask(unsigned long mask) {
  unsigned long shift = 0;
  for (shift=0; shift<sizeof(unsigned long)*8; ++shift)
    if (mask & (1 << shift))
      return shift;
  return shift;
}

class Hglobal {
public:
  Hglobal() : m_handle(nullptr) {
  }

  explicit Hglobal(HGLOBAL handle) : m_handle(handle) {
  }

  explicit Hglobal(size_t len) : m_handle(GlobalAlloc(GHND, len)) {
  }

  ~Hglobal() {
    if (m_handle)
      GlobalFree(m_handle);
  }

  void release() {
    m_handle = nullptr;
  }

  operator HGLOBAL() {
    return m_handle;
  }

private:
  HGLOBAL m_handle;
};

}

lock::impl::impl(void* hwnd) : m_locked(false) {
  for (int i=0; i<5; ++i) {
    if (OpenClipboard((HWND)hwnd)) {
      m_locked = true;
      break;
    }
    Sleep(20);
  }

  if (!m_locked) {
    error_handler e = get_error_handler();
    if (e)
      e(ErrorCode::CannotLock);
  }
}

lock::impl::~impl() {
  if (m_locked)
    CloseClipboard();
}

bool lock::impl::clear() {
  return (EmptyClipboard() ? true: false);
}

bool lock::impl::is_convertible(format f) const {
  if (f == text_format()) {
    return
      (IsClipboardFormatAvailable(CF_TEXT) ||
       IsClipboardFormatAvailable(CF_UNICODETEXT) ||
       IsClipboardFormatAvailable(CF_OEMTEXT));
  }
  else if (f == image_format()) {
    return (IsClipboardFormatAvailable(CF_DIB) ? true: false);
  }
  else if (IsClipboardFormatAvailable(f))
    return true;
  else
    return false;
}

bool lock::impl::set_data(format f, const char* buf, size_t len) {
  bool result = false;

  if (f == text_format()) {
    if (len > 0) {
      int reqsize = MultiByteToWideChar(CP_UTF8, 0, buf, len, NULL, 0);
      if (reqsize > 0) {
        ++reqsize;

        Hglobal hglobal(sizeof(WCHAR)*reqsize);
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        MultiByteToWideChar(CP_UTF8, 0, buf, len, lpstr, reqsize);
        GlobalUnlock(hglobal);

        result = (SetClipboardData(CF_UNICODETEXT, hglobal)) ? true: false;
        if (result)
          hglobal.release();
      }
    }
  }
  else {
    Hglobal hglobal(len+sizeof(CustomSizeT));
    if (hglobal) {
      auto dst = (uint8_t*)GlobalLock(hglobal);
      if (dst) {
        *((CustomSizeT*)dst) = len;
        memcpy(dst+sizeof(CustomSizeT), buf, len);
        GlobalUnlock(hglobal);
        result = (SetClipboardData(f, hglobal) ? true: false);
        if (result)
          hglobal.release();
      }
    }
  }

  return result;
}

bool lock::impl::get_data(format f, char* buf, size_t len) const {
  assert(buf);

  if (!buf || !is_convertible(f))
    return false;

  bool result = false;

  if (f == text_format()) {
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
      if (hglobal) {
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        if (lpstr) {
          size_t reqsize =
            WideCharToMultiByte(CP_UTF8, 0, lpstr, -1,
                                nullptr, 0, nullptr, nullptr);

          assert(reqsize <= len);
          if (reqsize <= len) {
            WideCharToMultiByte(CP_UTF8, 0, lpstr, -1,
                                buf, reqsize, nullptr, nullptr);
            result = true;
          }
          GlobalUnlock(hglobal);
        }
      }
    }
    else if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_TEXT);
      if (hglobal) {
        LPSTR lpstr = static_cast<LPSTR>(GlobalLock(hglobal));
        if (lpstr) {
          // TODO check length
          memcpy(buf, lpstr, len);
          result = true;
          GlobalUnlock(hglobal);
        }
      }
    }
  }
  else {
    if (IsClipboardFormatAvailable(f)) {
      HGLOBAL hglobal = GetClipboardData(f);
      if (hglobal) {
        const SIZE_T total_size = GlobalSize(hglobal);
        auto ptr = (const uint8_t*)GlobalLock(hglobal);
        if (ptr) {
          CustomSizeT reqsize = *((CustomSizeT*)ptr);

          // If the registered length of data in the first CustomSizeT
          // number of bytes of the hglobal data is greater than the
          // GlobalSize(hglobal), something is wrong, it should not
          // happen.
          assert(reqsize <= total_size);
          if (reqsize > total_size)
            reqsize = total_size - sizeof(CustomSizeT);

          if (reqsize <= len) {
            memcpy(buf, ptr+sizeof(CustomSizeT), reqsize);
            result = true;
          }
          GlobalUnlock(hglobal);
        }
      }
    }
  }

  return result;
}

size_t lock::impl::get_data_length(format f) const {
  size_t len = 0;

  if (f == text_format()) {
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
      if (hglobal) {
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        if (lpstr) {
          len =
            WideCharToMultiByte(CP_UTF8, 0, lpstr, -1,
                                nullptr, 0, nullptr, nullptr);
          GlobalUnlock(hglobal);
        }
      }
    }
    else if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_TEXT);
      if (hglobal) {
        LPSTR lpstr = (LPSTR)GlobalLock(hglobal);
        if (lpstr) {
          len = strlen(lpstr) + 1;
          GlobalUnlock(hglobal);
        }
      }
    }
  }
  else if (f != empty_format()) {
    if (IsClipboardFormatAvailable(f)) {
      HGLOBAL hglobal = GetClipboardData(f);
      if (hglobal) {
        const SIZE_T total_size = GlobalSize(hglobal);
        auto ptr = (const uint8_t*)GlobalLock(hglobal);
        if (ptr) {
          len = *((CustomSizeT*)ptr);

          assert(len <= total_size);
          if (len > total_size)
            len = total_size - sizeof(CustomSizeT);

          GlobalUnlock(hglobal);
        }
      }
    }
  }

  return len;
}

format register_format(const std::string& name) {
  int reqsize = 1+MultiByteToWideChar(CP_UTF8, 0,
                                      name.c_str(), name.size(), NULL, 0);
  std::vector<WCHAR> buf(reqsize);
  MultiByteToWideChar(CP_UTF8, 0, name.c_str(), name.size(),
                      &buf[0], reqsize);

  // From MSDN, registered clipboard formats are identified by values
  // in the range 0xC000 through 0xFFFF.
  return (format)RegisterClipboardFormatW(&buf[0]);
}

} // namespace clip
