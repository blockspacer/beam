#ifndef BEAM_SERVICESTESTS_HPP
#define BEAM_SERVICESTESTS_HPP
#include <memory>
#include "Beam/Codecs/NullEncoder.hpp"
#include "Beam/IO/LocalClientChannel.hpp"
#include "Beam/IO/LocalServerConnection.hpp"
#include "Beam/IO/SharedBuffer.hpp"
#include "Beam/Pointers/NativePointerPolicy.hpp"
#include "Beam/Serialization/BinaryReceiver.hpp"
#include "Beam/Serialization/BinarySender.hpp"
#include "Beam/ServiceLocator/AuthenticationServletAdapter.hpp"
#include "Beam/ServiceLocator/VirtualServiceLocatorClient.hpp"
#include "Beam/Services/Services.hpp"
#include "Beam/Services/ServiceProtocolClientBuilder.hpp"
#include "Beam/Services/ServiceProtocolServer.hpp"
#include "Beam/Services/ServiceProtocolServletContainer.hpp"
#include "Beam/Threading/TriggerTimer.hpp"

namespace Beam {
namespace Services {
namespace Tests {
  class ServiceProtocolClientTester;
  class ServiceProtocolServerTester;
  class ServiceProtocolServletContainerTester;
  template<typename ChannelType> class TestServlet;

  //! The type of ServerConnections used for testing.
  using TestServerConnection = IO::LocalServerConnection<IO::SharedBuffer>;

  //! The type of client Channels used for testing.
  using TestClientChannel = IO::LocalClientChannel<IO::SharedBuffer>;

  //! The type of ServiceProtocolServer used for testing.
  using TestServiceProtocolServer = Services::ServiceProtocolServer<
    std::shared_ptr<TestServerConnection>,
    Serialization::BinarySender<IO::SharedBuffer>, Codecs::NullEncoder,
    std::unique_ptr<Threading::TriggerTimer>>;

  //! The type of ServiceProtocolClientBuilder used for testing.
  using TestServiceProtocolClientBuilder =
    Services::ServiceProtocolClientBuilder<Services::MessageProtocol<
    std::unique_ptr<TestClientChannel>,
    Serialization::BinarySender<IO::SharedBuffer>, Codecs::NullEncoder>,
    Threading::TriggerTimer>;

  //! Instantiates types of ServiceProtocolServletContainers used for testing.
  template<typename MetaServlet>
  using TestServiceProtocolServletContainer =
    ServiceProtocolServletContainer<MetaServlet,
    std::shared_ptr<TestServerConnection>,
    Serialization::BinarySender<IO::SharedBuffer>,
    Codecs::NullEncoder, std::unique_ptr<Beam::Threading::TriggerTimer>>;

  //! Instantiates types of ServiceProtocolServletContainers used for testing
  //! that require authentication.
  template<typename MetaServlet>
  using TestAuthenticatedServiceProtocolServletContainer =
    TestServiceProtocolServletContainer<
    ServiceLocator::MetaAuthenticationServletAdapter<MetaServlet,
    std::shared_ptr<ServiceLocator::VirtualServiceLocatorClient>>>;

  //! Instantiates ServiceProtocolClients used for testing.
  using TestServiceProtocolClient = Services::ServiceProtocolClient<
    Services::MessageProtocol<TestClientChannel,
    Serialization::BinarySender<IO::SharedBuffer>,
    Codecs::NullEncoder>, Threading::TriggerTimer>;
}
}
}

#endif
