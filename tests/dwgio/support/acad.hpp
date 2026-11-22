/***********************************************************************************************************************
 *  @file      tests/dwgio/support/acad.hpp
 *  @brief     AutoCAD itself as a test oracle: its audit and its own save of what dwgio wrote, when asked for.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_ACAD_HPP
#define DWGIO_TESTS_SUPPORT_ACAD_HPP

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO
{
/**
 *  Whether the tests may ask AutoCAD: DWGIO_ACAD_VERIFY is set in the environment.
 */
inline auto IsVerifying() -> bool
{
  return std::getenv("DWGIO_ACAD_VERIFY") != nullptr; // NOLINT(concurrency-mt-unsafe): the tests run on one thread
}

/**
 *  The AutoCAD application: DWGIO_ACAD in the environment, as tools/acad/console.sh reads it, else AutoCAD 2026's place
 *  on macOS.
 */
inline auto FindApplication() -> std::filesystem::path
{
  const char *set = std::getenv("DWGIO_ACAD"); // NOLINT(concurrency-mt-unsafe): the tests run on one thread
  return set != nullptr ? std::filesystem::path{set}
                        : std::filesystem::path{"/Applications/Autodesk/AutoCAD 2026/AutoCAD 2026.app"};
}

/**
 *  Where the application keeps its sample drawings.
 */
inline auto FindResources() -> std::filesystem::path
{
  return FindApplication() / "Contents" / "Resources";
}

/**
 *  Whether AutoCAD's sample drawings are there to test against.
 */
inline auto HasSamples() -> bool
{
  return std::filesystem::is_directory(FindResources());
}

/**
 *  Writes bytes to a file under the temporary directory by the name given, and gives its path.
 */
inline auto WriteTemporary(std::span<const std::byte> bytes, const std::string &name) -> std::filesystem::path
{
  const std::filesystem::path file = std::filesystem::temp_directory_path() / name;
  std::ofstream out{file, std::ios::binary};
  out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return file;
}

/**
 *  Runs an AutoCAD script on a drawing through tools/acad/console.sh, under the lock of tools/acad/locked.py so that
 *  one AutoCAD runs at a time. Its output is dropped: the script's saves are what the caller reads.
 */
inline void RunScript(const std::filesystem::path &script, const std::filesystem::path &drawing)
{
  const std::filesystem::path tools = std::filesystem::path{DWGIO_SOURCE_DIR} / "tools" / "acad";
  const std::string command = "python3 '" + (tools / "locked.py").string() + "' sh '" +
                              (tools / "console.sh").string() + "' '" + script.string() + "' '" + drawing.string() +
                              "' > /dev/null 2>&1";
  std::system(command.c_str()); // NOLINT(concurrency-mt-unsafe,cert-env33-c): the test asks AutoCAD
}

/**
 *  Runs AutoCAD's AUDIT on saved bytes through tools/acad/verify.py, writing them under the temporary directory by the
 *  name given: true when AutoCAD finds nothing to fix. Only when IsVerifying().
 */
inline auto PassesAudit(std::span<const std::byte> bytes, const std::string &name) -> bool
{
  const std::filesystem::path file = WriteTemporary(bytes, name);
  const std::filesystem::path verify = std::filesystem::path{DWGIO_SOURCE_DIR} / "tools" / "acad" / "verify.py";
  const std::string command = "python3 '" + verify.string() + "' '" + file.string() + "'";
  return std::system(command.c_str()) == 0; // NOLINT(concurrency-mt-unsafe,cert-env33-c): the test asks AutoCAD
}

/**
 *  A drawing a case saved, waiting for AutoCAD's AUDIT.
 */
struct Queued
{
  std::string Name{};
  std::vector<std::byte> Bytes{};
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the cases of a file fill it, its last empties it
inline std::vector<Queued> AuditQueue;

/**
 *  Keeps saved bytes for the AUDIT case that closes the file, by the name AutoCAD's copy takes; nothing when
 *  DWGIO_ACAD_VERIFY is unset, so that the AUDIT shows as skipped rather than passed.
 */
inline void QueueAudit(std::span<const std::byte> bytes, std::string name)
{
  if (IsVerifying())
  {
    AuditQueue.push_back(Queued{.Name = std::move(name), .Bytes = {bytes.begin(), bytes.end()}});
  }
}

/**
 *  Runs AutoCAD's AUDIT on every drawing the cases before it queued, then empties the queue. A file's last case,
 *  skipped unless IsVerifying(); the queue is empty when that case runs alone.
 */
inline void CheckQueuedAudits()
{
  REQUIRE_MESSAGE(!AuditQueue.empty(), "nothing queued: run the file's other cases first");
  for (const Queued &queued : AuditQueue)
  {
    CHECK_MESSAGE(PassesAudit(queued.Bytes, queued.Name), queued.Name);
  }
  AuditQueue.clear();
}

/**
 *  AutoCAD's own save of saved bytes as R2000 after its AUDIT, writing under the temporary directory by the name given;
 *  empty when AutoCAD could not open them. Only when IsVerifying().
 */
inline auto Resave(std::span<const std::byte> bytes, const std::string &name) -> std::vector<std::byte>
{
  const std::filesystem::path file = WriteTemporary(bytes, name + ".dwg");
  const std::filesystem::path back = std::filesystem::temp_directory_path() / (name + "-back.dwg");
  const std::filesystem::path script = std::filesystem::temp_directory_path() / (name + ".scr");
  {
    std::ofstream lines{script};
    lines << "FILEDIA 0\nCMDECHO 0\n_.AUDIT _Y\n_.SAVEAS 2000 \"" << back.string() << "\"\n_.QUIT _Y\n";
  }
  std::filesystem::remove(back);
  RunScript(script, file);
  std::ifstream in{back, std::ios::binary};
  const std::string read{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
  const std::span<const std::byte> whole = std::as_bytes(std::span{read});
  return {whole.begin(), whole.end()};
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_ACAD_HPP */
