/* Diagnostic Client library
* Copyright (C) 2023  Avijit Dey
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef DIAGNOSTIC_CLIENT_LIB_LIB_BOOST_SUPPORT_SOCKET_TCP_TCP_TYPES_H
#define DIAGNOSTIC_CLIENT_LIB_LIB_BOOST_SUPPORT_SOCKET_TCP_TCP_TYPES_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/include/span.h"

namespace boost_support {
namespace socket {
namespace tcp {

/**
 * @brief    Immutable class to store received tcp message
 */
class TcpMessage final {
public:
  /**
   * @brief    Definition of different socket state
   */
  enum class SocketState : std::uint8_t {
    kIdle = 0x00,
    kSocketOnline,
    kSocketOffline,
    kSocketRxMessageReceived,
    kSocketTxMessageSend,
    kSocketTxMessageConf,
    kSocketError
  };

  /**
   * @brief    Definition of different socket error that could occur
   */
  enum class SocketError : std::uint8_t {
    // state
    kNone = 0x00
  };

  /**
   * @brief    Type alias for underlying buffer
   */
  using BufferType = std::vector<uint8_t>;

  /**
   * @brief    Type alias of IP address type
   */
  using IpAddressType = std::string_view;

public:
  /**
   * @brief         Default constructor
   */
  TcpMessage() = default;

  /**
   * @brief         Constructs an instance of TcpMessage
   * @param[in]     host_ip_address
   *                The host ip address
   * @param[in]     host_port_number
   *                The host port number
   * @param[in]     payload
   *                The received data payload
   */
  TcpMessage(IpAddressType host_ip_address, std::uint16_t host_port_number, BufferType &&payload)
      : host_ip_address_{host_ip_address},
        host_port_number_{host_port_number},
        rx_buffer_{std::move(payload)} {}

  TcpMessage(TcpMessage &&other) noexcept = default;
  TcpMessage &operator=(TcpMessage &&other) noexcept = default;

  TcpMessage(const TcpMessage &other) = delete;
  TcpMessage &operator=(const TcpMessage &other) = delete;

  /**
   * @brief         Destructs an instance of TcpMessage
   */
  ~TcpMessage() = default;

  /**
   * @brief       Get the host ip address
   * @return      The IP address
   */
  IpAddressType GetHostIpAddress() const { return host_ip_address_; }

  /**
   * @brief       Get the host port number
   * @return      The port number
   */
  std::uint16_t GetHostPortNumber() const { return host_port_number_; }

  /**
   * @brief       Get the view to the rx buffer
   * @return      The rx buffer
   */
  core_type::Span<std::uint8_t> GetRxBuffer() { return core_type::Span<std::uint8_t>{rx_buffer_}; }

  /**
   * @brief       Get the reference to tx buffer
   * @return      The reference to buffer
   */
  BufferType &GetTxBuffer() { return tx_buffer_; }

  /**
   * @brief       Get the reference to tx buffer
   * @return      The reference to buffer
   */
  BufferType const &GetTxBuffer() const { return tx_buffer_; }

private:
  /**
   * @brief         Store the socket state
   */
  SocketState socket_state_{SocketState::kIdle};

  /**
   * @brief         Store the socket error
   */
  SocketError socket_error_{SocketError::kNone};

  /**
   * @brief         The reception buffer
   */
  BufferType rx_buffer_;

  /**
   * @brief         The transmission buffer
   */
  BufferType tx_buffer_{};

  /**
   * @brief    Store remote ip address
   */
  std::string host_ip_address_{};

  /**
   * @brief    Store remote port number
   */
  uint16_t host_port_number_{};
};

/**
 * @brief    The unique pointer to const TcpMessage
 */
using TcpMessageConstPtr = std::unique_ptr<const TcpMessage>;

/**
 * @brief    The unique pointer to TcpMessage
 */
using TcpMessagePtr = std::unique_ptr<TcpMessage>;

/**
 * @brief    Doip HeaderSize
 */
constexpr std::uint8_t kDoipheadrSize = 8U;

}  // namespace tcp
}  // namespace socket
}  // namespace boost_support
#endif  // DIAGNOSTIC_CLIENT_LIB_LIB_BOOST_SUPPORT_SOCKET_TCP_TCP_TYPES_H