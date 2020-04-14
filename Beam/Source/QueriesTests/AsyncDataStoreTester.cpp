#include <doctest/doctest.h>
#include <algorithm>
#include <string>
#include <vector>
#include <boost/date_time/posix_time/ptime.hpp>
#include "Beam/Queries/AsyncDataStore.hpp"
#include "Beam/Queries/BasicQuery.hpp"
#include "Beam/Queries/EvaluatorTranslator.hpp"
#include "Beam/Queries/LocalDataStore.hpp"
#include "Beam/QueriesTests/TestDataStore.hpp"
#include "Beam/Routines/RoutineHandler.hpp"
#include "Beam/Routines/Scheduler.hpp"
#include "Beam/TimeService/IncrementalTimeClient.hpp"
#include "Beam/Utilities/Algorithm.hpp"

using namespace Beam;
using namespace Beam::Queries;
using namespace Beam::Queries::Tests;
using namespace Beam::Routines;
using namespace Beam::Threading;
using namespace Beam::TimeService;
using namespace boost;
using namespace boost::posix_time;

namespace {
  struct Entry {
    int m_value;
    ptime m_timestamp;

    Entry() = default;
    Entry(int value, ptime timestamp);
    bool operator ==(const Entry& rhs) const;
  };

  using TestLocalDataStore = LocalDataStore<BasicQuery<std::string>, Entry,
    EvaluatorTranslator<QueryTypes>>;
  using DataStore = AsyncDataStore<TestLocalDataStore>;
  using DataStoreDispatcher = TestDataStore<BasicQuery<std::string>, Entry>;
  using IntrusiveDataStore = AsyncDataStore<std::shared_ptr<
    DataStoreDispatcher>, EvaluatorTranslator<QueryTypes>>;
  using SequencedEntry = SequencedValue<Entry>;
  using SequencedIndexedEntry = SequencedValue<
    IndexedValue<Entry, std::string>>;

  Entry::Entry(int value, ptime timestamp)
    : m_value(value),
      m_timestamp(timestamp) {}

  bool Entry::operator ==(const Entry& rhs) const {
    return m_value == rhs.m_value && m_timestamp == rhs.m_timestamp;
  }

  std::ostream& operator <<(std::ostream& out, const Entry& entry) {
    return out << entry.m_value;
  }

  template<typename DataStore>
  SequencedIndexedEntry StoreValue(DataStore& dataStore, std::string index,
      int value, const ptime& timestamp,
      const Beam::Queries::Sequence& sequence) {
    auto entry = SequencedValue(IndexedValue(Entry(value, timestamp), index),
      sequence);
    dataStore.Store(entry);
    return entry;
  }

  template<typename DataStore>
  void TestQuery(DataStore& dataStore, std::string index,
      const Beam::Queries::Range& range, const SnapshotLimit& limit,
      const std::vector<SequencedEntry>& expectedResult) {
    auto query = BasicQuery<std::string>();
    query.SetIndex(index);
    query.SetRange(range);
    query.SetSnapshotLimit(limit);
    auto queryResult = dataStore.Load(query);
    REQUIRE(expectedResult == queryResult);
  }
}

