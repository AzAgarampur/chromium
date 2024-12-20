// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_stream_pool_job.h"

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "net/base/load_states.h"
#include "net/base/net_error_details.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/base/port_util.h"
#include "net/dns/public/resolve_error_info.h"
#include "net/http/http_network_session.h"
#include "net/http/http_stream_pool.h"
#include "net/http/http_stream_pool_attempt_manager.h"
#include "net/http/http_stream_pool_group.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/socket/connection_attempts.h"
#include "net/socket/next_proto.h"
#include "net/socket/stream_socket.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_info.h"
#include "net/third_party/quiche/src/quiche/quic/core/quic_versions.h"

namespace net {

namespace {

NextProtoSet CalculateAllowedAlpns(NextProto expected_protocol,
                                   bool is_http1_allowed) {
  NextProtoSet allowed_alpns = expected_protocol == NextProto::kProtoUnknown
                                   ? NextProtoSet::All()
                                   : NextProtoSet({expected_protocol});
  if (!is_http1_allowed) {
    static constexpr NextProtoSet kHttp11Protocols = {NextProto::kProtoUnknown,
                                                      NextProto::kProtoHTTP11};
    allowed_alpns.RemoveAll(kHttp11Protocols);
  }
  return allowed_alpns;
}

}  // namespace

HttpStreamPool::Job::Job(Delegate* delegate,
                         Group* group,
                         quic::ParsedQuicVersion quic_version,
                         NextProto expected_protocol,
                         const NetLogWithSource& net_log)
    : delegate_(delegate),
      group_(group),
      quic_version_(quic_version),
      allowed_alpns_(CalculateAllowedAlpns(expected_protocol,
                                           delegate_->is_http1_allowed())),
      net_log_(net_log),
      create_time_(base::TimeTicks::Now()) {
  CHECK(delegate_->is_http1_allowed() ||
        expected_protocol != NextProto::kProtoHTTP11);
}

HttpStreamPool::Job::~Job() {
  CHECK(group_);
  // `group_` may be deleted after this call.
  group_.ExtractAsDangling()->OnJobComplete(this);
}

void HttpStreamPool::Job::Start() {
  CHECK(group_);

  if (!group_->CanStartJob(this)) {
    return;
  }

  StartInternal();
}

void HttpStreamPool::Job::Resume() {
  resume_time_ = base::TimeTicks::Now();
  StartInternal();
}

LoadState HttpStreamPool::Job::GetLoadState() const {
  if (!attempt_manager()) {
    return LOAD_STATE_IDLE;
  }
  return attempt_manager()->GetLoadState();
}

void HttpStreamPool::Job::SetPriority(RequestPriority priority) {
  if (attempt_manager()) {
    attempt_manager()->SetJobPriority(this, priority);
  }
}

void HttpStreamPool::Job::AddConnectionAttempts(
    const ConnectionAttempts& attempts) {
  for (const auto& attempt : attempts) {
    connection_attempts_.emplace_back(attempt);
  }
}

void HttpStreamPool::Job::OnStreamReady(std::unique_ptr<HttpStream> stream,
                                        NextProto negotiated_protocol) {
  CHECK(delegate_);

  int result = OK;
  if (!allowed_alpns_.Has(negotiated_protocol)) {
    const bool is_h2_or_h3_required = !delegate_->is_http1_allowed();
    const bool is_h2_or_h3 = negotiated_protocol == NextProto::kProtoHTTP2 ||
                             negotiated_protocol == NextProto::kProtoQUIC;
    if (is_h2_or_h3_required && !is_h2_or_h3) {
      result = ERR_H2_OR_QUIC_REQUIRED;
    } else {
      result = ERR_ALPN_NEGOTIATION_FAILED;
    }
  }

  if (result != OK) {
    OnStreamFailed(result, NetErrorDetails(), ResolveErrorInfo());
    return;
  }

  group_->http_network_session()->proxy_resolution_service()->ReportSuccess(
      delegate_->proxy_info());
  delegate_->OnStreamReady(this, std::move(stream), negotiated_protocol);
}

void HttpStreamPool::Job::OnStreamFailed(
    int status,
    const NetErrorDetails& net_error_details,
    ResolveErrorInfo resolve_error_info) {
  CHECK(delegate_);
  delegate_->OnStreamFailed(this, status, net_error_details,
                            resolve_error_info);
}

void HttpStreamPool::Job::OnCertificateError(int status,
                                             const SSLInfo& ssl_info) {
  CHECK(delegate_);
  delegate_->OnCertificateError(this, status, ssl_info);
}

void HttpStreamPool::Job::OnNeedsClientAuth(SSLCertRequestInfo* cert_info) {
  CHECK(delegate_);
  delegate_->OnNeedsClientAuth(this, cert_info);
}

base::TimeDelta HttpStreamPool::Job::CreateToResumeTime() const {
  if (resume_time_.is_null()) {
    return base::TimeDelta();
  }
  return resume_time_ - create_time_;
}

HttpStreamPool::AttemptManager* HttpStreamPool::Job::attempt_manager() const {
  CHECK(group_);
  return group_->attempt_manager();
}

void HttpStreamPool::Job::StartInternal() {
  CHECK(attempt_manager());
  CHECK(!attempt_manager()->is_failing());

  const url::SchemeHostPort& destination = group_->stream_key().destination();
  if (!IsPortAllowedForScheme(destination.port(), destination.scheme())) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&Job::OnStreamFailed, weak_ptr_factory_.GetWeakPtr(),
                       ERR_UNSAFE_PORT, NetErrorDetails(), ResolveErrorInfo()));
    return;
  }

  attempt_manager()->StartJob(this, priority(), delegate_->allowed_bad_certs(),
                              quic_version_, net_log_);
}

}  // namespace net
