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
      using AggregateType = boost::variant<JsonObject, std::vector<JsonValue>>;
      std::deque<AggregateType> m_aggregateQueue;

      const JsonValue& ExtractValue(const char* name,
        boost::optional<JsonValue>& storage);
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
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
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
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
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
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
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
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
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
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
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
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
    if(auto s = boost::get<JsonObject>(&jsonValue)) {
      m_aggregateQueue.push_back(*s);
    } else {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::EndStructure() {
    m_aggregateQueue.pop_back();
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::StartSequence(const char* name, int& size) {
    boost::optional<JsonValue> storage;
    auto& jsonValue = ExtractValue(name, storage);
    if(auto s = boost::get<std::vector<JsonValue>>(&jsonValue)) {
      m_aggregateQueue.push_back(*s);
      size = static_cast<int>(s->size());
    } else {
      BOOST_THROW_EXCEPTION(SerializationException{"JSON type mismatch."});
    }
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::StartSequence(const char* name) {
    int dummy;
    StartSequence(name, dummy);
  }

  template<typename SourceType>
  void JsonReceiver<SourceType>::EndSequence() {
    m_aggregateQueue.pop_back();
  }

  template<typename SourceType>
  const JsonValue& JsonReceiver<SourceType>::ExtractValue(const char* name,
      boost::optional<JsonValue>& storage) {
    if(name == nullptr) {
      storage.emplace();
      if(!m_parser.Read(*m_parserStream, *storage)) {
        BOOST_THROW_EXCEPTION(SerializationException{"Invalid JSON format."});
      }
      return *storage;
    } else if(auto aggregate =
        boost::get<JsonObject>(&m_aggregateQueue.back()))  {
      return aggregate->At(name);
    } else if(auto aggregate =
        boost::get<std::vector<JsonValue>>(&m_aggregateQueue.back())) {
      aggregate->emplace_back();
      return aggregate->back();
    }
    BOOST_THROW_EXCEPTION(SerializationException{"Invalid JSON format."});
  }

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