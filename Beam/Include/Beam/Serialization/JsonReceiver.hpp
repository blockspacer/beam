#ifndef BEAM_JSONRECEIVER_HPP
#define BEAM_JSONRECEIVER_HPP
#include <cstdint>
#include <cstring>
#include <deque>
#include <type_traits>
#include "Beam/IO/Buffer.hpp"
#include "Beam/Json/JsonParser.hpp"
#include "Beam/Serialization/DataShuttle.hpp"
#include "Beam/Serialization/ReceiverMixin.hpp"
#include "Beam/Serialization/SerializationException.hpp"
#include "Beam/Utilities/FixedString.hpp"

namespace Beam {
namespace Serialization {

  /*! \class JsonReceiver
      \brief Implements a Receiver using the JSON format.
      \tparam SourceType The type of Buffer to receive the data from.
   */
  template<typename SourceType>
  class JsonReceiver : public ReceiverMixin<JsonReceiver<SourceType>> {
    public:
      static_assert(ImplementsConcept<SourceType, IO::Buffer>::value,
        "SourceType must implement the Buffer Concept.");
      using Source = SourceType;

      //! Constructs a JsonReceiver.
      JsonReceiver() = default;

      //! Constructs a JsonReceiver.
      /*!
        \param registry The TypeRegistry used for receiving polymorphic types.
      */
      JsonReceiver(RefType<TypeRegistry<JsonSender<SourceType>>> registry);

      void SetSource(RefType<Source> source);

      void Shuttle(const char* name, bool& value);

      void Shuttle(const char* name, unsigned char& value);

      void Shuttle(const char* name, signed char& value);

      void Shuttle(const char* name, char& value);

      template<typename T>
      typename std::enable_if<std::is_integral<T>::value>::type Shuttle(
        const char* name, T& value);

      template<typename T>
      typename std::enable_if<std::is_floating_point<T>::value>::type Shuttle(
        const char* name, T& value);

      template<typename T>
      typename std::enable_if<ImplementsConcept<T, IO::Buffer>::value>::type
        Shuttle(const char* name, T& value);

      void Shuttle(const char* name, std::string& value);

      template<std::size_t N>
      void Shuttle(const char* name, FixedString<N>& value);

      void StartStructure(const char* name);

      void EndStructure();

      void StartSequence(const char* name, int& size);

      void StartSequence(const char* name);

      void EndSequence();

      using ReceiverMixin<JsonReceiver<SourceType>>::Shuttle;

    private:
      boost::optional<Parsers::ReaderParserStream<IO::BufferReader<Source>>>
        m_parserStream;
      JsonParser m_parser;
      std::deque<JsonObject> m_objectQueue;
  };

  template<typename SourceType>
  JsonReceiver<SourceType>::JsonReceiver(RefType<TypeRegistry<
      JsonSender<SourceType>>> registry)
      : ReceiverMixin<JsonReceiver<SourceType>>(Ref(registry)) {}

