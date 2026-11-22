/***********************************************************************************************************************
 *  @file      source/file/sections/text.hpp
 *  @brief     The texts and numbers of the file's own sections.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_SECTIONS_TEXT_HPP
#define DWGIO_FILE_SECTIONS_TEXT_HPP

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "encoding/encoding.hpp"

#include "dwgio/version.hpp"

#include <cstddef>
#include <span>
#include <string>

/**
 *  In the code page below R2007 and in UTF-16 from R2007, as the summary and the file dependencies keep theirs, counted
 *  by a short of characters with the NUL that ends it, or by a long of bytes with none.
 */
namespace DwgIO::File::Sections
{
/**
 *  How a section counts its texts.
 */
enum class TextCount : std::uint8_t
{
  Ended, /**< A short of characters, the NUL after the text among them. */
  Sized, /**< A long of bytes, no NUL after the text.                   */
};

/**
 *  A section's texts and numbers read in turn.
 */
class TextReader
{
public:
  /**
   *  A reader at the section's first byte; the bytes and the scheme must outlive it.
   */
  TextReader(std::span<const std::byte> bytes, Version release, const Encoding::Scheme &text);

  /**
   *  A text counted as given; empty when its count says none.
   */
  auto Text(TextCount counted) -> std::string;
  /**
   *  An RS.
   */
  auto Short() -> std::uint16_t;
  /**
   *  An RL.
   */
  auto Long() -> std::uint32_t;
  [[nodiscard]] auto HasFailed() const -> bool;

private:
  Bits::Reader m_In;
  bool m_Wide;
  const Encoding::Scheme *m_Text;
};

/**
 *  A section's texts and numbers written in turn.
 */
class TextWriter
{
public:
  /**
   *  A writer for a section of the release; the scheme must outlive it.
   */
  TextWriter(Version release, const Encoding::Scheme &text);

  /**
   *  A text counted as given, cut to what its count holds.
   */
  void Text(const std::string &text, TextCount counted);
  void Byte(std::uint8_t value);
  /**
   *  An RS.
   */
  void Short(std::uint16_t value);
  /**
   *  An RL.
   */
  void Long(std::uint32_t value);
  auto Take() -> std::vector<std::byte>;

private:
  Bits::Writer m_Out;
  bool m_Wide;
  const Encoding::Scheme *m_Text;
};
} // namespace DwgIO::File::Sections

#endif /* DWGIO_FILE_SECTIONS_TEXT_HPP */
