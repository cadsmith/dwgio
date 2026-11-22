/***********************************************************************************************************************
 *  @file      source/bits/reed.cpp
 *  @brief     The Reed-Solomon codes that guard R2007's pages.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reed.hpp"

#include <algorithm>
#include <array>

namespace DwgIO::Bits::Reed
{
namespace
{
constexpr std::size_t Order = 255;

/**
 *  The field's powers of alpha, twice over so a sum of logarithms needs no modulo, and its logarithms.
 */
struct Field
{
  std::array<std::uint8_t, 2 * Order> Powers{};
  std::array<std::size_t, Order + 1> Logs{};

  explicit Field(std::uint16_t polynomial)
  {
    constexpr unsigned Carry = 0x100;
    unsigned value = 1;
    for (std::size_t power = 0; power < Order; ++power)
    {
      Powers.at(power) = static_cast<std::uint8_t>(value);
      Powers.at(power + Order) = static_cast<std::uint8_t>(value);
      Logs.at(value) = power;
      value <<= 1U;
      if ((value & Carry) != 0)
      {
        value ^= polynomial;
      }
    }
  }

  /**
   *  The product of a value and a factor that is never zero: a power of alpha or a coefficient of the generators of
   *  these codes.
   */
  [[nodiscard]] auto Times(std::uint8_t value, std::uint8_t factor) const -> std::uint8_t
  {
    return value == 0 ? 0 : Powers.at(Logs.at(value) + Logs.at(factor));
  }
};

/**
 *  The generator's coefficients, the highest power first: the product of (x - alpha^i) for i from 1.
 */
auto Generator(const Field &field, std::size_t parity) -> std::vector<std::uint8_t>
{
  std::vector<std::uint8_t> generator{1};
  for (std::size_t root = 1; root <= parity; ++root)
  {
    std::vector<std::uint8_t> next(generator.size() + 1, 0);
    for (std::size_t at = 0; at < generator.size(); ++at)
    {
      next.at(at) ^= generator.at(at);
      next.at(at + 1) ^= field.Times(generator.at(at), field.Powers.at(root));
    }
    generator = std::move(next);
  }
  return generator;
}
} // namespace

auto Interleave(std::span<const std::byte> data, const Code &code) -> std::vector<std::byte>
{
  const Field field{code.Polynomial};
  const std::size_t parity = Codeword - code.Data;
  const std::vector<std::uint8_t> generator = Generator(field, parity);
  const std::size_t blocks = (data.size() + code.Data - 1) / code.Data;
  std::vector<std::byte> out(blocks * Codeword);
  std::vector<std::uint8_t> remainder(parity);
  for (std::size_t block = 0; block < blocks; ++block)
  {
    std::ranges::fill(remainder, 0);
    for (std::size_t at = 0; at < code.Data; ++at)
    {
      const std::size_t index = (block * code.Data) + at;
      const std::byte one = index < data.size() ? data[index] : std::byte{0};
      out[(blocks * at) + block] = one;
    }
    for (std::size_t at = code.Data; at-- > 0;) // The highest power is the block's last byte.
    {
      const std::uint8_t feedback = std::to_integer<std::uint8_t>(out[(blocks * at) + block]) ^ remainder.front();
      std::ranges::rotate(remainder, remainder.begin() + 1);
      remainder.back() = 0;
      for (std::size_t term = 0; term < parity; ++term)
      {
        remainder.at(term) ^= field.Times(feedback, generator.at(term + 1));
      }
    }
    for (std::size_t term = 0; term < parity; ++term) // The parity's highest power first, reversed in the file.
    {
      out[(blocks * (code.Data + term)) + block] = static_cast<std::byte>(remainder.at(parity - 1 - term));
    }
  }
  return out;
}
} // namespace DwgIO::Bits::Reed
