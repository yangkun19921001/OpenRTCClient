/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/rtp_rtcp/source/rtcp_packet.h"

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/modules/rtp_rtcp/source/byte_io.h"

namespace webrtc {
namespace rtcp {
namespace {
void AssignUWord8(uint8_t* buffer, size_t* offset, uint8_t value) {
  buffer[(*offset)++] = value;
}
void AssignUWord16(uint8_t* buffer, size_t* offset, uint16_t value) {
  ByteWriter<uint16_t>::WriteBigEndian(buffer + *offset, value);
  *offset += 2;
}
}  // namespace

void RtcpPacket::Append(RtcpPacket* packet) {
  assert(packet);
  appended_packets_.push_back(packet);
}

rtc::scoped_ptr<RawPacket> RtcpPacket::Build() const {
  size_t length = 0;
  rtc::scoped_ptr<RawPacket> packet(new RawPacket(IP_PACKET_SIZE));

  class PacketVerifier : public PacketReadyCallback {
   public:
    explicit PacketVerifier(RawPacket* packet)
        : called_(false), packet_(packet) {}
    virtual ~PacketVerifier() {}
    void OnPacketReady(uint8_t* data, size_t length) override {
      RTC_CHECK(!called_) << "Fragmentation not supported.";
      called_ = true;
      packet_->SetLength(length);
    }

   private:
    bool called_;
    RawPacket* const packet_;
  } verifier(packet.get());
  CreateAndAddAppended(packet->MutableBuffer(), &length, packet->BufferLength(),
                       &verifier);
  OnBufferFull(packet->MutableBuffer(), &length, &verifier);
  return packet;
}

bool RtcpPacket::Build(PacketReadyCallback* callback) const {
  uint8_t buffer[IP_PACKET_SIZE];
  return BuildExternalBuffer(buffer, IP_PACKET_SIZE, callback);
}

bool RtcpPacket::BuildExternalBuffer(uint8_t* buffer,
                                     size_t max_length,
                                     PacketReadyCallback* callback) const {
  size_t index = 0;
  if (!CreateAndAddAppended(buffer, &index, max_length, callback))
    return false;
  return OnBufferFull(buffer, &index, callback);
}

bool RtcpPacket::CreateAndAddAppended(uint8_t* packet,
                                      size_t* index,
                                      size_t max_length,
                                      PacketReadyCallback* callback) const {
  if (!Create(packet, index, max_length, callback))
    return false;
  for (RtcpPacket* appended : appended_packets_) {
    if (!appended->CreateAndAddAppended(packet, index, max_length, callback))
      return false;
  }
  return true;
}

bool RtcpPacket::OnBufferFull(uint8_t* packet,
                              size_t* index,
                              RtcpPacket::PacketReadyCallback* callback) const {
  if (*index == 0)
    return false;
  callback->OnPacketReady(packet, *index);
  *index = 0;
  return true;
}

size_t RtcpPacket::HeaderLength() const {
  size_t length_in_bytes = BlockLength();
  // Length in 32-bit words minus 1.
  assert(length_in_bytes > 0);
  return ((length_in_bytes + 3) / 4) - 1;
}

// From RFC 3550, RTP: A Transport Protocol for Real-Time Applications.
//
// RTP header format.
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P| RC/FMT  |      PT       |             length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

void RtcpPacket::CreateHeader(
    uint8_t count_or_format,  // Depends on packet type.
    uint8_t packet_type,
    size_t length,
    uint8_t* buffer,
    size_t* pos) {
  assert(length <= 0xffff);
  const uint8_t kVersion = 2;
  AssignUWord8(buffer, pos, (kVersion << 6) + count_or_format);
  AssignUWord8(buffer, pos, packet_type);
  AssignUWord16(buffer, pos, length);
}

RawPacket::RawPacket(size_t buffer_length)
    : buffer_length_(buffer_length), length_(0) {
  buffer_.reset(new uint8_t[buffer_length]);
}

RawPacket::RawPacket(const uint8_t* packet, size_t packet_length)
    : buffer_length_(packet_length), length_(packet_length) {
  buffer_.reset(new uint8_t[packet_length]);
  memcpy(buffer_.get(), packet, packet_length);
}

const uint8_t* RawPacket::Buffer() const {
  return buffer_.get();
}

uint8_t* RawPacket::MutableBuffer() {
  return buffer_.get();
}

size_t RawPacket::BufferLength() const {
  return buffer_length_;
}

size_t RawPacket::Length() const {
  return length_;
}

void RawPacket::SetLength(size_t length) {
  assert(length <= buffer_length_);
  length_ = length;
}

}  // namespace rtcp
}  // namespace webrtc
