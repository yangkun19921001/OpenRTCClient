/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/rtp_rtcp/source/rtcp_transceiver_impl.h"

#include <utility>

#include "api/call/transport.h"
#include "modules/rtp_rtcp/include/receive_statistics.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "modules/rtp_rtcp/source/rtcp_packet.h"
#include "modules/rtp_rtcp/source/rtcp_packet/common_header.h"
#include "modules/rtp_rtcp/source/rtcp_packet/extended_reports.h"
#include "modules/rtp_rtcp/source/rtcp_packet/fir.h"
#include "modules/rtp_rtcp/source/rtcp_packet/nack.h"
#include "modules/rtp_rtcp/source/rtcp_packet/pli.h"
#include "modules/rtp_rtcp/source/rtcp_packet/receiver_report.h"
#include "modules/rtp_rtcp/source/rtcp_packet/report_block.h"
#include "modules/rtp_rtcp/source/rtcp_packet/sdes.h"
#include "modules/rtp_rtcp/source/rtcp_packet/sender_report.h"
#include "modules/rtp_rtcp/source/time_util.h"
#include "rtc_base/checks.h"
#include "rtc_base/ptr_util.h"
#include "rtc_base/task_queue.h"
#include "rtc_base/timeutils.h"

namespace webrtc {
namespace {

struct SenderReportTimes {
  int64_t local_received_time_us;
  NtpTime remote_sent_time;
};

}  // namespace

struct RtcpTransceiverImpl::RemoteSenderState {
  uint8_t fir_sequence_number = 0;
  rtc::Optional<SenderReportTimes> last_received_sender_report;
};

// Helper to put several RTCP packets into lower layer datagram composing
// Compound or Reduced-Size RTCP packet, as defined by RFC 5506 section 2.
// TODO(danilchap): When in compound mode and packets are so many that several
// compound RTCP packets need to be generated, ensure each packet is compound.
class RtcpTransceiverImpl::PacketSender
    : public rtcp::RtcpPacket::PacketReadyCallback {
 public:
  PacketSender(Transport* transport, size_t max_packet_size)
      : transport_(transport), max_packet_size_(max_packet_size) {
    RTC_CHECK_LE(max_packet_size, IP_PACKET_SIZE);
  }
  ~PacketSender() override {
    RTC_DCHECK_EQ(index_, 0) << "Unsent rtcp packet.";
  }

  // Appends a packet to pending compound packet.
  // Sends rtcp compound packet if buffer was already full and resets buffer.
  void AppendPacket(const rtcp::RtcpPacket& packet) {
    packet.Create(buffer_, &index_, max_packet_size_, this);
  }

  // Sends pending rtcp compound packet.
  void Send() {
    if (index_ > 0) {
      OnPacketReady(buffer_, index_);
      index_ = 0;
    }
  }

  bool IsEmpty() const { return index_ == 0; }

 private:
  // Implements RtcpPacket::PacketReadyCallback
  void OnPacketReady(uint8_t* data, size_t length) override {
    transport_->SendRtcp(data, length);
  }

  Transport* const transport_;
  const size_t max_packet_size_;
  size_t index_ = 0;
  uint8_t buffer_[IP_PACKET_SIZE];
};

RtcpTransceiverImpl::RtcpTransceiverImpl(const RtcpTransceiverConfig& config)
    : config_(config), ptr_factory_(this) {
  RTC_CHECK(config_.Validate());
  if (config_.schedule_periodic_compound_packets)
    SchedulePeriodicCompoundPackets(config_.initial_report_delay_ms);
}

RtcpTransceiverImpl::~RtcpTransceiverImpl() = default;

void RtcpTransceiverImpl::ReceivePacket(rtc::ArrayView<const uint8_t> packet,
                                        int64_t now_us) {
  while (!packet.empty()) {
    rtcp::CommonHeader rtcp_block;
    if (!rtcp_block.Parse(packet.data(), packet.size()))
      return;

    HandleReceivedPacket(rtcp_block, now_us);

    // TODO(danilchap): Use packet.remove_prefix() when that function exists.
    packet = packet.subview(rtcp_block.packet_size());
  }
}

void RtcpTransceiverImpl::SendCompoundPacket() {
  SendPeriodicCompoundPacket();
  ReschedulePeriodicCompoundPackets();
}

void RtcpTransceiverImpl::SetRemb(int bitrate_bps,
                                  std::vector<uint32_t> ssrcs) {
  RTC_DCHECK_GE(bitrate_bps, 0);
  remb_.emplace();
  remb_->SetSsrcs(std::move(ssrcs));
  remb_->SetBitrateBps(bitrate_bps);
  // TODO(bugs.webrtc.org/8239): Move logic from PacketRouter for sending remb
  // immideately on large bitrate change when there is one RtcpTransceiver per
  // rtp transport.
}

void RtcpTransceiverImpl::UnsetRemb() {
  remb_.reset();
}

void RtcpTransceiverImpl::SendNack(uint32_t ssrc,
                                   std::vector<uint16_t> sequence_numbers) {
  RTC_DCHECK(!sequence_numbers.empty());
  rtcp::Nack nack;
  nack.SetSenderSsrc(config_.feedback_ssrc);
  nack.SetMediaSsrc(ssrc);
  nack.SetPacketIds(std::move(sequence_numbers));
  SendImmediateFeedback(nack);
}

void RtcpTransceiverImpl::SendPictureLossIndication(uint32_t ssrc) {
  rtcp::Pli pli;
  pli.SetSenderSsrc(config_.feedback_ssrc);
  pli.SetMediaSsrc(ssrc);
  SendImmediateFeedback(pli);
}

void RtcpTransceiverImpl::SendFullIntraRequest(
    rtc::ArrayView<const uint32_t> ssrcs) {
  RTC_DCHECK(!ssrcs.empty());
  rtcp::Fir fir;
  fir.SetSenderSsrc(config_.feedback_ssrc);
  for (uint32_t media_ssrc : ssrcs)
    fir.AddRequestTo(media_ssrc,
                     remote_senders_[media_ssrc].fir_sequence_number++);
  SendImmediateFeedback(fir);
}

void RtcpTransceiverImpl::HandleReceivedPacket(
    const rtcp::CommonHeader& rtcp_packet_header,
    int64_t now_us) {
  switch (rtcp_packet_header.type()) {
    case rtcp::SenderReport::kPacketType:
      HandleSenderReport(rtcp_packet_header, now_us);
      break;
    case rtcp::ExtendedReports::kPacketType:
      HandleExtendedReports(rtcp_packet_header, now_us);
      break;
  }
}

void RtcpTransceiverImpl::HandleSenderReport(
    const rtcp::CommonHeader& rtcp_packet_header,
    int64_t now_us) {
  rtcp::SenderReport sender_report;
  if (!sender_report.Parse(rtcp_packet_header))
    return;
  rtc::Optional<SenderReportTimes>& last =
      remote_senders_[sender_report.sender_ssrc()].last_received_sender_report;
  last.emplace();
  last->local_received_time_us = now_us;
  last->remote_sent_time = sender_report.ntp();
}

void RtcpTransceiverImpl::HandleExtendedReports(
    const rtcp::CommonHeader& rtcp_packet_header,
    int64_t now_us) {
  rtcp::ExtendedReports extended_reports;
  if (!extended_reports.Parse(rtcp_packet_header))
    return;
  if (extended_reports.dlrr() && config_.non_sender_rtt_measurement &&
      config_.rtt_observer) {
    // Delay and last_rr are transferred using 32bit compact ntp resolution.
    // Convert packet arrival time to same format through 64bit ntp format.
    uint32_t receive_time_ntp = CompactNtp(TimeMicrosToNtp(now_us));
    for (const rtcp::ReceiveTimeInfo& rti :
         extended_reports.dlrr().sub_blocks()) {
      if (rti.ssrc != config_.feedback_ssrc)
        continue;
      uint32_t rtt_ntp =
          receive_time_ntp - rti.delay_since_last_rr - rti.last_rr;
      int64_t rtt_ms = CompactNtpRttToMs(rtt_ntp);
      config_.rtt_observer->OnRttUpdate(rtt_ms);
    }
  }
}

void RtcpTransceiverImpl::ReschedulePeriodicCompoundPackets() {
  if (!config_.schedule_periodic_compound_packets)
    return;
  // Stop existent send task.
  ptr_factory_.InvalidateWeakPtrs();
  SchedulePeriodicCompoundPackets(config_.report_period_ms);
}

void RtcpTransceiverImpl::SchedulePeriodicCompoundPackets(int64_t delay_ms) {
  class SendPeriodicCompoundPacketTask : public rtc::QueuedTask {
   public:
    SendPeriodicCompoundPacketTask(rtc::TaskQueue* task_queue,
                                   rtc::WeakPtr<RtcpTransceiverImpl> ptr)
        : task_queue_(task_queue), ptr_(std::move(ptr)) {}
    bool Run() override {
      RTC_DCHECK(task_queue_->IsCurrent());
      if (!ptr_)
        return true;
      ptr_->SendPeriodicCompoundPacket();
      task_queue_->PostDelayedTask(rtc::WrapUnique(this),
                                   ptr_->config_.report_period_ms);
      return false;
    }

   private:
    rtc::TaskQueue* const task_queue_;
    const rtc::WeakPtr<RtcpTransceiverImpl> ptr_;
  };

  RTC_DCHECK(config_.schedule_periodic_compound_packets);

  auto task = rtc::MakeUnique<SendPeriodicCompoundPacketTask>(
      config_.task_queue, ptr_factory_.GetWeakPtr());
  if (delay_ms > 0)
    config_.task_queue->PostDelayedTask(std::move(task), delay_ms);
  else
    config_.task_queue->PostTask(std::move(task));
}

void RtcpTransceiverImpl::CreateCompoundPacket(PacketSender* sender) {
  RTC_DCHECK(sender->IsEmpty());
  const uint32_t sender_ssrc = config_.feedback_ssrc;
  int64_t now_us = rtc::TimeMicros();
  rtcp::ReceiverReport receiver_report;
  receiver_report.SetSenderSsrc(sender_ssrc);
  receiver_report.SetReportBlocks(CreateReportBlocks(now_us));
  sender->AppendPacket(receiver_report);

  if (!config_.cname.empty()) {
    rtcp::Sdes sdes;
    bool added = sdes.AddCName(config_.feedback_ssrc, config_.cname);
    RTC_DCHECK(added) << "Failed to add cname " << config_.cname
                      << " to rtcp sdes packet.";
    sender->AppendPacket(sdes);
  }
  if (remb_) {
    remb_->SetSenderSsrc(sender_ssrc);
    sender->AppendPacket(*remb_);
  }
  // TODO(bugs.webrtc.org/8239): Do not send rrtr if this packet starts with
  // SenderReport instead of ReceiverReport
  // when RtcpTransceiver supports rtp senders.
  if (config_.non_sender_rtt_measurement) {
    rtcp::ExtendedReports xr;

    rtcp::Rrtr rrtr;
    rrtr.SetNtp(TimeMicrosToNtp(now_us));
    xr.SetRrtr(rrtr);

    xr.SetSenderSsrc(sender_ssrc);
    sender->AppendPacket(xr);
  }
}

void RtcpTransceiverImpl::SendPeriodicCompoundPacket() {
  PacketSender sender(config_.outgoing_transport, config_.max_packet_size);
  CreateCompoundPacket(&sender);
  sender.Send();
}

void RtcpTransceiverImpl::SendImmediateFeedback(
    const rtcp::RtcpPacket& rtcp_packet) {
  PacketSender sender(config_.outgoing_transport, config_.max_packet_size);
  // Compound mode requires every sent rtcp packet to be compound, i.e. start
  // with a sender or receiver report.
  if (config_.rtcp_mode == RtcpMode::kCompound)
    CreateCompoundPacket(&sender);

  sender.AppendPacket(rtcp_packet);
  sender.Send();

  // If compound packet was sent, delay (reschedule) the periodic one.
  if (config_.rtcp_mode == RtcpMode::kCompound)
    ReschedulePeriodicCompoundPackets();
}

std::vector<rtcp::ReportBlock> RtcpTransceiverImpl::CreateReportBlocks(
    int64_t now_us) {
  if (!config_.receive_statistics)
    return {};
  // TODO(danilchap): Support sending more than
  // |ReceiverReport::kMaxNumberOfReportBlocks| per compound rtcp packet.
  std::vector<rtcp::ReportBlock> report_blocks =
      config_.receive_statistics->RtcpReportBlocks(
          rtcp::ReceiverReport::kMaxNumberOfReportBlocks);
  for (rtcp::ReportBlock& report_block : report_blocks) {
    auto it = remote_senders_.find(report_block.source_ssrc());
    if (it == remote_senders_.end() || !it->second.last_received_sender_report)
      continue;
    const SenderReportTimes& last_sender_report =
        *it->second.last_received_sender_report;
    report_block.SetLastSr(CompactNtp(last_sender_report.remote_sent_time));
    report_block.SetDelayLastSr(SaturatedUsToCompactNtp(
        now_us - last_sender_report.local_received_time_us));
  }
  return report_blocks;
}

}  // namespace webrtc
