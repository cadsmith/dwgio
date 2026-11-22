/***********************************************************************************************************************
 *  @file      source/bits/reed.hpp
 *  @brief     The Reed-Solomon codes that guard R2007's pages.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_REED_HPP
#define DWGIO_BITS_REED_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/**
 *  The Reed-Solomon codes of chapter 5.13, which guard R2007's pages against damage: codewords of 255 bytes, the data
 *  in the order the file has it as the message's rising powers, the generator's roots alpha^1 onwards.
 */
namespace DwgIO::Bits::Reed
{
/**
 *  A code: the field's primitive polynomial and how many of a codeword's bytes are data.
 */
struct Code
{
  std::uint16_t Polynomial;
  std::size_t Data;
};

inline constexpr std::size_t Codeword = 255;
/**
 *  The data pages' code, (255, 251).
 */
inline constexpr Code Pages{.Polynomial = 0x11D, .Data = 251};
/**
 *  The system pages' and the file header's code, (255, 239).
 */
inline constexpr Code System{.Polynomial = 0x169, .Data = 239};

/**
 *  Codes `data` in as many codewords as it fills, the last padded with zeros, interleaved: codeword b's byte i at
 *  blocks * i + b.
 */
[[nodiscard]] auto Interleave(std::span<const std::byte> data, const Code &code) -> std::vector<std::byte>;
} // namespace DwgIO::Bits::Reed

#endif /* DWGIO_BITS_REED_HPP */
