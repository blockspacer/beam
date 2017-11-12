#ifndef BEAM_MULTI_REACTOR_HPP
#define BEAM_MULTI_REACTOR_HPP
#include <type_traits>
#include <boost/optional/optional.hpp>
#include "Beam/Pointers/LocalPtr.hpp"
#include "Beam/Reactors/CommitReactor.hpp"
#include "Beam/Reactors/Reactor.hpp"
#include "Beam/Reactors/ReactorUnavailableException.hpp"
#include "Beam/Reactors/Reactors.hpp"
#include "Beam/Utilities/Algorithm.hpp"
#include "Beam/Utilities/Functional.hpp"

namespace Beam {
namespace Reactors {
namespace Details {
  template<typename T>
  struct MultiReactorType {
    using type = T;
  };

  template<typename T>
  struct MultiReactorType<boost::optional<T>> {
    using type = T;
  };

  template<typename T>
  struct MultiReactorEval {
    template<typename V, typename F, typename P>
    bool operator ()(V& value, F& function, const P& p) const {
      auto update = boost::optional<T>{function(p)};
      if(update.is_initialized()) {
        value = std::move(*update);
        return true;
      }
      return false;
    }
  };

  template<>
  struct MultiReactorEval<void> {
    template<typename V, typename F, typename P>
    bool operator ()(V& value, F& function, const P& p) const {
      function(p);
      return true;
    }
  };
}

  /*! \class MultiReactor
      \brief A Reactor that calls a function when any of its children updates.
      \tparam FunctionType The type of function to apply.
   */
  template<typename FunctionType>
  class MultiReactor : public Reactor<typename Details::MultiReactorType<
      typename std::decay<GetResultOf<FunctionType,
      const std::vector<std::shared_ptr<BaseReactor>>&>>::type>::type> {
    public:
      using Type = typename Reactor<typename Details::MultiReactorType<
        typename std::decay<GetResultOf<FunctionType,
        const std::vector<std::shared_ptr<BaseReactor>>&>>::type>::type>::Type;

      //! The type of function to apply.
      using Function = FunctionType;

      //! Constructs a MultiReactor.
      /*!
        \param function The function to apply.
        \param children The Reactors to monitor.
      */
      template<typename FunctionForward>
      MultiReactor(FunctionForward&& function,
        std::vector<std::shared_ptr<BaseReactor>> children);

      virtual bool IsComplete() const override final;

      virtual BaseReactor::Update Commit(int sequenceNumber) override final;

      virtual Type Eval() const override final;

    private:
      Function m_function;
      std::vector<std::shared_ptr<BaseReactor>> m_parameters;
      CommitReactor m_commitReactor;
      Expect<Type> m_value;
      bool m_hasValue;
      BaseReactor::Update m_update;
      int m_currentSequenceNumber;

      bool UpdateEval();
  };

  //! Makes a MultiReactor.
  /*!
    \param function The function to apply.
    \param children The Reactors to monitor.
  */
  template<typename Function>
  auto MakeMultiReactor(Function&& f,
      std::vector<std::shared_ptr<BaseReactor>> children) {
    return std::make_shared<MultiReactor<typename std::decay<Function>::type>>(
      std::forward<Function>(f), std::move(children));
  }

  template<typename FunctionType>
  template<typename FunctionForward>
  MultiReactor<FunctionType>::MultiReactor(FunctionForward&& function,
      std::vector<std::shared_ptr<BaseReactor>> children)
      : m_function{std::forward<FunctionForward>(function)},
        m_parameters{std::move(children)},
        m_commitReactor{
          Transform(m_parameters,
          [] (auto& reactor) {
            return reactor.get();
          })},
        m_value{std::make_exception_ptr(ReactorUnavailableException{})},
        m_hasValue{false},
        m_currentSequenceNumber{-1} {}

  template<typename FunctionType>
  bool MultiReactor<FunctionType>::IsComplete() const {
    return m_commitReactor.IsComplete();
  }

  template<typename FunctionType>
  BaseReactor::Update MultiReactor<FunctionType>::Commit(int sequenceNumber) {
    if(sequenceNumber == m_currentSequenceNumber) {
      return m_update;
    } else if(sequenceNumber == 0 && m_currentSequenceNumber != -1) {
      if(m_hasValue) {
        return BaseReactor::Update::EVAL;
      }
      return BaseReactor::Update::COMPLETE;
    }
    if(IsComplete()) {
      return BaseReactor::Update::NONE;
    }
    m_update = m_commitReactor.Commit(sequenceNumber);
    if(m_update == BaseReactor::Update::EVAL) {
      if(!UpdateEval()) {
        if(IsComplete()) {
          m_update = BaseReactor::Update::COMPLETE;
        } else {
          m_update = BaseReactor::Update::NONE;
        }
      } else {
        m_hasValue = true;
      }
    }
    m_currentSequenceNumber = sequenceNumber;
    return m_update;
  }

  template<typename FunctionType>
  typename MultiReactor<FunctionType>::Type
      MultiReactor<FunctionType>::Eval() const {
    return m_value.Get();
  }

  template<typename FunctionType>
  bool MultiReactor<FunctionType>::UpdateEval() {
    try {
      return Details::MultiReactorEval<Type>{}(m_value, m_function,
        m_parameters);
    } catch(const std::exception&) {
      m_value = std::current_exception();
      return true;
    }
  }
}
}

#endif
