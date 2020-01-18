#include "Beam/Python/Threading.hpp"
#include "Beam/Python/Beam.hpp"
#include "Beam/Python/Queues.hpp"
#include "Beam/Python/ToPythonTimer.hpp"
#include "Beam/Threading/LiveTimer.hpp"
#include "Beam/Threading/TimeoutException.hpp"
#include "Beam/Threading/TriggerTimer.hpp"
#include "Beam/Threading/VirtualTimer.hpp"

using namespace Beam;
using namespace Beam::Python;
using namespace Beam::Threading;
using namespace boost;
using namespace boost::posix_time;
using namespace pybind11;

namespace {
  struct TrampolineTimer final : VirtualTimer {
    void Start() override {
      PYBIND11_OVERLOAD_PURE_NAME(void, VirtualTimer, "start", Start);
    }

    void Cancel() override {
      PYBIND11_OVERLOAD_PURE_NAME(void, VirtualTimer, "cancel", Cancel);
    }

    void Wait() override {
      PYBIND11_OVERLOAD_PURE_NAME(void, VirtualTimer, "wait", Wait);
    }

    const Publisher<Timer::Result>& GetPublisher() const override {
      PYBIND11_OVERLOAD_PURE_NAME(const Publisher<Timer::Result>&, VirtualTimer,
        "get_publisher", GetPublisher);
    }
  };
}

void Beam::Python::ExportLiveTimer(pybind11::module& module) {
  class_<ToPythonTimer<LiveTimer>, VirtualTimer,
      std::shared_ptr<ToPythonTimer<LiveTimer>>>(module, "LiveTimer")
    .def(init(
      [] (time_duration interval) {
        return MakeToPythonTimer(std::make_unique<LiveTimer>(interval,
          Ref(*GetTimerThreadPool())));
      }));
}

void Beam::Python::ExportThreading(pybind11::module& module) {
  auto submodule = module.def_submodule("threading");
  ExportTimer(submodule);
  ExportLiveTimer(submodule);
  ExportTriggerTimer(submodule);
  register_exception<TimeoutException>(submodule, "TimeoutException");
}

void Beam::Python::ExportTimer(pybind11::module& module) {
  auto outer = class_<VirtualTimer, TrampolineTimer,
      std::shared_ptr<VirtualTimer>>(module, "Timer")
    .def("start", &VirtualTimer::Start)
    .def("cancel", &VirtualTimer::Cancel)
    .def("wait", &VirtualTimer::Wait)
    .def("get_publisher", &VirtualTimer::GetPublisher,
      return_value_policy::reference_internal);
  enum_<Timer::Result::Type>(outer, "Result")
    .value("NONE", Timer::Result::NONE)
    .value("EXPIRED", Timer::Result::EXPIRED)
    .value("CANCELED", Timer::Result::CANCELED)
    .value("FAIL", Timer::Result::FAIL);
  ExportQueueSuite<Timer::Result>(module, "TimerResult");
}

void Beam::Python::ExportTriggerTimer(pybind11::module& module) {
  class_<ToPythonTimer<TriggerTimer>, VirtualTimer,
      std::shared_ptr<ToPythonTimer<TriggerTimer>>>(module, "TriggerTimer")
    .def(init(
      [] {
        return MakeToPythonTimer(std::make_unique<TriggerTimer>());
      }))
    .def("trigger",
      [] (ToPythonTimer<TriggerTimer>& self) {
        self.GetTimer().Trigger();
      })
    .def("fail",
      [] (ToPythonTimer<TriggerTimer>& self) {
        self.GetTimer().Fail();
      });
}
