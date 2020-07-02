/*
 * This file is based on
 * https://github.com/apertium/lttoolbox/blob/master/lttoolbox/compression.cc
 * and
 * https://github.com/apertium/lttoolbox/blob/master/lttoolbox/my_stdio.h
 *
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _COMPRESSION_
#define _COMPRESSION_

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unicode/unistr.h>

// Check individually all '_unlocked' functions because using
// g++-2.95 fgetc_unlocked and fputc_unlocked compile perfectly but
// fputs_unlocked is not defined in their C headers.

#if !HAVE_DECL_FPUTS_UNLOCKED
#define fputs_unlocked fputs
#endif

#if !HAVE_DECL_FGETC_UNLOCKED
#define fgetc_unlocked fgetc
#endif

#if !HAVE_DECL_FPUTC_UNLOCKED
#define fputc_unlocked fputc
#endif

#if !HAVE_DECL_FWRITE_UNLOCKED
#define fwrite_unlocked fwrite
#endif

#if !HAVE_DECL_FREAD_UNLOCKED
#define fread_unlocked fread
#endif

#if !HAVE_DECL_FGETWC_UNLOCKED
#define fgetwc_unlocked fgetwc
#endif

#if !HAVE_DECL_FPUTWC_UNLOCKED
#define fputwc_unlocked fputwc
#endif

#if !HAVE_DECL_FPUTWS_UNLOCKED
#define fputws_unlocked fputws
#endif

using namespace std;

constexpr char HEADER_TRANSDUCER[4]{'F', 'S', 'N', 'T'};
enum TD_FEATURES : uint64_t {
  TDF_WEIGHTS = (1ull << 0),
  TDF_UNKNOWN = (1ull << 1), // Features >= this are unknown, so throw an error; Inc this if more features are added
  TDF_RESERVED = (1ull << 63), // If we ever reach this many feature flags, we need a flag to know how to extend beyond 64 bits
};


inline auto write_u64(FILE *out, uint64_t value) {
  auto rv = fwrite(reinterpret_cast<const char*>(&value), 1, sizeof(value), out);
  if (rv != sizeof(value)) {
    throw std::runtime_error("Failed to write uint64_t");
  }
  return rv;
}

inline auto& write_u64(std::ostream& out, uint64_t value) {
  return out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename Stream, typename Value>
inline auto write_u64_le(Stream& out, const Value& value) {
  uint64_t v = static_cast<uint64_t>(value);
  v =
    ((v & 0xFF) << 56) |
    ((v & 0xFF00) << 40) |
    ((v & 0xFF0000) << 24) |
    ((v & 0xFF000000) << 8) |
    ((v & 0xFF00000000) >> 8) |
    ((v & 0xFF0000000000) >> 24) |
    ((v & 0xFF000000000000) >> 40) |
    ((v & 0xFF00000000000000) >> 56)
  ;
  return write_u64(out, v);
}

template<typename Stream>
inline auto write_le(Stream& out, uint64_t value) {
  return write_u64_le(out, value);
}


inline auto read_u64(FILE *in) {
  uint64_t value = 0;
  if (fread(reinterpret_cast<char*>(&value), 1, sizeof(value), in) != sizeof(value)) {
    throw std::runtime_error("Failed to read uint64_t");
  }
  return value;
}

inline auto read_u64(std::istream& in) {
  uint64_t value = 0;
  in.read(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

template<typename Stream>
inline auto read_u64_le(Stream& in) {
  uint64_t v = read_u64(in);
  v =
    ((v & 0xFF00000000000000) >> 56) |
    ((v & 0xFF000000000000) >> 40) |
    ((v & 0xFF0000000000) >> 24) |
    ((v & 0xFF00000000) >> 8) |
    ((v & 0xFF000000) << 8) |
    ((v & 0xFF0000) << 24) |
    ((v & 0xFF00) << 40) |
    ((v & 0xFF) << 56)
  ;
  return v;
}

template<typename Stream>
inline auto read_le(Stream& in, uint64_t) {
  return read_u64_le(in);
}

template<typename Value = void>
inline auto read_le(FILE *in) {
  return read_le(in, Value{});
}

template<typename Value = void>
inline auto read_le(std::istream& in) {
  return read_le(in, Value{});
}

/**
 * Clase "Compression".
 * Class methods to access compressed data by the byte-aligned method
 */
class Compression
{
private:
  /**
   * Writing a byte
   * @param byte char to write.
   * @param output output stream.
   */
  static void writeByte(unsigned char byte, FILE *output);

  /**
   * Readinging a byte
   * @param input input stream
   * @return the value of the next byte in the input
   */
  static unsigned char readByte(FILE *input);

public:
  /**
   * Encodes an integer value and writes it into the output stream
   * @see multibyte_read()
   * @param value integer to write.
   * @param output output stream.
   */
  static void multibyte_write(unsigned int value, FILE *output);

  /**
   * Encodes an integer value and writes it into the output stream
   * @see multibyte_read()
   * @param value integer to write.
   * @param output output stream.
   */
  static void multibyte_write(unsigned int value, ostream &os);

  /**
   * Read and decode an integer from the input stream.
   * @see multibyte_read()
   * @param input input stream.
   * @return the integer value readed.
   */
  static unsigned int multibyte_read(FILE *input);

  /**
   * Read and decode an integer from the input stream.
   * @see multibyte_read()
   * @param input input stream.
   * @return the integer value readed.
   */
  static unsigned int multibyte_read(istream &is);

  /**
   * This method allows to write a string to an output stream
   * using its UCSencoding as integer.
   * @see string_read()
   * @param str the string to write.
   * @param output the output stream.
   */
  static void string_write(UnicodeString const &str, FILE *output);

  /**
   * This method reads a string from the input stream.
   * @see string_write()
   * @param input the input stream.
   * @return the string read.
   */
  static UnicodeString string_read(FILE *input);

  /**
   * Encodes a double value and writes it into the output stream
   * @see long_multibyte_read()
   * @param value double to write.
   * @param output output stream.
   */
  static void long_multibyte_write(const double& value, FILE *output);

  /**
   * Encodes a double value and writes it into the output stream
   * @see long_multibyte_read()
   * @param value double to write.
   * @param output output stream.
   */
  static void long_multibyte_write(const double& value, ostream &os);

  /**
   * Read and decode a double from the input stream.
   * @see long_multibyte_read()
   * @param input input stream.
   * @return the double value read.
   */
  static double long_multibyte_read(FILE *input);

  /**
   * Read and decode a double from the input stream.
   * @see long_multibyte_read()
   * @param input input stream.
   * @return the double value read.
   */
  static double long_multibyte_read(istream &is);
};

#endif
