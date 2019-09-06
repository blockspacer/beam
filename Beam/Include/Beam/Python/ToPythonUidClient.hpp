#ifndef BEAM_TO_PYTHON_UID_CLIENT_HPP
#define BEAM_TO_PYTHON_UID_CLIENT_HPP
#include "Beam/UidService/VirtualUidClient.hpp"

namespace Beam::UidService {

  /** Wraps a UidClient for use with Python.
   * @param C The type of UidClient to wrap.
   */
  template<typename C>
  class ToPythonUidClient final : public VirtualUidClient {
    public:

      /** The type of UidClient to wrap. */
      using Client = C;

      /**
       * Constructs a ToPythonUidClient.
       * @param client The UidClient to wrap.
       */
      ToPythonUidClient(std::unique_ptr<Client> client);

      ~ToPythonUidClient() override;

      std::uint64_t LoadNextUid() override;

      void Open() override;

      void Close() override;

    private:
      std::unique_ptr<Client> m_client;
  };

  /**
   * Makes a ToPythonUidClient.
   * @param client The UidClient to wrap.
   */
  template<typename Client>
  auto MakeToPythonUidClient(std::unique_ptr<Client> client) {
    return std::make_unique<ToPythonUidClient<Client>>(std::move(client));
  }

  template<typename C>
  ToPythonUidClient<C>::ToPythonUidClient(std::unique_ptr<Client> client)
    : m_client(std::move(client)) {}

  template<typename C>
  ToPythonUidClient<C>::~ToPythonUidClient() {
    Close();
    auto release = pybind11::gil_scoped_release();
    m_client.reset();
  }

  template<typename C>
  std::uint64_t ToPythonUidClient<C>::LoadNextUid() {
    auto release = pybind11::gil_scoped_release();
    return m_client->LoadNextUid();
  }

  template<typename C>
  void ToPythonUidClient<C>::Open() {
    auto release = pybind11::gil_scoped_release();
    m_client->Open();
  }

  template<typename C>
  void ToPythonUidClient<C>::Close() {
    auto release = pybind11::gil_scoped_release();
    m_client->Close();
  }
}

#endif
