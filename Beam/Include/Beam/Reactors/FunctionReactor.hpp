#ifndef BEAM_FUNCTION_REACTOR_HPP
#define BEAM_FUNCTION_REACTOR_HPP
#include <tuple>
#include <type_traits>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/accumulate.hpp>
#include <boost/optional/optional.hpp>
#include "Beam/Pointers/LocalPtr.hpp"
#include "Beam/Reactors/Reactor.hpp"
#include "Beam/Reactors/ReactorUnavailableException.hpp"
#include "Beam/Reactors/Reactors.hpp"
#include "Beam/Utilities/Expect.hpp"

namespace Beam {
namespace Reactors {

  /*! \class FunctionEvaluation
      \brief Stores the result of a function used in a FunctionReactor.
      \tparam T The result of the function.
   */
  template<typename T>
  struct FunctionEvaluation {
    using Type = T;
    boost::optional<Expect<Type>> m_value;
    BaseReactor::Update m_update;

    //! Constructs an unitialized evaluation.
    FunctionEvaluation();

    //! Constructs a FunctionEvaluation resulting in a type and an EVAL.
    /*!
      \param value The value returned by the function.
    */
    FunctionEvaluation(Expect<Type> value);

    //! Constructs a FunctionEvaluation resulting in a type and an EVAL.
    /*!
      \param value The value returned by the function.
    */
    FunctionEvaluation(Type value);

    //! Constructs a FunctionEvaluation resulting in a type and an EVAL.
    /*!
      \param value The value returned by the function.
    */
    FunctionEvaluation(boost::optional<Expect<Type>> value);

    //! Constructs a FunctionEvaluation resulting in a type and an EVAL.
    /*!
      \param value The value returned by the function.
    */
    FunctionEvaluation(boost::optional<Type> value);

    //! Constructs a FunctionEvaluation resulting in a type and an update.
    /*!
      \param value The value returned by the function.
      \param update The type of update.
    */
    FunctionEvaluation(Expect<Type> value, BaseReactor::Update update);

    //! Constructs a FunctionEvaluation resulting in a type and an update.
    /*!
      \param value The value returned by the function.
      \param update The type of update.
    */
    FunctionEvaluation(boost::optional<Expect<Type>> value,
      BaseReactor::Update update);

    //! Constructs a FunctionEvaluation resulting in a type and an update.
    /*!
      \param value The value returned by the function.
      \param update The type of update.
    */
    FunctionEvaluation(boost::optional<Type> value, BaseReactor::Update update);

    //! Constructs a FunctionEvaluation resulting in just an update.
    /*!
      \param update The type of update.
    */
    FunctionEvaluation(BaseReactor::Update update);
  };

namespace Details {
  template<typename T>
  struct FunctionReactorType {
    using type = T;
  };

  template<typename T>
  struct FunctionReactorType<boost::optional<T>> {
    using type = T;
  };

  template<typename T>
  struct FunctionReactorType<FunctionEvaluation<T>> {
    using type = T;
  };

  struct IsComplete {
    using result_type = bool;

    template<typename T>
    bool operator ()(bool state, const T& reactor) const {
      return state && reactor->IsComplete();
    }
  };

  struct Initialize {
    using result_type = int;
    int m_sequenceNumber;

    template<typename T>
    int operator()(int count, T& reactor) const {
      if(reactor->Commit(0) != BaseReactor::Update::NONE ||
          reactor->Commit(m_sequenceNumber) != BaseReactor::Update::NONE) {
        return count + 1;
      }
      return count;
    }
  };

  struct Commit {
    using result_type = BaseReactor::Update;
    int m_sequenceNumber;

    template<typename T>
    BaseReactor::Update operator ()(BaseReactor::Update update,
        T& reactor) const {
      auto reactorUpdate = reactor->Commit(m_sequenceNumber);
      if(reactorUpdate == BaseReactor::Update::NONE) {
        return update;
      } else if(reactorUpdate == BaseReactor::Update::COMPLETE) {
        if(update == BaseReactor::Update::NONE ||
            update == BaseReactor::Update::COMPLETE) {
          return reactorUpdate;
        } else {
          return update;
        }
      }
      return reactorUpdate;
    }
  };

  struct FunctionEval {
    template<typename T>
    auto operator ()(const Reactor<T>* reactor) const {
      return reactor->Eval();
    }

    Expect<void> operator ()(const BaseReactor* reactor) const {

      // TODO: Find a way to evaluate this.
      return {};
    }
  };

