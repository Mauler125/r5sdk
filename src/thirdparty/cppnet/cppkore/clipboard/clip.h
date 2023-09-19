// Clip Library
// Copyright (c) 2015-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CLIP_H_INCLUDED
#define CLIP_H_INCLUDED
#pragma once

#include <cassert>
#include <memory>
#include <string>

namespace clip {

  // ======================================================================
  // Low-level API to lock the clipboard/pasteboard and modify it
  // ======================================================================

  // Clipboard format identifier.
  typedef size_t format;

  struct image_spec;

  class lock {
  public:
    // You can give your current HWND as the "native_window_handle."
    // Windows clipboard functions use this handle to open/close
    // (lock/unlock) the clipboard. From the MSDN documentation we
    // need this handler so SetClipboardData() doesn't fail after a
    // EmptyClipboard() call. Anyway it looks to work just fine if we
    // call OpenClipboard() with a null HWND.
    lock(void* native_window_handle = nullptr);
    ~lock();

    // Returns true if we've locked the clipboard successfully in
    // lock() constructor.
    bool locked() const;

    // Clears the clipboard content. If you don't clear the content,
    // previous clipboard content (in unknown formats) could persist
    // after the unlock.
    bool clear();

    // Returns true if the clipboard can be converted to the given
    // format.
    bool is_convertible(format f) const;
    bool set_data(format f, const char* buf, size_t len);
    bool get_data(format f, char* buf, size_t len) const;
    size_t get_data_length(format f) const;

  private:
    class impl;
    std::unique_ptr<impl> p;
  };

  format register_format(const std::string& name);

  // This format is when the clipboard has no content.
  format empty_format();

  // When the clipboard has UTF8 text.
  format text_format();

  // When the clipboard has an image.
  format image_format();

  // Returns true if the clipboard has content of the given type.
  bool has(format f);

  // Clears the clipboard content.
  bool clear();

  // ======================================================================
  // Error handling
  // ======================================================================

  enum class ErrorCode {
    CannotLock,
    ImageNotSupported,
  };

  typedef void (*error_handler)(ErrorCode code);

  void set_error_handler(error_handler f);
  error_handler get_error_handler();

  // ======================================================================
  // Text
  // ======================================================================

  // High-level API to put/get UTF8 text in/from the clipboard. These
  // functions returns false in case of error.
  bool set_text(const std::string& value);
  bool get_text(std::string& value);

} // namespace clip

#endif // CLIP_H_INCLUDED
