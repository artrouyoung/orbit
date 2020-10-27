// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_SERVICE_LINUX_TRACING_HANDLER_H_
#define ORBIT_SERVICE_LINUX_TRACING_HANDLER_H_

#include "CaptureResponseListener.h"
#include "OrbitBase/Logging.h"
#include "OrbitBase/Tracing.h"
#include "OrbitLinuxTracing/Tracer.h"
#include "OrbitLinuxTracing/TracerListener.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "capture.pb.h"

namespace orbit_service {

class LinuxTracingHandler : public LinuxTracing::TracerListener {
 public:
  explicit LinuxTracingHandler(CaptureResponseListener* capture_response_listener)
      : capture_response_listener_{capture_response_listener} {}

  ~LinuxTracingHandler() override = default;
  LinuxTracingHandler(const LinuxTracingHandler&) = delete;
  LinuxTracingHandler& operator=(const LinuxTracingHandler&) = delete;
  LinuxTracingHandler(LinuxTracingHandler&&) = delete;
  LinuxTracingHandler& operator=(LinuxTracingHandler&&) = delete;

  void Start(orbit_grpc_protos::CaptureOptions capture_options);
  void Stop();

  void OnSchedulingSlice(orbit_grpc_protos::SchedulingSlice scheduling_slice) override;
  void OnCallstackSample(orbit_grpc_protos::CallstackSample callstack_sample) override;
  void OnFunctionCall(orbit_grpc_protos::FunctionCall function_call) override;
  void OnIntrospectionScope(orbit_grpc_protos::IntrospectionScope introspection_call) override;
  void OnGpuJob(orbit_grpc_protos::GpuJob gpu_job) override;
  void OnThreadName(orbit_grpc_protos::ThreadName thread_name) override;
  void OnThreadStateSlice(orbit_grpc_protos::ThreadStateSlice thread_state_slice) override;
  void OnAddressInfo(orbit_grpc_protos::AddressInfo address_info) override;
  void OnTracepointEvent(orbit_grpc_protos::TracepointEvent tracepoint_event) override;

 private:
  CaptureResponseListener* capture_response_listener_;
  std::unique_ptr<LinuxTracing::Tracer> tracer_;

  // Manual instrumentation tracing listener.
  std::unique_ptr<orbit::tracing::Listener> orbit_tracing_listener_;

  [[nodiscard]] static uint64_t ComputeCallstackKey(const orbit_grpc_protos::Callstack& callstack);
  [[nodiscard]] uint64_t InternCallstackIfNecessaryAndGetKey(
      orbit_grpc_protos::Callstack callstack);
  [[nodiscard]] static uint64_t ComputeStringKey(const std::string& str);
  [[nodiscard]] uint64_t InternStringIfNecessaryAndGetKey(std::string str);
  [[nodiscard]] uint64_t InternTracepointInfoIfNecessaryAndGetKey(
      orbit_grpc_protos::TracepointInfo tracepoint_info);

  absl::flat_hash_set<uint64_t> addresses_seen_;
  absl::Mutex addresses_seen_mutex_;
  absl::flat_hash_set<uint64_t> callstack_keys_sent_;
  absl::Mutex callstack_keys_sent_mutex_;
  absl::flat_hash_set<uint64_t> string_keys_sent_;
  absl::Mutex string_keys_sent_mutex_;
  absl::flat_hash_set<uint64_t> tracepoint_keys_sent_;
  absl::Mutex tracepoint_keys_sent_mutex_;

  void SetupIntrospection();
  void SenderThread();

  std::vector<orbit_grpc_protos::CaptureEvent> event_buffer_;
  absl::Mutex event_buffer_mutex_;
  std::thread sender_thread_;
};

}  // namespace orbit_service

#endif  // ORBIT_SERVICE_LINUX_TRACING_HANDLER_H_