  template<typename T>
  struct FunctionUpdateEval {
    template<typename V, typename F, typename P>
    BaseReactor::Update operator ()(V& value, F& function, const P& p) const {
      auto evaluation = Apply(p,
        [&] (const auto&... parameters) {
          return FunctionEvaluation<T>{function(
            Try(std::bind(FunctionEval{}, &*parameters))...)};
        });
      if(evaluation.m_value.is_initialized()) {
        value = std::move(*evaluation.m_value);
      }
      return evaluation.m_update;
    }
  };

  template<>
  struct FunctionUpdateEval<void> {
    template<typename V, typename F, typename P>
    BaseReactor::Update operator ()(V& value, F& function, const P& p) const {
      Apply(p,
        [&] (const auto&... parameters) {
          function(Try(std::bind(FunctionEval{}, &*parameters))...);
        });
      return BaseReactor::Update::EVAL;
    }
  };
}

  /*! \class FunctionReactor
      \brief A Reactor that applies a function to its parameters.
      \tparam FunctionType The type of function to apply.
      \tparam ParameterTypes The type of parameters to apply the function to.
   */
  template<typename FunctionType, typename... ParameterTypes>
  class FunctionReactor : public Reactor<typename Details::FunctionReactorType<
      typename std::decay<GetResultOf<FunctionType, const Expect<GetReactorType<
      ParameterTypes>>&...>>::type>::type> {
    public:
      using Type = typename Reactor<typename Details::FunctionReactorType<
        typename std::decay<GetResultOf<FunctionType,
        const Expect<GetReactorType<ParameterTypes>>&...>>::type>::type>::Type;

      //! The type of function to apply.
      using Function = FunctionType;

      //! Constructs a FunctionReactor.
      /*!
        \param function The function to apply.
        \param parameters The parameters to apply the <i>function</i> to.
      */
      template<typename FunctionForward, typename... ParameterForwards>
      FunctionReactor(FunctionForward&& function,
        ParameterForwards&&... parameters);

      virtual bool IsComplete() const override final;

      virtual BaseReactor::Update Commit(int sequenceNumber) override final;

      virtual Type Eval() const override final;

    private:
      Function m_function;
      std::tuple<GetOptionalLocalPtr<ParameterTypes>...> m_parameters;
      Expect<Type> m_value;
      bool m_hasValue;
      BaseReactor::Update m_state;
      BaseReactor::Update m_update;
      int m_initializationCount;
      int m_currentSequenceNumber;

      bool AreParametersComplete() const;
      BaseReactor::Update UpdateEval();
  };

  //! Makes a FunctionReactor.
  /*!
    \param function The function to apply.
    \param parameters The parameters to apply the <i>function</i> to.
  */
  template<typename Function, typename... Parameters>
  auto MakeFunctionReactor(Function&& f, Parameters&&... parameters) {
    return std::make_shared<FunctionReactor<typename std::decay<Function>::type,
      typename std::decay<Parameters>::type...>>(std::forward<Function>(f),
      std::forward<Parameters>(parameters)...);
  }

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation()
      : m_update{BaseReactor::Update::NONE} {}

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(Expect<Type> value)
      : m_value{std::move(value)},
        m_update{BaseReactor::Update::EVAL} {}

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(Type value)
      : FunctionEvaluation<T>{Expect<Type>{std::move(value)}} {}

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(boost::optional<Expect<Type>> value)
      : m_value{std::move(value)} {
    if(m_value.is_initialized()) {
      m_update = BaseReactor::Update::EVAL;
    } else {
      m_update = BaseReactor::Update::NONE;
    }
  }

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(boost::optional<Type> value)
      : FunctionEvaluation{boost::optional<Expect<Type>>{std::move(value)}} {}

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(Expect<Type> value,
      BaseReactor::Update update)
      : m_value{std::move(value)},
        m_update{update} {
    assert(m_update != BaseReactor::Update::NONE);
  }

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(boost::optional<Expect<Type>> value,
      BaseReactor::Update update)
      : m_value{std::move(value)},
        m_update{update} {
    if(m_value.is_initialized()) {
      assert(m_update != BaseReactor::Update::NONE);
    } else {
      assert(m_update != BaseReactor::Update::EVAL);
    }
  }

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(boost::optional<Type> value,
      BaseReactor::Update update)
      : FunctionEvaluation{boost::optional<Expect<Type>>{
          Expect<Type>{std::move(value)}}, update} {}

