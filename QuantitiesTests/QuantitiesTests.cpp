﻿#include "stdafx.h"
#include "CppUnitTest.h"

#include "..\Quantities\Quantities.h"
#include "..\Quantities\NamedQuantities.h"
#include "..\Quantities\SI.h"
#include "..\Quantities\Constants.h"
#include "..\Quantities\Astronomy.h"
#include <stdio.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Principia;
using namespace Principia::Quantities;
using namespace Principia::Constants;
using namespace Principia::SI;
using namespace Principia::Astronomy;

namespace QuantitiesTests {

// TODO(robin): move the logging functions somewhere else, they are generally
// useful.

// The Microsoft equivalent only takes a wchar_t*.
void Log(std::wstring const& message) {
  Logger::WriteMessage(message.c_str());
}
void NewLine() {
  Logger::WriteMessage(L"\n");
}
void LogLine(std::wstring const& message) {
  Log(message);
  NewLine();
}
// The Microsoft equivalent only takes a wchar_t*.
void Assert(bool const test, std::wstring const& message) {
  Assert::IsTrue(test, message.c_str());
}

// The Microsoft equivalent supports errors only for double.
template<typename ValueType, typename ErrorType>
void AssertEqualWithin(ValueType const& left,
                       ValueType const& right,
                       ErrorType const& ε) {
  std::wstring message = L"Should be equal within " + ToString(ε, 3) +
                         L": " + ToString(left) + L" and " + ToString(right) +
                         L".";
  LogLine(message);
  Assert(left == right || Abs(left / right - 1) < ε, message);
  LogLine(L"> Passed!");
}

template<typename ValueType, typename ErrorType>
void AssertNotEqualWithin(ValueType const& left,
                          ValueType const& right,
                          ErrorType const& ε) {
  std::wstring message = L"Should differ by more than " + ToString(ε, 3) +
                         L": " + ToString(left) + L" and " + ToString(right) +
                         L".";
  LogLine(message);
  Assert(Abs(left / right - 1) > ε, message);
  LogLine(L"> Passed!");
}

template<typename D>
void AssertEqual(Quantity<D> const& left,
                 Quantity<D> const& right,
                 Dimensionless const& ε = 1e-15) {
  AssertEqualWithin(left, right, ε);
}

template<typename D>
void AssertNotEqual(Quantity<D> const& left,
                    Quantity<D> const& right,
                    Dimensionless const& ε = 1e-15) {
  AssertNotEqualWithin(left, right, ε);
}

void AssertEqual(Dimensionless const& left,
                 Dimensionless const& right,
                 Dimensionless const& ε = 1e-15) {
  AssertEqualWithin(left, right, ε);
}

void AssertNotEqual(Dimensionless const& left,
                    Dimensionless const& right,
                    Dimensionless const& ε = 1e-15) {
  AssertNotEqualWithin(left, right, ε);
}

TEST_CLASS(QuantitiesTest) {
public:
  TEST_METHOD(DimensionlessComparisons) {
    Dimensionless zero = 0;
    Assert(0 == zero, L"0 == zero was false.");
    Assert(1 != zero, L"1 != zero was false.");
    Assert(1 > zero, L"1 > zero was false.");
    Assert(-1 < zero, L"-1 < zero was false.");
    Assert(zero >= 0, L"zero >= 0 was false.");
    Assert(0 <= zero, L"0 <= zero was false.");

    Assert(!(1 == zero), L"1 == zero was true.");
    Assert(!(0 != zero), L"0 != zero was true.");
    Assert(!(0 > zero), L"0 > zero was true.");
    Assert(!(0 < zero), L"0 < zero was true.");
    Assert(!(zero >= 1), L"zero >= 1 was true.");
    Assert(!(1 <= zero), L"1 <= zero was true.");
  }
  TEST_METHOD(DimensionlessOperations) {
    Dimensionless const number = 1729;
    Dimensionless accumulator = 0;
    AssertNotEqual(1, 0);
    for (int i = 1; i < 10; ++i) {
      accumulator += number;
      AssertEqual(accumulator, i * number);
    }
    for (int i = 1; i < 10; ++i) { accumulator -= number; }
    AssertEqual(accumulator, 0);
  }
  TEST_METHOD(DimensionlessExponentiation) {
    Dimensionless const number   = π - 42;
    Dimensionless positivePowers = 1;
    Dimensionless negativePowers = 1;
    AssertEqual(Dimensionless(1), number.Pow<0>());
    for (int i = 1; i < 10; ++i) {
      positivePowers *= number;
      negativePowers /= number;
      AssertEqual(number.Pow(i), positivePowers);
      AssertEqual(number.Pow(-i), negativePowers);
    }
  }
  TEST_METHOD(PhysicalConstants) {
    AssertEqual(1 / SpeedOfLight.Pow<2>(),
                VacuumPermittivity * VacuumPermeability);
    // The Keplerian approximation for the mass of the sun
    // is fairly accurate.
    AssertEqual(4 * π.Pow<2>() * AstronomicalUnit.Pow<3>() /
                 (GravitationalConstant * JulianYear.Pow<2>()),
               SolarMass, 1e-4);
  }
};
}
