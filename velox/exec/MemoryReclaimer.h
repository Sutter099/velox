/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "velox/common/base/Exceptions.h"
#include "velox/common/base/Portability.h"
#include "velox/common/memory/MemoryArbitrator.h"

namespace facebook::velox::exec {
/// Provides the default memory reclaimer implementation for velox task
/// execution.
class MemoryReclaimer : public memory::MemoryReclaimer {
 public:
  virtual ~MemoryReclaimer() = default;

  static std::unique_ptr<memory::MemoryReclaimer> create();

  void enterArbitration() override;

  void leaveArbitration() noexcept override;

  void abort(memory::MemoryPool* pool, const std::exception_ptr& error)
      override;

 protected:
  MemoryReclaimer() = default;
};

/// Callback used by memory arbitration to check if a driver thread under memory
/// arbitration has been put in suspension state. This is to prevent arbitration
/// deadlock as the arbitrator might reclaim memory from the task of the driver
/// thread which is under arbitration. The task reclaim needs to wait for the
/// drivers to go off thread. A suspended driver thread is not counted as
/// running.
void memoryArbitrationStateCheck(memory::MemoryPool& pool);

/// The object is used to set/clear non-reclaimable section of an operation in
/// the middle of its execution. It allows the memory arbitrator to reclaim
/// memory from a running operator which is waiting for memory arbitration.
/// 'nonReclaimableSection' points to the corresponding flag of the associated
/// operator.
class ReclaimableSectionGuard {
 public:
  explicit ReclaimableSectionGuard(tsan_atomic<bool>* nonReclaimableSection)
      : nonReclaimableSection_(nonReclaimableSection),
        oldNonReclaimableSectionValue_(*nonReclaimableSection_) {
    *nonReclaimableSection_ = false;
  }

  ~ReclaimableSectionGuard() {
    *nonReclaimableSection_ = oldNonReclaimableSectionValue_;
  }

 private:
  tsan_atomic<bool>* const nonReclaimableSection_;
  const bool oldNonReclaimableSectionValue_;
};

class NonReclaimableSectionGuard {
 public:
  explicit NonReclaimableSectionGuard(tsan_atomic<bool>* nonReclaimableSection)
      : nonReclaimableSection_(nonReclaimableSection),
        oldNonReclaimableSectionValue_(*nonReclaimableSection_) {
    *nonReclaimableSection_ = true;
  }

  ~NonReclaimableSectionGuard() {
    *nonReclaimableSection_ = oldNonReclaimableSectionValue_;
  }

 private:
  tsan_atomic<bool>* const nonReclaimableSection_;
  const bool oldNonReclaimableSectionValue_;
};
} // namespace facebook::velox::exec
