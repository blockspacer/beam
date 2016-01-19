#ifndef BEAM_NETWORK_DETAILS_HPP
#define BEAM_NETWORK_DETAILS_HPP
#include <memory>
#include <utility>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include "Beam/IO/EndOfFileException.hpp"
#include "Beam/Network/Network.hpp"
#include "Beam/Threading/ConditionVariable.hpp"
#include "Beam/Threading/Mutex.hpp"

namespace Beam {
namespace Network {
namespace Details {
  template<typename SocketType>
  struct SocketEntry {
    using Socket = SocketType;
    Threading::Mutex m_mutex;
    Socket m_socket;
    bool m_isOpen;
    bool m_isReadPending;
    int m_pendingWrites;
    Threading::ConditionVariable m_isPendingCondition;

    template<typename... Args>
    SocketEntry(Args&&... args)
        : m_socket{std::forward<Args>(args)...},
          m_isOpen{false},
          m_isReadPending{false},
          m_pendingWrites{0} {}

    void Close() {
      boost::system::error_code errorCode;
      boost::unique_lock<Threading::Mutex> lock(m_mutex);
      if(!m_isOpen) {
        return;
      }
      m_isOpen = false;
      m_socket.shutdown(Socket::shutdown_both, errorCode);
      m_socket.close(errorCode);
      while(m_isReadPending) {
        m_isPendingCondition.wait(lock);
      }
      while(m_pendingWrites != 0) {
        m_isPendingCondition.wait(lock);
      }
    }

    void EndReadOperation() {
      boost::lock_guard<Threading::Mutex> lock{m_mutex};
      m_isReadPending = false;
      if(!m_isOpen) {
        m_isPendingCondition.notify_one();
      }
    }

    void BeginWriteOperation() {
      boost::lock_guard<Threading::Mutex> lock{m_mutex};
      if(!m_isOpen) {
        BOOST_THROW_EXCEPTION(IO::EndOfFileException{});
      }
      ++m_pendingWrites;
    }

    void EndWriteOperation() {
      boost::lock_guard<Threading::Mutex> lock{m_mutex};
      --m_pendingWrites;
      if(m_pendingWrites == 0 && !m_isOpen) {
        m_isPendingCondition.notify_one();
      }
    }
  };
  using TcpSocketEntry = SocketEntry<boost::asio::ip::tcp::socket>;
  using UdpSocketEntry = SocketEntry<boost::asio::ip::udp::socket>;

  inline bool IsEndOfFile(const boost::system::error_code& error) {
    return error == boost::asio::error::broken_pipe ||
      error == boost::asio::error::connection_aborted ||
      error == boost::asio::error::connection_reset ||
      error == boost::asio::error::eof ||
      error == boost::asio::error::operation_aborted ||
      error == boost::asio::error::shut_down ||
      error == boost::asio::error::timed_out;
  }
}
}
}

#endif
