#ifndef BEAM_CONNECTEXCEPTION_HPP
#define BEAM_CONNECTEXCEPTION_HPP
#include "Beam/IO/IOException.hpp"

namespace Beam {
namespace IO {

  /*! \class ConnectException
      \brief Signals that a connect operation failed.
   */
  class ConnectException : public IOException {
    public:

      //! Constructs a ConnectException.
      ConnectException();

      //! Constructs a ConnectException.
      /*!
        \param message A message describing the error.
      */
      ConnectException(const std::string& message);
  };

  inline ConnectException::ConnectException()
    : IOException("Unable to connect.") {}

  inline ConnectException::ConnectException(const std::string& message)
    : IOException(message) {}
}
}

#endif
