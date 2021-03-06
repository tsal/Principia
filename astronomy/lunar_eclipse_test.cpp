﻿
#include "astronomy/epoch.hpp"
#include "geometry/named_quantities.hpp"
#include "geometry/grassmann.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "integrators/symplectic_runge_kutta_nyström_integrator.hpp"
#include "numerics/root_finders.hpp"
#include "physics/ephemeris.hpp"
#include "physics/solar_system.hpp"
#include "quantities/elementary_functions.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"
#include "testing_utilities/numerics.hpp"

namespace principia {

using astronomy::JulianDate;
using geometry::AngleBetween;
using geometry::Sign;
using integrators::McLachlanAtela1992Order5Optimal;
using numerics::Bisect;
using physics::Ephemeris;
using physics::SolarSystem;
using quantities::Length;
using quantities::si::Day;
using quantities::si::Kilo;
using quantities::si::Metre;
using quantities::si::Milli;
using quantities::si::Minute;
using quantities::si::Radian;
using testing_utilities::AbsoluteError;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Lt;

namespace astronomy {

namespace {

Time const bisection_interval = 10 * Minute;

Sign const U14 = Sign(1);
Sign const U23 = Sign(-1);

char const arrow[] = "-------------------> ";

}  // namespace

class LunarEclipseTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    google::LogToStderr();
    solar_system_1950_.Initialize(
        SOLUTION_DIR / "astronomy" / "gravity_model.proto.txt",
        SOLUTION_DIR / "astronomy" /
            "initial_state_jd_2433282_500000000.proto.txt");
    ephemeris_ = solar_system_1950_.MakeEphemeris(
        /*fitting_tolerance=*/5 * Milli(Metre),
        Ephemeris<ICRFJ2000Equator>::FixedStepParameters(
            McLachlanAtela1992Order5Optimal<Position<ICRFJ2000Equator>>(),
            /*step=*/45 * Minute));
    r_sun_ = solar_system_1950_.mean_radius("Sun");
    r_moon_ = solar_system_1950_.mean_radius("Moon");

    // We follow A. Danjon, see http://eclipse.gsfc.nasa.gov/LEcat5/shadow.html
    // and http://www.imcce.fr/langues/fr/ephemerides/phenomenes/eclipses/lune/.
    // In particular, this means that we must use the equatorial radius for the
    // Earth, not the mean radius.
    r_earth_ = 6378.1363 * Kilo(Metre);
    atmospheric_depth_ = (1.0 / 85.0 - 1.0 / 594.0) * r_earth_;
  }

  // A positive |time_error| means that the actual contact happens after
  // |current_time|.
  void CheckLunarUmbralEclipse(Instant const& current_time,
                               Sign const moon_offset_sign,
                               Angle const& angular_error,
                               Time const& time_error) {
    ephemeris_->Prolong(current_time + bisection_interval);
    auto const sun = solar_system_1950_.massive_body(*ephemeris_, "Sun");
    auto const earth = solar_system_1950_.massive_body(*ephemeris_, "Earth");
    auto const moon = solar_system_1950_.massive_body(*ephemeris_, "Moon");

    // Angle formed by a right circular cone with sides defined by tangent lines
    // between Sun and Earth, and axis running through the centers of each.
    auto const umbral_half_aperture = [this, earth, moon, sun](
        Instant const& t) {
      auto const q_sun =
          ephemeris_->trajectory(sun)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_moon =
          ephemeris_->trajectory(moon)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_earth =
          ephemeris_->trajectory(earth)->EvaluatePosition(t, /*hint=*/nullptr);
      return ArcSin((r_sun_ - (r_earth_ + atmospheric_depth_)) /
                    (q_sun - q_earth).Norm());
    };

    auto const earth_moon_angle = [this,
                                   earth,
                                   moon,
                                   moon_offset_sign,
                                   sun,
                                   umbral_half_aperture](Instant const& t) {
      auto const q_sun =
          ephemeris_->trajectory(sun)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_moon =
          ephemeris_->trajectory(moon)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_earth =
          ephemeris_->trajectory(earth)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const apex_of_moon_locus_at_umbral_contact =
          q_earth +
          Normalize(q_earth - q_sun) *
              (r_earth_ + atmospheric_depth_ + moon_offset_sign * r_moon_) /
              Sin(umbral_half_aperture(t));
      // Angle between Earth and Moon as seen at
      // |apex_of_moon_locus_at_umbral_contact|.
      return AngleBetween(apex_of_moon_locus_at_umbral_contact - q_earth,
                          apex_of_moon_locus_at_umbral_contact - q_moon);
    };

    // We are at the desired contact if the angle between Earth and Moon from
    // the apex of locus of the moon at that contact is the same value as the
    // half-aperture of the umbra (Earth-Sun cone).
    EXPECT_THAT(AbsoluteError(umbral_half_aperture(current_time),
                              earth_moon_angle(current_time)),
                AllOf(Lt(angular_error), Gt(0.5 * angular_error)))
        << NAMED(umbral_half_aperture(current_time)) << ", "
        << NAMED(earth_moon_angle(current_time)) << ", " << NAMED(current_time);

    Instant const& actual_contact_time = Bisect(
        [earth_moon_angle, umbral_half_aperture](Instant const& t) {
          return umbral_half_aperture(t) - earth_moon_angle(t);
        },
        current_time - bisection_interval,
        current_time + bisection_interval);
    EXPECT_THAT(Sign(actual_contact_time - current_time), Eq(Sign(time_error)))
        << NAMED(actual_contact_time - current_time);
    EXPECT_THAT(AbsoluteError(actual_contact_time, current_time),
                AllOf(Lt(Abs(time_error)), Gt(0.9 * Abs(time_error))))
        << NAMED(actual_contact_time) << ", " << NAMED(current_time);
    LOG(INFO) << arrow << AbsoluteError(umbral_half_aperture(current_time),
                                        earth_moon_angle(current_time))
              << " " << actual_contact_time - current_time;
  }

