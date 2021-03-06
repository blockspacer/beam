#ifndef BEAM_VARIANTQUEUE_HPP
#define BEAM_VARIANTQUEUE_HPP
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/variant/variant.hpp>
#include "Beam/Queues/Queue.hpp"
#include "Beam/Queues/Queues.hpp"
#include "Beam/Utilities/NullType.hpp"

#define BEAM_VARIANT_QUEUE_PARAMETERS 10

namespace Beam {
  template<BOOST_PP_ENUM_BINARY_PARAMS(BEAM_VARIANT_QUEUE_PARAMETERS,
    typename A, = NullType BOOST_PP_INTERCEPT), typename DummyType = NullType>
  class VariantQueue {};

  #define BEAM_INHERIT_QUEUE(z, n, q)                                          \
    BOOST_PP_COMMA_IF(n) public QueueWriter<A##n>

  #define BEAM_DECLARE_PUSH(z, n, q)                                           \
    virtual void Push(const A##n& value) {                                     \
      m_queue.Push(value);                                                     \
    }

  #define BOOST_PP_LOCAL_MACRO(n)                                              \
  template<BOOST_PP_ENUM_PARAMS(n, typename A)>                                \
  class VariantQueue<BOOST_PP_ENUM_PARAMS(n, A)> :                             \
      BOOST_PP_REPEAT(n, BEAM_INHERIT_QUEUE, BOOST_PP_EMPTY),                  \
      public QueueReader<boost::variant<BOOST_PP_ENUM_PARAMS(n, A)>> {         \
    public:                                                                    \
      typedef typename QueueReader<                                            \
        boost::variant<BOOST_PP_ENUM_PARAMS(n, A)>>::Target Target;            \
                                                                               \
      VariantQueue() {}                                                        \
                                                                               \
      virtual ~VariantQueue() {                                                \
        Break();                                                               \
      }                                                                        \
                                                                               \
      virtual bool IsEmpty() const {                                           \
        return m_queue.IsEmpty();                                              \
      }                                                                        \
                                                                               \
      virtual Target Top() const {                                             \
        return m_queue.Top();                                                  \
      }                                                                        \
                                                                               \
      virtual void Pop() {                                                     \
        return m_queue.Pop();                                                  \
      }                                                                        \
                                                                               \
      BOOST_PP_REPEAT(n, BEAM_DECLARE_PUSH, BOOST_PP_EMPTY);                   \
                                                                               \
      virtual void Break(const std::exception_ptr& e) {                        \
        m_queue.Break(e);                                                      \
      }                                                                        \
                                                                               \
    protected:                                                                 \
      virtual bool IsAvailable() const {                                       \
        return m_queue.IsAvailable();                                          \
      }                                                                        \
                                                                               \
    private:                                                                   \
      Queue<boost::variant<BOOST_PP_ENUM_PARAMS(n, A)>> m_queue;               \
  };

  #define BOOST_PP_LOCAL_LIMITS (1, BEAM_VARIANT_QUEUE_PARAMETERS)
  #include BOOST_PP_LOCAL_ITERATE()
  #undef BEAM_DECLARE_PUSH
  #undef BEAM_INHERIT_QUEUE
}

#endif