  template<typename T>
  FunctionEvaluation<T>::FunctionEvaluation(BaseReactor::Update update)
      : m_value{boost::none},
        m_update{update} {
    assert(m_update != BaseReactor::Update::EVAL);
  }

  template<typename FunctionType, typename... ParameterTypes>
  template<typename FunctionForward, typename... ParameterForwards>
  FunctionReactor<FunctionType, ParameterTypes...>::FunctionReactor(
      FunctionForward&& function, ParameterForwards&&... parameters)
      : m_function{std::forward<FunctionForward>(function)},
        m_parameters{std::forward<ParameterForwards>(parameters)...},
        m_value{std::make_exception_ptr(ReactorUnavailableException{})},
        m_hasValue{false},
        m_state{BaseReactor::Update::NONE},
        m_initializationCount{0},
        m_currentSequenceNumber{-1} {}

  template<typename FunctionType, typename... ParameterTypes>
  bool FunctionReactor<FunctionType, ParameterTypes...>::IsComplete() const {
    return m_state == BaseReactor::Update::COMPLETE;
  }

  template<typename FunctionType, typename... ParameterTypes>
  BaseReactor::Update FunctionReactor<FunctionType, ParameterTypes...>::Commit(
      int sequenceNumber) {
    if(sequenceNumber == m_currentSequenceNumber) {
      return m_update;
    } else if(sequenceNumber == 0 && m_currentSequenceNumber != -1) {
      if(m_hasValue) {
        return BaseReactor::Update::EVAL;
      }
      return BaseReactor::Update::COMPLETE;
    }
    if(m_state == BaseReactor::Update::COMPLETE) {
      return BaseReactor::Update::NONE;
    }
    auto update =
      [&] {
        if(sizeof...(ParameterTypes) == 0) {
          if(sequenceNumber == 0) {
            return BaseReactor::Update::EVAL;
          } else {
            return BaseReactor::Update::NONE;
          }
        }
        if(m_initializationCount != sizeof...(ParameterTypes)) {
          m_initializationCount = boost::fusion::accumulate(m_parameters,
            0, Details::Initialize{sequenceNumber});
          if(m_initializationCount != sizeof...(ParameterTypes)) {
            return BaseReactor::Update::NONE;
          }
        }
        return boost::fusion::accumulate(m_parameters,
          BaseReactor::Update::NONE, Details::Commit{sequenceNumber});
      }();
    if(update == BaseReactor::Update::NONE) {
      return update;
    }
    m_update = update;
    if(m_update == BaseReactor::Update::EVAL) {
      auto hasEval = UpdateEval();
      if(AreParametersComplete()) {
        m_state = BaseReactor::Update::COMPLETE;
      }
      if(hasEval == BaseReactor::Update::NONE) {
        if(sizeof...(ParameterTypes) == 0) {
          m_update = BaseReactor::Update::COMPLETE;
        } else {
          m_update = BaseReactor::Update::NONE;
        }
      } else if(hasEval == BaseReactor::Update::EVAL) {
        m_hasValue = true;
      } else {
        m_hasValue = true;
        m_state = BaseReactor::Update::COMPLETE;
      }
    } else if(m_update == BaseReactor::Update::COMPLETE) {
      if(AreParametersComplete()) {
        m_state = BaseReactor::Update::COMPLETE;
      } else {
        m_update = BaseReactor::Update::NONE;
      }
    }
    m_currentSequenceNumber = sequenceNumber;
    return m_update;
  }

  template<typename FunctionType, typename... ParameterTypes>
  typename FunctionReactor<FunctionType, ParameterTypes...>::Type
      FunctionReactor<FunctionType, ParameterTypes...>::Eval() const {
    return m_value.Get();
  }

  template<typename FunctionType, typename... ParameterTypes>
  bool FunctionReactor<FunctionType, ParameterTypes...>::
      AreParametersComplete() const {
    return boost::fusion::accumulate(m_parameters, true, Details::IsComplete());
  }

  template<typename FunctionType, typename... ParameterTypes>
  BaseReactor::Update FunctionReactor<FunctionType, ParameterTypes...>::
      UpdateEval() {
    try {
      return Details::FunctionUpdateEval<Type>{}(m_value, m_function,
        m_parameters);
    } catch(const std::exception&) {
      m_value = std::current_exception();
      return BaseReactor::Update::EVAL;
    }
  }
}
}

#endif
