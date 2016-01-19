#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include "Beam/ServicesTests/MessageProtocolTester.hpp"
#include "Beam/ServicesTests/ServiceProtocolClientTester.hpp"
#include "Beam/ServicesTests/ServiceProtocolServerTester.hpp"
#include "Beam/ServicesTests/ServiceProtocolServletContainerTester.hpp"

using namespace Beam::Services::Tests;

int main() {
  CPPUNIT_NS::TextUi::TestRunner runner;
  CppUnit::BriefTestProgressListener listener;
  runner.addTest(MessageProtocolTester::suite());
  runner.addTest(ServiceProtocolClientTester::suite());
  runner.addTest(ServiceProtocolServerTester::suite());
  runner.addTest(ServiceProtocolServletContainerTester::suite());
  runner.eventManager().addListener(&listener);
  runner.setOutputter(new CPPUNIT_NS::CompilerOutputter(&runner.result(),
    CPPUNIT_NS::stdCOut()));
  bool wasSucessful = runner.run();
  return wasSucessful ? 0 : 1;
}
