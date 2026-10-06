//===- WASI/Program.inc - WASI Program support ------------------*- C++ -*-===//
//
// Browser WASI cannot create child processes. Keep path discovery and ordinary
// file output available while reporting process execution as unsupported.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace sys;

ProcessInfo::ProcessInfo() : Pid(0), Process(0), ReturnCode(0) {}

ErrorOr<std::string> sys::findProgramByName(StringRef Name,
                                            ArrayRef<StringRef> Paths) {
  assert(!Name.empty() && "Must have a name!");
  if (Name.contains('/'))
    return std::string(Name);

  SmallVector<StringRef, 16> EnvironmentPaths;
  if (Paths.empty())
    if (const char *PathEnv = std::getenv("PATH")) {
      SplitString(PathEnv, EnvironmentPaths, ":");
      Paths = EnvironmentPaths;
    }

  for (StringRef Path : Paths) {
    if (Path.empty())
      continue;
    SmallString<128> FilePath(Path);
    sys::path::append(FilePath, Name);
    if (sys::fs::exists(FilePath))
      return std::string(FilePath);
  }
  return errc::no_such_file_or_directory;
}

static bool Execute(ProcessInfo &PI, StringRef, ArrayRef<StringRef>,
                    std::optional<ArrayRef<StringRef>>,
                    ArrayRef<std::optional<StringRef>>, unsigned,
                    std::string *ErrMsg, BitVector *, bool) {
  PI = ProcessInfo();
  if (ErrMsg)
    *ErrMsg = "subprocess execution is not supported by WASI";
  return false;
}

ProcessInfo llvm::sys::Wait(const ProcessInfo &, std::optional<unsigned>,
                            std::string *ErrMsg,
                            std::optional<ProcessStatistics> *ProcStat, bool) {
  ProcessInfo Result;
  Result.ReturnCode = -1;
  if (ErrMsg)
    *ErrMsg = "subprocess waiting is not supported by WASI";
  if (ProcStat)
    ProcStat->reset();
  return Result;
}

std::error_code llvm::sys::ChangeStdinMode(fs::OpenFlags) {
  return std::error_code();
}
std::error_code llvm::sys::ChangeStdoutMode(fs::OpenFlags) {
  return std::error_code();
}
std::error_code llvm::sys::ChangeStdinToBinary() {
  return std::error_code();
}
std::error_code llvm::sys::ChangeStdoutToBinary() {
  return std::error_code();
}

std::error_code
llvm::sys::writeFileWithEncoding(StringRef FileName, StringRef Contents,
                                 WindowsEncodingMethod) {
  std::error_code EC;
  raw_fd_ostream OS(FileName, EC);
  if (EC)
    return EC;
  OS << Contents;
  if (OS.has_error())
    return make_error_code(errc::io_error);
  return std::error_code();
}

bool llvm::sys::commandLineFitsWithinSystemLimits(StringRef,
                                                  ArrayRef<StringRef>) {
  return true;
}
