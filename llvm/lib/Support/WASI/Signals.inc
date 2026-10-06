//===- WASI/Signals.inc - WASI signal support -------------------*- C++ -*-===//
//
// Core WASI has no asynchronous POSIX signals. LLVM can still register and run
// cleanup callbacks explicitly, but signal installation and stack unwinding are
// unavailable in a browser-hosted compiler.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Process.h"

static void (*WASIInterruptFunction)() = nullptr;
static void (*WASIInfoFunction)() = nullptr;
static void (*WASIOneShotPipeFunction)() = nullptr;

void sys::unregisterHandlers() {}

void sys::CleanupOnSignal(uintptr_t) {
  sys::RunSignalHandlers();
  if (WASIInterruptFunction) {
    auto *Fn = WASIInterruptFunction;
    WASIInterruptFunction = nullptr;
    Fn();
  }
}

void sys::RunInterruptHandlers() {
  if (WASIInterruptFunction) {
    auto *Fn = WASIInterruptFunction;
    WASIInterruptFunction = nullptr;
    Fn();
  }
}

void llvm::sys::SetInterruptFunction(void (*IF)()) {
  WASIInterruptFunction = IF;
}
void llvm::sys::SetInfoSignalFunction(void (*Handler)()) {
  WASIInfoFunction = Handler;
}
void llvm::sys::SetOneShotPipeSignalFunction(void (*Handler)()) {
  WASIOneShotPipeFunction = Handler;
}
void llvm::sys::DefaultOneShotPipeSignalHandler() {
  Process::Exit(1);
}

bool llvm::sys::RemoveFileOnSignal(StringRef, std::string *) {
  return false;
}
void llvm::sys::DontRemoveFileOnSignal(StringRef) {}

void llvm::sys::AddSignalHandler(sys::SignalHandlerCallback FnPtr,
                                 void *Cookie) {
  insertSignalHandler(FnPtr, Cookie);
}

static bool findModulesAndOffsets(void **, int, const char **, intptr_t *,
                                  const char *, StringSaver &) {
  return false;
}
static bool printMarkupContext(llvm::raw_ostream &, const char *) {
  return false;
}

void llvm::sys::PrintStackTrace(raw_ostream &, int) {}
void llvm::sys::DisableSystemDialogsOnCrash() {}
void llvm::sys::PrintStackTraceOnErrorSignal(StringRef, bool) {}
