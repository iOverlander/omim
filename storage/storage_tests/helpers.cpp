#include "testing/testing.hpp"

#include "storage/storage_tests/helpers.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

#include <memory>

namespace storage
{
std::unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
{
  return CountryInfoReader::CreateCountryInfoReader(GetPlatform());
}

bool AlmostEqualRectsAbs(const m2::RectD & r1, const m2::RectD & r2)
{
  double constexpr kEpsilon = 1e-2;
  return base::AlmostEqualAbs(r1.maxX(), r2.maxX(), kEpsilon)
      && base::AlmostEqualAbs(r1.maxY(), r2.maxY(), kEpsilon)
      && base::AlmostEqualAbs(r1.minX(), r2.minX(), kEpsilon)
      && base::AlmostEqualAbs(r1.minY(), r2.minY(), kEpsilon);
}
}  // namespace storage
