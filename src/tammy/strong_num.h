// Copyright 2016 Pacific Northwest National Laboratory

#ifndef TAMMY_STRONGNUM_H__
#define TAMMY_STRONGNUM_H__

#include <limits>
#include <iosfwd>
#include <functional>

namespace tammy {


/**
 * @todo Check the narrow cast implementation in:
 *  http://stackoverflow.com/questions/17860657/well-defined-narrowing-cast
 */

#define DEBUG

template<typename Target, typename Source,
         typename = std::enable_if_t<std::is_arithmetic<Source>::value>,
         typename = std::enable_if_t<std::is_arithmetic<Target>::value>>
Target checked_cast(const Source& s) {
#if defined(DEBUG)
  auto r = static_cast<Target>(s);
  assert(static_cast<Source>(r) == s);
  return r;  
#else
  return Target{static_cast<Target>(s)};
#endif
}

template<typename Target, typename Source>
Target strongnum_cast(Source s) {
  return checked_cast<Target>(s);
}

/**
 * @brief Strongly typed wrapper for a numeric type.
 *
 * This class provides a strongly typed alias that cannot be implicitly converted to another type. To define a new StrongNum wrapper StrongInt to, say, int, we do the following:
 *
 * @code
 * class StrongIntSpace;
 * using StrongInt = StrongNum<StrongIntSpace, int>;
 * @endcode
 *
 * Checked casts are to be used to convert between types and possibly check the conversions in debug mode.
 *
 * @tparam Space Unique type name
 * @tparam T Numeric typed being wrapper
 */
template<typename Space, typename T>
struct StrongNum {
  using value_type = T;
  using NumType =  StrongNum<Space, T>;

  StrongNum() = default;
  StrongNum(const StrongNum<Space, T>&) = default;
  StrongNum& operator=(const StrongNum&) = default;
  ~StrongNum() = default;

  template<typename T2>
  explicit StrongNum(const T2 v1): v(checked_cast<T>(v1)) {}

  NumType& operator=(const T& t)        { v = t; return *this; }

  NumType& operator+=(const T& t)       { v += t; return *this; }
  NumType& operator+=(const NumType& d) { v += d.v; return *this; }
  NumType& operator-=(const T& t)       { v -= t; return *this; }
  NumType& operator-=(const NumType& d) { v -= d.v; return *this; }
  NumType& operator*=(const T& t)       { v *= t; return *this; }
  NumType& operator*=(const NumType& d) { v *= d.v; return *this; }
  NumType& operator/=(const T& t)       { v /= t; return *this; }
  NumType& operator/=(const NumType& d) { v /= d.v; return *this; }
  NumType& operator^=(const NumType& d) { v ^= d.v; return *this; }

  NumType& operator++()       { v += 1; return *this; }
  NumType  operator++(int)       { v += 1; return *this; }
  NumType& operator--()       { v -= 1; return *this; }

  NumType operator+(const NumType& d) const { return NumType(v+d.v); }
  NumType operator-(const NumType& d) const { return NumType(v-d.v); }
  NumType operator*(const NumType& d) const { return NumType(v*d.v); }
  NumType operator/(const NumType& d) const { return NumType(v/d.v); }
  NumType operator%(const NumType& d) const { return NumType(v%d.v); }

  NumType operator+(const T& t)       const { return NumType(v+t); }
  NumType operator-(const T& t)       const { return NumType(v-t); }
  NumType operator*(const T& t)       const { return NumType(v*t); }
  NumType operator/(const T& t)       const { return NumType(v/t); }
  NumType operator%(const T& t)       const { return NumType(v%t); }

  bool operator==(const NumType& d) const { return v == d.v; }
  bool operator!=(const NumType& d) const { return v != d.v; }
  bool operator>=(const NumType& d) const { return v >= d.v; }
  bool operator<=(const NumType& d) const { return v <= d.v; }
  bool operator> (const NumType& d) const { return v >  d.v; }
  bool operator< (const NumType& d) const { return v <  d.v; }

  bool operator==(const T& t) const { return v == t; }
  bool operator!=(const T& t) const { return v != t; }
  bool operator>=(const T& t) const { return v >= t; }
  bool operator<=(const T& t) const { return v <= t; }
  bool operator> (const T& t) const { return v >  t; }
  bool operator< (const T& t) const { return v <  t; }

  T value() const { return v; }
  T& value() { return v; }

 private:
  T v;
};

template<typename Space, typename Int, typename Int2>
StrongNum<Space,Int> operator * (Int2 value, StrongNum<Space,Int> sint) {
  return StrongNum<Space,Int>{checked_cast<Int>(sint.value() * value)};
}


template<typename S, typename T>
std::ostream& operator<<(std::ostream& os, const StrongNum<S, T>& s) {
  return os << s.value();
}

template<typename S, typename T>
std::istream& operator>>(std::istream& is, StrongNum<S, T>& s) {
  is >> s.value();
  return is;
}

}  // namespace tammy

#endif  // TAMMY_STRONGNUM_H_

