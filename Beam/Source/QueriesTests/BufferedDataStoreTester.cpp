#include "Beam/QueriesTests/BufferedDataStoreTester.hpp"
#include <string>
#include <vector>
#include <boost/date_time/posix_time/ptime.hpp>
#include "Beam/Queries/BasicQuery.hpp"
#include "Beam/Queries/BufferedDataStore.hpp"
#include "Beam/Queries/EvaluatorTranslator.hpp"
#include "Beam/Queries/LocalDataStore.hpp"
#include "Beam/TimeService/IncrementalTimeClient.hpp"

using namespace Beam;
using namespace Beam::Queries;
using namespace Beam::Queries::Tests;
using namespace Beam::Threading;
using namespace Beam::TimeService;
using namespace boost;
using namespace boost::posix_time;
using namespace std;

namespace {
  struct Entry {
    int m_value;
    ptime m_timestamp;

    Entry();
    Entry(int value, ptime timestamp);
    bool operator ==(const Entry& rhs) const;
  };

  typedef LocalDataStore<BasicQuery<string>, Entry,
    EvaluatorTranslator<QueryTypes>> TestLocalDataStore;
  typedef BufferedDataStore<TestLocalDataStore> DataStore;
  typedef SequencedValue<Entry> SequencedEntry;
  typedef SequencedValue<IndexedValue<Entry, string>> SequencedIndexedEntry;

  Entry::Entry() {}

  Entry::Entry(int value, ptime timestamp)
      : m_value(value),
        m_timestamp(timestamp) {}

  bool Entry::operator ==(const Entry& rhs) const {
    return m_value == rhs.m_value && m_timestamp == rhs.m_timestamp;
  }

  template<typename DataStore>
  SequencedIndexedEntry StoreValue(DataStore& dataStore, string index,
      int value, const ptime& timestamp,
      const Beam::Queries::Sequence& sequence) {
    auto entry = SequencedValue(IndexedValue(Entry(value, timestamp), index),
      sequence);
    dataStore.Store(entry);
    return entry;
  }

  template<typename DataStore>
  void TestQuery(DataStore& dataStore, string index,
      const Beam::Queries::Range& range, const SnapshotLimit& limit,
      const std::vector<SequencedEntry>& expectedResult) {
    BasicQuery<string> query;
    query.SetIndex(index);
    query.SetRange(range);
    query.SetSnapshotLimit(limit);
    auto queryResult = dataStore.Load(query);
    CPPUNIT_ASSERT(expectedResult == queryResult);
  }
}

void BufferedDataStoreTester::TestStoreAndLoad() {
  ThreadPool threadPool(1);
  DataStore dataStore(Initialize(), 10, Ref(threadPool));
  IncrementalTimeClient timeClient;
  Beam::Queries::Sequence sequence(5);
  SequencedIndexedEntry entryA = StoreValue(dataStore, "hello", 100,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryB = StoreValue(dataStore, "hello", 200,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryC = StoreValue(dataStore, "hello", 300,
    timeClient.GetTime(), sequence);
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit::Unlimited(), {entryA, entryB, entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 0), std::vector<SequencedEntry>());
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 1), {entryA});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 2), {entryA, entryB});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 3), {entryA, entryB, entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 4), {entryA, entryB, entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::TAIL, 0), std::vector<SequencedEntry>());
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::TAIL, 1), {entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::TAIL, 2), {entryB, entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::TAIL, 3), {entryA, entryB, entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::TAIL, 4), {entryA, entryB, entryC});
}

void BufferedDataStoreTester::TestHeadSpanningLoad() {
  TestLocalDataStore localDataStore;
  ThreadPool threadPool(1);
  BufferedDataStore<TestLocalDataStore*> dataStore(&localDataStore, 10,
    Ref(threadPool));
  IncrementalTimeClient timeClient;
  Beam::Queries::Sequence sequence(5);
  SequencedIndexedEntry entryA = StoreValue(localDataStore, "hello", 100,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryB = StoreValue(localDataStore, "hello", 101,
    timeClient.GetTime(), sequence);
  dataStore.Store(entryB);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryC = StoreValue(dataStore, "hello", 102,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryD = StoreValue(dataStore, "hello", 103,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit::Unlimited(), {entryA, entryB, entryC, entryD});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 1), {entryA});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 2), {entryA, entryB});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 3), {entryA, entryB, entryC});
  TestQuery(dataStore, "hello", Beam::Queries::Range::Total(),
    SnapshotLimit(SnapshotLimit::Type::HEAD, 4),
    {entryA, entryB, entryC, entryD});
}

void BufferedDataStoreTester::TestTailSpanningLoad() {
  TestLocalDataStore localDataStore;
  ThreadPool threadPool(1);
  BufferedDataStore<TestLocalDataStore*> dataStore(&localDataStore, 10,
    Ref(threadPool));
  IncrementalTimeClient timeClient;
  Beam::Queries::Sequence sequence(5);
  SequencedIndexedEntry entryA = StoreValue(localDataStore, "hello", 100,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryB = StoreValue(localDataStore, "hello", 101,
    timeClient.GetTime(), sequence);
  dataStore.Store(entryB);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryC = StoreValue(dataStore, "hello", 102,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
  SequencedIndexedEntry entryD = StoreValue(dataStore, "hello", 103,
    timeClient.GetTime(), sequence);
  sequence = Increment(sequence);
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
