/***********************************************************************************************************************
 *  @file      source/version.cpp
 *  @brief     The names, $ACADVER and maintenance versions of each release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/version.hpp"

#include <array>
#include <utility>

namespace DwgIO
{
namespace
{
struct Step
{
  Version Release{};
  std::string_view Acadver;
  std::string_view Name;
  int Maintenance{};
};

constexpr std::array Ladder{
    Step{.Release = Version::R14, .Acadver = "AC1014", .Name = "R14", .Maintenance = 10},
    Step{.Release = Version::R2000, .Acadver = "AC1015", .Name = "R2000", .Maintenance = 15},
    Step{.Release = Version::R2004, .Acadver = "AC1018", .Name = "R2004", .Maintenance = 104},
    Step{.Release = Version::R2007, .Acadver = "AC1021", .Name = "R2007", .Maintenance = 50},
    Step{.Release = Version::R2010, .Acadver = "AC1024", .Name = "R2010", .Maintenance = 226},
    Step{.Release = Version::R2013, .Acadver = "AC1027", .Name = "R2013", .Maintenance = 125},
    Step{.Release = Version::R2018, .Acadver = "AC1032", .Name = "R2018", .Maintenance = 0},
};

auto At(Version release) -> const Step &
{
  return Ladder.at(std::to_underlying(release));
}
} // namespace

auto AcadverOf(Version release) -> std::string_view
{
  return At(release).Acadver;
}

auto NameOf(Version release) -> std::string_view
{
  return At(release).Name;
}

auto MaintenanceOf(Version release) -> int
{
  return At(release).Maintenance;
}

auto ReleaseOf(std::string_view code) -> std::optional<Version>
{
  for (const Step &one : Ladder)
  {
    if (one.Acadver == code)
    {
      return one.Release;
    }
  }
  return std::nullopt;
}
} // namespace DwgIO