  template<typename SourceType>
  void JsonReceiver<SourceType>::SetSource(RefType<Source> source) {
    m_parserStream.emplace(*source);
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::Shuttle(const char* name, bool& value) {
    boost::optional<JsonValue> outValue;
    auto& jsonValue =
      [&]() -> const JsonValue& {
        if(name == nullptr) {
          outValue.emplace();
          if(!m_parser.Read(*m_parserStream, *outValue)) {
            BOOST_THROW_EXCEPTION(
              SerializationException{"Invalid JSON format."});
          }
          return *outValue;
        } else {
          return m_objectQueue.back().At(name);
        }
      }();
    try {
      value = boost::get<bool>(jsonValue);
    } catch(const boost::bad_get&) {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::Shuttle(const char* name,
      unsigned char& value) {
    int numericValue;
    Shuttle(name, numericValue);
    if(numericValue < std::numeric_limits<unsigned char>::min() ||
        numericValue > std::numeric_limits<unsigned char>::max()) {
      BOOST_THROW_EXCEPTION(SerializationException{"Value out of range."});
    }
    value = static_cast<unsigned char>(numericValue);
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::Shuttle(const char* name, signed char& value) {
    int numericValue;
    Shuttle(name, numericValue);
    if(numericValue < std::numeric_limits<signed char>::min() ||
        numericValue > std::numeric_limits<signed char>::max()) {
      BOOST_THROW_EXCEPTION(SerializationException{"Value out of range."});
    }
    value = static_cast<signed char>(numericValue);
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::Shuttle(const char* name, char& value) {
    boost::optional<JsonValue> outValue;
    auto& jsonValue =
      [&]() -> const JsonValue& {
        if(name == nullptr) {
          outValue.emplace();
          if(!m_parser.Read(*m_parserStream, *outValue)) {
            BOOST_THROW_EXCEPTION(
              SerializationException{"Invalid JSON format."});
          }
          return *outValue;
        } else {
          return m_objectQueue.back().At(name);
        }
      }();
    if(auto s = boost::get<std::string>(&jsonValue)) {
      if(s->size() != 1) {
        BOOST_THROW_EXCEPTION(SerializationException{"Length out of range."});
      }
      value = s->front();
    } else if(boost::get<double>(&jsonValue)) {
      value = '\0';
    } else {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  template<typename T>
  typename std::enable_if<std::is_integral<T>::value>::type
      JsonReceiver<SourceType>::Shuttle(const char* name, T& value) {
    double rawValue;
    Shuttle(name, rawValue);
    value = static_cast<T>(rawValue);
  }

  template<typename SourceType>
  template<typename T>
  typename std::enable_if<std::is_floating_point<T>::value>::type
      JsonReceiver<SourceType>::Shuttle(const char* name, T& value) {
    boost::optional<JsonValue> outValue;
    auto& jsonValue =
      [&]() -> const JsonValue& {
        if(name == nullptr) {
          outValue.emplace();
          if(!m_parser.Read(*m_parserStream, *outValue)) {
            BOOST_THROW_EXCEPTION(
              SerializationException{"Invalid JSON format."});
          }
          return *outValue;
        } else {
          return m_objectQueue.back().At(name);
        }
      }();
    if(auto s = boost::get<double>(&jsonValue)) {
      value = static_cast<T>(*s);
    } else {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  template<typename T>
  typename std::enable_if<ImplementsConcept<T, IO::Buffer>::value>::type
      JsonReceiver<SourceType>::Shuttle(const char* name, T& value) {}

  template<typename SourceType>
  void JsonReceiver<SourceType>::Shuttle(const char* name, std::string& value) {
    boost::optional<JsonValue> outValue;
    auto& jsonValue =
      [&]() -> const JsonValue& {
        if(name == nullptr) {
          outValue.emplace();
          if(!m_parser.Read(*m_parserStream, *outValue)) {
            BOOST_THROW_EXCEPTION(
              SerializationException{"Invalid JSON format."});
          }
          return *outValue;
        } else {
          return m_objectQueue.back().At(name);
        }
      }();
    if(auto s = boost::get<std::string>(&jsonValue)) {
      value = std::move(*s);
    } else {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  template<std::size_t N>
  void JsonReceiver<SourceType>::Shuttle(const char* name,
      FixedString<N>& value) {
    boost::optional<JsonValue> outValue;
    auto& jsonValue =
      [&]() -> const JsonValue& {
        if(name == nullptr) {
          outValue.emplace();
          if(!m_parser.Read(*m_parserStream, *outValue)) {
            BOOST_THROW_EXCEPTION(
              SerializationException{"Invalid JSON format."});
          }
          return *outValue;
        } else {
          return m_objectQueue.back().At(name);
        }
      }();
    if(auto s = boost::get<std::string>(&jsonValue)) {
      if(s->size() > N) {
        BOOST_THROW_EXCEPTION(SerializationException{"Length out of range."});
      }
      value = *s;
    } else {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::StartStructure(const char* name) {
    if(name == nullptr) {
      JsonValue jsonValue;
      if(!m_parser.Read(*m_parserStream, jsonValue)) {
        BOOST_THROW_EXCEPTION(SerializationException{"Invalid JSON format."});
      }
      if(auto s = boost::get<JsonObject>(&jsonValue)) {
        m_objectQueue.push_back(*s);
      } else {
        BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
      }
    }
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::EndStructure() {
    m_objectQueue.pop_back();
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::StartSequence(const char* name, int& size) {}

  template<typename SourceType>
  void JsonReceiver<SourceType>::StartSequence(const char* name) {}

  template<typename SourceType>
  void JsonReceiver<SourceType>::EndSequence() {}

  template<typename SourceType>
  struct Inverse<JsonReceiver<SourceType>> {
    using type = JsonSender<SourceType>;
  };
}

  template<typename SourceType>
  struct ImplementsConcept<Serialization::JsonReceiver<SourceType>,
    Serialization::Receiver<SourceType>> : std::true_type {};
}

#endif
