#ifndef BEAM_BASIC_REACTOR_TESTER_HPP
#define BEAM_BASIC_REACTOR_TESTER_HPP
#include <cppunit/extensions/HelperMacros.h>
#include "Beam/ReactorsTests/ReactorTests.hpp"
#include "Beam/Utilities/BeamWorkaround.hpp"

namespace Beam {
namespace Reactors {
namespace Tests {

  /*  \class BasicReactorTester
      \brief Tests the BasicReactor class.
   */
  class BasicReactorTester : public CPPUNIT_NS::TestFixture {
    public:

      //! Tests completing without producing a value.
      void TestCompleteImmediately();

      //! Tests completing with an exception without producing a value.
      void TestCompleteWithThrowImmediately();

    private:
      CPPUNIT_TEST_SUITE(BasicReactorTester);
        CPPUNIT_TEST(TestCompleteImmediately);
        CPPUNIT_TEST(TestCompleteWithThrowImmediately);
      BEAM_CPPUNIT_TEST_SUITE_END();
  };
}
}
}

#endif
