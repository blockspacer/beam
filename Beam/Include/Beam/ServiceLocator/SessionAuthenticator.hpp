#ifndef BEAM_SESSIONAUTHENTICATOR_HPP
#define BEAM_SESSIONAUTHENTICATOR_HPP
#include <cryptopp/osrng.h>
#include "Beam/Pointers/Ref.hpp"
#include "Beam/ServiceLocator/Authenticator.hpp"
#include "Beam/ServiceLocator/ServiceLocator.hpp"
#include "Beam/ServiceLocator/ServiceLocatorServices.hpp"

namespace Beam {
namespace ServiceLocator {

  /*! \class SessionAuthenticator
      \brief Used to authenticate a ServiceProtocolClient's session.
      \tparam ServiceLocatorClientType The type of ServiceLocatorClient used to
              authenticate the session.
  */
  template<typename ServiceLocatorClientType>
  class SessionAuthenticator {
    public:

      //! Constructs a SessionAuthenticator.
      /*!
        \param serviceLocatorClient The ServiceLocatorClient used to
               authenticate the session.
      */
      SessionAuthenticator(Beam::Ref<ServiceLocatorClientType>
        serviceLocatorClient);

      template<typename ServiceProtocolClient>
      void operator ()(ServiceProtocolClient& client) const;

    private:
      ServiceLocatorClientType* m_serviceLocatorClient;
  };

  template<typename ServiceLocatorClientType>
  SessionAuthenticator<ServiceLocatorClientType>::SessionAuthenticator(
      Beam::Ref<ServiceLocatorClientType> serviceLocatorClient)
      : m_serviceLocatorClient(serviceLocatorClient.Get()) {}

  template<typename ServiceLocatorClientType>
  template<typename ServiceProtocolClient>
  void SessionAuthenticator<ServiceLocatorClientType>::operator ()(
      ServiceProtocolClient& client) const {
    CryptoPP::AutoSeededRandomPool randomGenerator;
    int key;
    randomGenerator.GenerateBlock(reinterpret_cast<unsigned char*>(&key),
      sizeof(key));
    RegisterServiceLocatorServices(Store(client.GetSlots()));
    client.template SendRequest<SendSessionIdService>(key,
      m_serviceLocatorClient->GetEncryptedSessionId(key));
  }
}
}

#endif