  // A positive |time_error| means that the actual contact happens after
  // |current_time|.
  void CheckLunarPenumbralEclipse(Instant const& current_time,
                                  Sign const moon_offset_sign,
                                  Angle const& angular_error,
                                  Time const& time_error) {
    ephemeris_->Prolong(current_time + bisection_interval);
    auto const sun = solar_system_1950_.massive_body(*ephemeris_, "Sun");
    auto const earth = solar_system_1950_.massive_body(*ephemeris_, "Earth");
    auto const moon = solar_system_1950_.massive_body(*ephemeris_, "Moon");

    auto const penumbral_half_aperture = [this, earth, moon, sun](
        Instant const& t) {
      auto const q_sun =
          ephemeris_->trajectory(sun)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_moon =
          ephemeris_->trajectory(moon)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_earth =
          ephemeris_->trajectory(earth)->EvaluatePosition(t, /*hint=*/nullptr);
      return ArcSin((r_sun_ + r_earth_ + atmospheric_depth_) /
                    (q_sun - q_earth).Norm());
    };

    auto const earth_moon_angle = [this,
                                   earth,
                                   moon,
                                   moon_offset_sign,
                                   sun,
                                   penumbral_half_aperture](Instant const& t) {
      auto const q_sun =
          ephemeris_->trajectory(sun)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_moon =
          ephemeris_->trajectory(moon)->EvaluatePosition(t, /*hint=*/nullptr);
      auto const q_earth =
          ephemeris_->trajectory(earth)->EvaluatePosition(t, /*hint=*/nullptr);

      auto const apex_of_moon_locus_at_penumbral_contact =
          q_earth +
          Normalize(q_sun - q_earth) *
              (r_earth_ + atmospheric_depth_ + moon_offset_sign * r_moon_) /
              Sin(penumbral_half_aperture(t));
      // Angle between Earth and Moon as seen at
      // apex_of_moon_locus_at_penumbral_contact.
      return AngleBetween(apex_of_moon_locus_at_penumbral_contact - q_earth,
                          apex_of_moon_locus_at_penumbral_contact - q_moon);
    };

    // We are at the desired contact if the angle between Earth and Moon from
    // the apex of locus of the moon at that contact is the same value as the
    // half-aperture of the penumbra.
    EXPECT_THAT(AbsoluteError(penumbral_half_aperture(current_time),
                              earth_moon_angle(current_time)),
                AllOf(Lt(angular_error), Gt(0.5 * angular_error)))
        << NAMED(penumbral_half_aperture(current_time)) << ", "
        << NAMED(earth_moon_angle(current_time)) << ", " << NAMED(current_time);

    Instant const& actual_contact_time = Bisect(
        [earth_moon_angle, penumbral_half_aperture](Instant const& t) {
          return penumbral_half_aperture(t) - earth_moon_angle(t);
        },
        current_time - bisection_interval,
        current_time + bisection_interval);
    EXPECT_THAT(Sign(actual_contact_time - current_time), Eq(Sign(time_error)))
        << NAMED(actual_contact_time - current_time);
    EXPECT_THAT(AbsoluteError(actual_contact_time, current_time),
                AllOf(Lt(Abs(time_error)), Gt(0.9 * Abs(time_error))))
        << NAMED(actual_contact_time) << ", " << NAMED(current_time);
    LOG(INFO) << arrow
              << Sign(penumbral_half_aperture(current_time) -
                      earth_moon_angle(current_time)) *
                     AbsoluteError(penumbral_half_aperture(current_time),
                                   earth_moon_angle(current_time))
              << " " << actual_contact_time - current_time;
  }

