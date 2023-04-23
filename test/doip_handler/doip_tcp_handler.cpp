/* Diagnostic Client library
* Copyright (C) 2022  Avijit Dey
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "doip_handler/doip_tcp_handler.h"
#include "doip_handler/common_doip_types.h"

namespace ara {
namespace diag {
namespace doip {

DoipTcpHandler::DoipTcpHandler(std::string_view local_tcp_address, std::uint16_t tcp_port_num)
  : tcp_socket_handler_{
      std::make_unique<tcpSocket::DoipTcpSocketHandler>(local_tcp_address, tcp_port_num)} {
}

DoipTcpHandler::~DoipTcpHandler() = default;

DoipTcpHandler::DoipChannel& DoipTcpHandler::CreateDoipChannel(std::uint16_t logical_address) {
  // create new doip channel
  doip_channel_list_.emplace(logical_address,
                             std::make_unique<DoipTcpHandler::DoipChannel>(logical_address, *tcp_socket_handler_));
  return *doip_channel_list_[logical_address];
}

DoipTcpHandler::DoipChannel::DoipChannel(std::uint16_t logical_address,
                                         tcpSocket::DoipTcpSocketHandler &tcp_socket_handler)
  : logical_address_{logical_address},
    tcp_socket_handler_{tcp_socket_handler},
    tcp_connection_{},
    exit_request_{false},
    running_{false} {
  // Start thread to receive messages
  thread_ = std::thread([this]() {
    std::unique_lock<std::mutex> lck(mutex_);
    while (!exit_request_) {
      if (!running_) {
        cond_var_.wait(lck, [this]() {
          return exit_request_ || running_;
        });
      }
      if (!exit_request_.load()) {
        if (running_) {
          std::function<void(void)> const job{job_queue_.front()};
          job();
          job_queue_.pop();
        }
      }
    }
  });
}

DoipTcpHandler::DoipChannel::~DoipChannel() {
  exit_request_ = true;
  running_ = false;
  cond_var_.notify_all();
  thread_.join();
}

void DoipTcpHandler::DoipChannel::Initialize() {
  {
    std::lock_guard<std::mutex> const lck{mutex_};
    job_queue_.emplace([this](){
      this->StartAcceptingConnection();
    });
    running_ = true;
  }
  cond_var_.notify_all();
}

void DoipTcpHandler::DoipChannel::DeInitialize() {
  if(tcp_connection_) {
    tcp_connection_->DeInitialize();
  }
}

void DoipTcpHandler::DoipChannel::StartAcceptingConnection() {
  // Get the tcp connection - this will return after client is connected
  tcp_connection_ = std::move(
      tcp_socket_handler_.CreateTcpConnection(
          [this](TcpMessagePtr tcp_rx_message) {
            // handle message
            this->HandleMessage(std::move(tcp_rx_message));
          }
          ));
  if(tcp_connection_) {
    tcp_connection_->Initialize();
    running_ = false;
  }
}

void DoipTcpHandler::DoipChannel::HandleMessage(TcpMessagePtr tcp_rx_message) {
  received_doip_message_.host_ip_address = tcp_rx_message->host_ip_address_;
  received_doip_message_.port_num = tcp_rx_message->host_port_num_;
  received_doip_message_.protocol_version = tcp_rx_message->rxBuffer_[0];
  received_doip_message_.protocol_version_inv = tcp_rx_message->rxBuffer_[1];
  received_doip_message_.payload_type = GetDoIPPayloadType(tcp_rx_message->rxBuffer_);
  received_doip_message_.payload_length = GetDoIPPayloadLength(tcp_rx_message->rxBuffer_);
  if (received_doip_message_.payload_length > 0U) {
    received_doip_message_.payload.insert(received_doip_message_.payload.begin(),
                                          tcp_rx_message->rxBuffer_.begin() + kDoipheadrSize, tcp_rx_message->rxBuffer_.end());
  }
  // Trigger async transmission
  {
    std::lock_guard<std::mutex> const lck{mutex_};
    job_queue_.emplace([this](){
      if(received_doip_message_.payload_type == kDoip_RoutingActivation_ReqType) {
        this->SendRoutingActivationResponse();
      }
    });
    running_ = true;
  }
  cond_var_.notify_all();
}

auto DoipTcpHandler::DoipChannel::GetDoIPPayloadType(std::vector<uint8_t> payload) noexcept -> uint16_t {
  return ((uint16_t) (((payload[BYTE_POS_TWO] & 0xFF) << 8) | (payload[BYTE_POS_THREE] & 0xFF)));
}

auto DoipTcpHandler::DoipChannel::GetDoIPPayloadLength(std::vector<uint8_t> payload) noexcept -> uint32_t {
  return ((uint32_t) ((payload[BYTE_POS_FOUR] << 24U) & 0xFF000000) |
          (uint32_t) ((payload[BYTE_POS_FIVE] << 16U) & 0x00FF0000) |
          (uint32_t) ((payload[BYTE_POS_SIX] << 8U) & 0x0000FF00) | (uint32_t) ((payload[BYTE_POS_SEVEN] & 0x000000FF)));
}

void DoipTcpHandler::DoipChannel::CreateDoipGenericHeader(std::vector<uint8_t> &doipHeader, std::uint16_t payload_type,
                                                          std::uint32_t payload_len) {
  doipHeader.push_back(kDoip_ProtocolVersion);
  doipHeader.push_back(~((uint8_t) kDoip_ProtocolVersion));
  doipHeader.push_back((uint8_t) ((payload_type & 0xFF00) >> 8));
  doipHeader.push_back((uint8_t) (payload_type & 0x00FF));
  doipHeader.push_back((uint8_t) ((payload_len & 0xFF000000) >> 24));
  doipHeader.push_back((uint8_t) ((payload_len & 0x00FF0000) >> 16));
  doipHeader.push_back((uint8_t) ((payload_len & 0x0000FF00) >> 8));
  doipHeader.push_back((uint8_t) (payload_len & 0x000000FF));
}

void DoipTcpHandler::DoipChannel::SendRoutingActivationResponse() {
  TcpMessagePtr routing_activation_response{std::make_unique<TcpMessage>()};
  // create header
  routing_activation_response->txBuffer_.reserve(kDoipheadrSize + kDoip_RoutingActivation_ResMinLen);
  CreateDoipGenericHeader(routing_activation_response->txBuffer_, kDoip_RoutingActivation_ResType, kDoip_RoutingActivation_ResMinLen);

  // logical address of client
  routing_activation_response->txBuffer_.emplace_back(received_doip_message_.payload[0]);
  routing_activation_response->txBuffer_.emplace_back(received_doip_message_.payload[1]);
  // logical address of server
  routing_activation_response->txBuffer_.emplace_back(logical_address_ >> 8U);
  routing_activation_response->txBuffer_.emplace_back(logical_address_ & 0xFFU);
  // activation response code
  routing_activation_response->txBuffer_.emplace_back(routing_activation_res_code_);
  routing_activation_response->txBuffer_.emplace_back(0x00);
  routing_activation_response->txBuffer_.emplace_back(0x00);
  routing_activation_response->txBuffer_.emplace_back(0x00);
  routing_activation_response->txBuffer_.emplace_back(0x00);

  if(tcp_connection_->Transmit(std::move(routing_activation_response))) {
    running_ = false;
  }
}

void DoipTcpHandler::DoipChannel::SetExpectedRoutingActivationResponseToBeSent(
    std::uint8_t routing_activation_res_code) {
  routing_activation_res_code_ = routing_activation_res_code;
}

}  // namespace doip
}  // namespace diag
}  // namespace ara