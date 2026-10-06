//===- WASI/Process.inc - WASI Process support ------------------*- C++ -*-===//
//
// Browser-hosted WASI has no process table, job control, or terminal ioctls.
// Keep the in-process portions of llvm::sys::Process functional and make the
// unavailable operating-system facilities explicit.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Errno.h"
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

using namespace llvm;
using namespace sys;

Process::Pid Process::getProcessId() {
  return static_cast<Pid>(::getpid());
}

Expected<unsigned> Process::getPageSize() {
  return 64 * 1024;
}

size_t Process::GetMallocUsage() { return 0; }

void Process::GetTimeUsage(TimePoint<> &Elapsed,
                           std::chrono::nanoseconds &UserTime,
                           std::chrono::nanoseconds &SysTime) {
  Elapsed = std::chrono::system_clock::now();
  UserTime = std::chrono::nanoseconds::zero();
  SysTime = std::chrono::nanoseconds::zero();
}

void Process::PreventCoreFiles() { coreFilesPrevented = true; }

std::optional<std::string> Process::GetEnv(StringRef Name) {
  std::string NameStr = Name.str();
  if (const char *Value = ::getenv(NameStr.c_str()))
    return std::string(Value);
  return std::nullopt;
}

std::error_code Process::FixupStandardFileDescriptors() {
  return std::error_code();
}

std::error_code Process::SafelyCloseFileDescriptor(int FD) {
  if (::close(FD) < 0)
    return errnoAsErrorCode();
  return std::error_code();
}

bool Process::StandardInIsUserInput() { return FileDescriptorIsDisplayed(0); }
bool Process::StandardOutIsDisplayed() { return FileDescriptorIsDisplayed(1); }
bool Process::StandardErrIsDisplayed() { return FileDescriptorIsDisplayed(2); }

bool Process::FileDescriptorIsDisplayed(int FD) {
#if HAVE_ISATTY
  return ::isatty(FD);
#else
  return false;
#endif
}

static unsigned getColumns() {
  if (const char *ColumnsStr = std::getenv("COLUMNS")) {
    int Columns = std::atoi(ColumnsStr);
    if (Columns > 0)
      return static_cast<unsigned>(Columns);
  }
  return 0;
}

unsigned Process::StandardOutColumns() {
  return StandardOutIsDisplayed() ? getColumns() : 0;
}
unsigned Process::StandardErrColumns() {
  return StandardErrIsDisplayed() ? getColumns() : 0;
}

static bool terminalHasColors() {
  if (const char *TermStr = std::getenv("TERM"))
    return StringSwitch<bool>(TermStr)
        .Case("ansi", true)
        .Case("linux", true)
        .StartsWith("screen", true)
        .StartsWith("xterm", true)
        .StartsWith("vt100", true)
        .StartsWith("rxvt", true)
        .EndsWith("color", true)
        .Default(false);
  return false;
}

bool Process::FileDescriptorHasColors(int FD) {
  return FileDescriptorIsDisplayed(FD) && terminalHasColors();
}
bool Process::StandardOutHasColors() { return FileDescriptorHasColors(1); }
bool Process::StandardErrHasColors() { return FileDescriptorHasColors(2); }
void Process::UseANSIEscapeCodes(bool) {}
bool Process::ColorNeedsFlush() { return false; }

const char *Process::OutputColor(char Code, bool Bold, bool BG) {
  return colorcodes[BG ? 1 : 0][Bold ? 1 : 0][Code & 15];
}
const char *Process::OutputBold(bool) { return "\033[1m"; }
const char *Process::OutputReverse() { return "\033[7m"; }
const char *Process::ResetColor() { return "\033[0m"; }

unsigned Process::GetRandomNumber() {
  static int Seeded =
      (static_cast<void>(::srand(static_cast<unsigned>(
           std::chrono::high_resolution_clock::now().time_since_epoch().count() ^
           static_cast<unsigned>(::getpid())))),
       0);
  (void)Seeded;
  return static_cast<unsigned>(::rand());
}

[[noreturn]] void Process::ExitNoCleanup(int RetCode) { ::_Exit(RetCode); }