  static SolarSystem<ICRFJ2000Equator> solar_system_1950_;
  static std::unique_ptr<Ephemeris<ICRFJ2000Equator>> ephemeris_;
  static Length r_sun_;
  static Length r_earth_;
  static Length r_moon_;
  static Length atmospheric_depth_;
};

SolarSystem<ICRFJ2000Equator> LunarEclipseTest::solar_system_1950_;
std::unique_ptr<Ephemeris<ICRFJ2000Equator>> LunarEclipseTest::ephemeris_;
Length LunarEclipseTest::r_sun_;
Length LunarEclipseTest::r_earth_;
Length LunarEclipseTest::r_moon_;
Length LunarEclipseTest::atmospheric_depth_;

#if !defined(_DEBUG)

TEST_F(LunarEclipseTest, Year1950) {
  // Times are TDB Julian Day for 1950-04-02.
  auto P1 = JulianDate(2433374.25788409);  // 18:10:49 UT
  auto U1 = JulianDate(2433374.29850909);  // 19:09:19
  auto U2 = JulianDate(2433374.354979);    // 20:30:38
  auto U3 = JulianDate(2433374.37367113);  // 20:57:33
  auto U4 = JulianDate(2433374.43016419);  // 22:18:54
  auto P4 = JulianDate(2433374.47075446);  // 23:17:21

  CheckLunarPenumbralEclipse(P1, U14, 2e-5 * Radian, 28 * Second);
  CheckLunarUmbralEclipse(U1, U14,    2e-5 * Radian, 30 * Second);
  CheckLunarUmbralEclipse(U2, U23,    2e-5 * Radian, 39 * Second);
  CheckLunarUmbralEclipse(U3, U23,    9e-6 * Radian, 21 * Second);
  CheckLunarUmbralEclipse(U4, U14,    2e-5 * Radian, 28 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 2e-5 * Radian, 30 * Second);

  // Times are TDB Julian Day for 1950-09-26.
  P1 = JulianDate(2433550.55712016);  // 01:21:43 UT
  U1 = JulianDate(2433550.60578913);  // 02:31:48
  U2 = JulianDate(2433550.66325441);  // 03:54:33
  U3 = JulianDate(2433550.69399515);  // 04:38:49
  U4 = JulianDate(2433550.75144885);  // 06:01:33
  P4 = JulianDate(2433550.800222);    // 07:11:47

  CheckLunarPenumbralEclipse(P1, U14, 2e-5 * Radian, 34 * Second);
  CheckLunarUmbralEclipse(U1, U14,    3e-5 * Radian, 36 * Second);
  CheckLunarUmbralEclipse(U2, U23,    3e-5 * Radian, 42 * Second);
  CheckLunarUmbralEclipse(U3, U23,    2e-5 * Radian, 29 * Second);
  CheckLunarUmbralEclipse(U4, U14,    3e-5 * Radian, 34 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 2e-5 * Radian, 36 * Second);
}

TEST_F(LunarEclipseTest, Year1951) {
  // Times are TDB Julian Day for 1951-03-23.
  auto P1 = JulianDate(2433728.86842806);  // 08:50:50
  auto P4 = JulianDate(2433729.01725909);  // 12:24:19

  CheckLunarPenumbralEclipse(P1, U14, 9e-6 * Radian, 30 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 8e-6 * Radian, 25 * Second);

  // Times are TDB Julian Day for 1951-09-15.
  P1 = JulianDate(2433904.93736321);  // 10:29:16
  P4 = JulianDate(2433905.1002799);   // 14:23:52

  CheckLunarPenumbralEclipse(P1, U14, 9e-6 * Radian, 28 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 8e-6 * Radian, 23 * Second);
}