TEST_SUITE("AsyncDataStore") {
  TEST_CASE("store_and_load") {
    auto dataStore = DataStore(Initialize());
    dataStore.Open();
    auto timeClient = IncrementalTimeClient();
    auto sequence = Beam::Queries::Sequence(5);
    auto entryA = StoreValue(dataStore, "hello", 100, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryB = StoreValue(dataStore, "hello", 200, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryC = StoreValue(dataStore, "hello", 300, timeClient.GetTime(),
      sequence);
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit::Unlimited(), {entryA, entryB, entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 0), {});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 1), {entryA});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 2), {entryA, entryB});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 3), {entryA, entryB, entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 4), {entryA, entryB, entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 0), {});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 1), {entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 2), {entryB, entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 3), {entryA, entryB, entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 4), {entryA, entryB, entryC});
  }

  TEST_CASE("head_spanning_load") {
    auto localDataStore = TestLocalDataStore();
    auto dataStore = AsyncDataStore(&localDataStore);
    dataStore.Open();
    auto timeClient = IncrementalTimeClient();
    auto sequence = Beam::Queries::Sequence(5);
    auto entryA = StoreValue(localDataStore, "hello", 100, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryB = StoreValue(localDataStore, "hello", 101, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryC = StoreValue(dataStore, "hello", 102, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryD = StoreValue(dataStore, "hello", 103, timeClient.GetTime(),
      sequence);
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 1), {entryA});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 2), {entryA, entryB});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 3), {entryA, entryB, entryC});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::HEAD, 4),
      {entryA, entryB, entryC, entryD});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit::Unlimited(), {entryA, entryB, entryC, entryD});
  }

  TEST_CASE("tail_spanning_load") {
    auto localDataStore = TestLocalDataStore();
    auto dataStore = AsyncDataStore(&localDataStore);
    dataStore.Open();
    auto timeClient = IncrementalTimeClient();
    auto sequence = Beam::Queries::Sequence(5);
    auto entryA = StoreValue(localDataStore, "hello", 100, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryB = StoreValue(localDataStore, "hello", 101, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryC = StoreValue(dataStore, "hello", 102, timeClient.GetTime(),
      sequence);
    sequence = Increment(sequence);
    auto entryD = StoreValue(dataStore, "hello", 103, timeClient.GetTime(),
      sequence);
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 1), {entryD});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 2), {entryC, entryD});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 3), {entryB, entryC, entryD});
    TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
      SnapshotLimit(SnapshotLimit::Type::TAIL, 4),
      {entryA, entryB, entryC, entryD});
  }

  TEST_CASE("buffered_load") {
    auto dispatcher = std::make_shared<DataStoreDispatcher>();
    auto dataStore = IntrusiveDataStore(dispatcher);
    Open(*dispatcher);
    dataStore.Open();
    auto operations = std::make_shared<Queue<
      std::shared_ptr<DataStoreDispatcher::Operation>>>();
    dispatcher->GetOperationPublisher().Monitor(operations);
    auto timeClient = IncrementalTimeClient();
    auto entryA = SequencedIndexedEntry();
    auto entryB = SequencedIndexedEntry();
    auto entryC = SequencedIndexedEntry();
    auto firstStoreOperation =
      std::optional<DataStoreDispatcher::StoreOperation*>();
    {
      auto handler = RoutineHandler(Spawn(
        [&] {
          auto operation = operations->Top();
          operations->Pop();
          auto storeOperation = std::get_if<
            DataStoreDispatcher::StoreOperation>(&*operation);
          firstStoreOperation = storeOperation;
      }));
      auto sequence = Beam::Queries::Sequence(5);
      entryA = StoreValue(dataStore, "hello", 100, timeClient.GetTime(),
        sequence);
      sequence = Increment(sequence);
      entryB = StoreValue(dataStore, "hello", 200, timeClient.GetTime(),
        sequence);
      sequence = Increment(sequence);
      entryC = StoreValue(dataStore, "hello", 300, timeClient.GetTime(),
        sequence);
    }
    {
      auto handler = RoutineHandler(Spawn(
        [&] {
          for(auto i = 0; i != 11; ++i) {
            auto operation = operations->Top();
            operations->Pop();
            auto& loadOperation = std::get<DataStoreDispatcher::LoadOperation>(
              *operation);
            loadOperation.m_result.SetResult(std::vector<SequencedEntry>{});
          }
        }));
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit::Unlimited(), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 0), {});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 1), {entryA});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 2), {entryA, entryB});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 3), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 4), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 0), {});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 1), {entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 2), {entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 3), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 4), {entryA, entryB, entryC});
    }
    REQUIRE(firstStoreOperation.has_value());
    auto entriesLeft = 3 - (*firstStoreOperation)->m_values.size();
    (*firstStoreOperation)->m_result.SetResult();
    while(entriesLeft != 0) {
      auto operation = operations->Top();
      operations->Pop();
      auto& storeOperation = std::get<DataStoreDispatcher::StoreOperation>(
        *operation);
      entriesLeft -= storeOperation.m_values.size();
      storeOperation.m_result.SetResult();
    }
  }

  TEST_CASE("flushed_load") {
    auto localDataStore = TestLocalDataStore();
    auto dispatcher = std::make_shared<DataStoreDispatcher>();
    auto dataStore = IntrusiveDataStore(dispatcher);
    Open(*dispatcher);
    dataStore.Open();
    auto operations = std::make_shared<
      Queue<std::shared_ptr<DataStoreDispatcher::Operation>>>();
    dispatcher->GetOperationPublisher().Monitor(operations);
    auto timeClient = IncrementalTimeClient();
    auto entryA = SequencedIndexedEntry();
    auto entryB = SequencedIndexedEntry();
    auto entryC = SequencedIndexedEntry();
    {
      auto handler = RoutineHandler(Spawn(
        [&] {
          for(auto i = 3; i != 0;) {
            auto operation = operations->Top();
            operations->Pop();
            auto& storeOperation = std::get<
              DataStoreDispatcher::StoreOperation>(*operation);
            i -= storeOperation.m_values.size();
            localDataStore.Store(storeOperation.m_values);
            storeOperation.m_result.SetResult();
          }
        }));
      auto sequence = Beam::Queries::Sequence(5);
      entryA = StoreValue(dataStore, "hello", 100, timeClient.GetTime(),
        sequence);
      sequence = Increment(sequence);
      entryB = StoreValue(dataStore, "hello", 200, timeClient.GetTime(),
        sequence);
      sequence = Increment(sequence);
      entryC = StoreValue(dataStore, "hello", 300, timeClient.GetTime(),
        sequence);
    }
    {
      auto handler = RoutineHandler(Spawn(
        [&] {
          for(auto i = 0; i != 11; ++i) {
            auto operation = operations->Top();
            operations->Pop();
            auto& loadOperation = std::get<DataStoreDispatcher::LoadOperation>(
              *operation);
            auto values = localDataStore.Load(loadOperation.m_query);
            loadOperation.m_result.SetResult(values);
          }
        }));
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit::Unlimited(), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 0), {});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 1), {entryA});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 2), {entryA, entryB});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 3), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::HEAD, 4), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 0), {});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 1), {entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 2), {entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 3), {entryA, entryB, entryC});
      TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
        SnapshotLimit(SnapshotLimit::Type::TAIL, 4), {entryA, entryB, entryC});
    }
  }
}
