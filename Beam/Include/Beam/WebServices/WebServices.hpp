#ifndef BEAM_WEBSERVICES_HPP
#define BEAM_WEBSERVICES_HPP

namespace Beam {
namespace WebServices {
  class Cookie;
  class FileStore;
  class HttpHeader;
  enum class HttpMethod;
  struct HttpRequestSlot;
  class HttpServerRequest;
  class HttpServerResponse;
  template<typename ServletType, typename ServerConnectionType>
    class HttpServletContainer;
  enum class HttpStatusCode;
  class HttpVersion;
  class InvalidHttpRequestException;
  class MalformedUriException;
  class Uri;
}
}

#endif