TEST_F(LunarEclipseTest, Year1952) {
  // Times are TDB Julian Day for 1952-02-11 (or 10 for P1).
  auto P1 = JulianDate(2434053.42282623);  // P1 = 22:08:20 UT
  auto U1 = JulianDate(2434053.50334705);  // U1 = 00:04:17
  auto U4 = JulianDate(2434053.55203917);  // U4 = 01:14:24
  auto P4 = JulianDate(2434053.63249055);  // P4 = 03:10:15

  CheckLunarPenumbralEclipse(P1, U14, 1e-5 * Radian, 30 * Second);
  CheckLunarUmbralEclipse(U1, U14,    4e-6 * Radian, 19 * Second);
  CheckLunarUmbralEclipse(U4, U14,    1e-5 * Radian, 50 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 2e-5 * Radian, 40 * Second);

  // Times are TDB Julian Day for 1952-08-05.
  P1 = JulianDate(2434230.22830075);  // P1 = 17:28:13 UT
  U1 = JulianDate(2434230.27385631);  // U1 = 18:33:49
  U4 = JulianDate(2434230.37606695);  // U4 = 21:01:00
  P4 = JulianDate(2434230.42161093);  // P4 = 22:06:35

  CheckLunarPenumbralEclipse(P1, U14, 8e-6 * Radian, 18 * Second);
  CheckLunarUmbralEclipse(U1, U14,    9e-6 * Radian, 18 * Second);
  CheckLunarUmbralEclipse(U4, U14,    2e-5 * Radian, 25 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 2e-5 * Radian, 26 * Second);
}

#if 0
TEST_F(LunarEclipseTest, Year2000) {
  // Times are TDB Julian Day for 2000-01-21.
  auto P1 = JulianDate(2451564.58715491);  // 02:04:26 UT
  auto U1 = JulianDate(2451564.62701602);  // 03:01:50
  auto U2 = JulianDate(2451564.67089334);  // 04:05:01
  auto U3 = JulianDate(2451564.72435399);  // 05:22:00
  auto U4 = JulianDate(2451564.76820815);  // 06:25:09
  auto P4 = JulianDate(2451564.80812714);  // 07:22:38

  CheckLunarPenumbralEclipse(P1, U14, 8e-5 * Radian, -167 * Second);
  CheckLunarUmbralEclipse(U1, U14,    2e-4 * Radian, -164 * Second);
  CheckLunarUmbralEclipse(U2, U23,    3e-4 * Radian, -167 * Second);
  CheckLunarUmbralEclipse(U3, U23,    2e-4 * Radian, -160 * Second);
  CheckLunarUmbralEclipse(U4, U14,    2e-4 * Radian, -161 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 8e-5 * Radian, -160 * Second);
}

TEST_F(LunarEclipseTest, Year2048) {
  // Times are TDB Julian Day for 2048-01-01.
  auto P1 = JulianDate(2469076.66235167);  // 03:52:39 UT
  auto U1 = JulianDate(2469076.71279148);  // 05:05:17
  auto U2 = JulianDate(2469076.76776833);  // 06:24:27
  auto U3 = JulianDate(2469076.80661092);  // 07:20:23
  auto U4 = JulianDate(2469076.86158778);  // 08:39:33
  auto P4 = JulianDate(2469076.91195815);  // 09:52:05

  CheckLunarPenumbralEclipse(P1, U14, 2e-4 * Radian, -338 * Second);
  CheckLunarUmbralEclipse(U1, U14,    3e-4 * Radian, -336 * Second);
  CheckLunarUmbralEclipse(U2, U23,    3e-4 * Radian, -335 * Second);
  CheckLunarUmbralEclipse(U3, U23,    3e-4 * Radian, -337 * Second);
  CheckLunarUmbralEclipse(U4, U14,    3e-4 * Radian, -336 * Second);
  CheckLunarPenumbralEclipse(P4, U14, 2e-4 * Radian, -335 * Second);
}
#endif

#endif

}  // namespace astronomy
}  // namespace principia
