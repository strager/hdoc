// Copyright 2019-2022 hdoc
// SPDX-License-Identifier: AGPL-3.0-only

#include "common.hpp"

#include "doctest.hpp"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"

#include "indexer/Indexer.hpp"
#include "indexer/Matchers.hpp"
#include "types/Symbols.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

void runOverCode(const std::string_view code, hdoc::types::Index& index, const hdoc::types::Config cfg, std::vector<std::string> projectClangArgs) {
  clang::ast_matchers::MatchFinder          Finder;
  hdoc::indexer::matchers::FunctionMatcher  FunctionFinder(&index, &cfg);
  hdoc::indexer::matchers::RecordMatcher    RecordFinder(&index, &cfg);
  hdoc::indexer::matchers::EnumMatcher      EnumFinder(&index, &cfg);
  hdoc::indexer::matchers::NamespaceMatcher NamespaceFinder(&index, &cfg);

  Finder.addMatcher(FunctionFinder.getMatcher(), &FunctionFinder);
  Finder.addMatcher(RecordFinder.getMatcher(), &RecordFinder);
  Finder.addMatcher(EnumFinder.getMatcher(), &EnumFinder);
  Finder.addMatcher(NamespaceFinder.getMatcher(), &NamespaceFinder);

  std::unique_ptr<clang::tooling::FrontendActionFactory> Factory(clang::tooling::newFrontendActionFactory(&Finder));
  std::unique_ptr<clang::FrontendAction> ToolAction = Factory->create();

  std::string fileName = "input.cc";
  std::string codeCopy(code);  // TODO(strager): Avoid this copy.

  std::vector<std::string> args = hdoc::indexer::getArgumentAdjusterForConfig(cfg)(projectClangArgs, fileName);
  args.insert(args.begin(), "-fsyntax-only");
  args.insert(args.begin(), "index-test-tool");
  args.push_back(fileName);

  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> vfs(new llvm::vfs::InMemoryFileSystem());
  vfs->addFile(fileName, 0, llvm::MemoryBuffer::getMemBuffer(codeCopy));
  llvm::IntrusiveRefCntPtr<clang::FileManager> files(new clang::FileManager(clang::FileSystemOptions(), vfs));

  clang::tooling::ToolInvocation invocation(args, std::move(ToolAction), files.get(), std::make_shared<clang::PCHContainerOperations>());
  invocation.run();
}

void checkIndexSizes(const hdoc::types::Index& index,
                     const uint32_t            recordsSize,
                     const uint32_t            functionsSize,
                     const uint32_t            enumsSize,
                     const uint32_t            namespacesSize) {
  CHECK(index.records.entries.size() == recordsSize);
  CHECK(index.functions.entries.size() == functionsSize);
  CHECK(index.enums.entries.size() == enumsSize);
  CHECK(index.namespaces.entries.size() == namespacesSize);
}
