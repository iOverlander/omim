#include "followed_polyline.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>

namespace routing
{
using namespace std;
using Iter = routing::FollowedPolyline::Iter;

Iter FollowedPolyline::Begin() const
{
  ASSERT(IsValid(), ());
  return Iter(m_poly.Front(), 0);
}

Iter FollowedPolyline::End() const
{
  ASSERT(IsValid(), ());
  return Iter(m_poly.Back(), m_poly.GetSize() - 1);
}

Iter FollowedPolyline::GetIterToIndex(size_t index) const
{
  ASSERT(IsValid(), ());
  ASSERT_LESS(index, m_poly.GetSize(), ());

  return Iter(m_poly.GetPoint(index), index);
}

double FollowedPolyline::GetDistanceM(Iter const & it1, Iter const & it2) const
{
  ASSERT(IsValid(), ());
  ASSERT(it1.IsValid() && it2.IsValid(), ());
  ASSERT_LESS_OR_EQUAL(it1.m_ind, it2.m_ind, ());
  ASSERT_LESS(it1.m_ind, m_poly.GetSize(), ());
  ASSERT_LESS(it2.m_ind, m_poly.GetSize(), ());

  if (it1.m_ind == it2.m_ind)
    return mercator::DistanceOnEarth(it1.m_pt, it2.m_pt);

  return (mercator::DistanceOnEarth(it1.m_pt, m_poly.GetPoint(it1.m_ind + 1)) +
          m_segDistance[it2.m_ind - 1] - m_segDistance[it1.m_ind] +
          mercator::DistanceOnEarth(m_poly.GetPoint(it2.m_ind), it2.m_pt));
}

double FollowedPolyline::GetTotalDistanceMeters() const
{
  if (!IsValid())
  {
    ASSERT(IsValid(), ());
    return 0;
  }
  return m_segDistance.back();
}

double FollowedPolyline::GetDistanceFromStartMeters() const
{
  if (!IsValid() || !m_current.IsValid())
  {
    ASSERT(IsValid(), ());
    ASSERT(m_current.IsValid(), ());
    return 0;
  }

  return (m_current.m_ind > 0 ? m_segDistance[m_current.m_ind - 1] : 0.0) +
         mercator::DistanceOnEarth(m_current.m_pt, m_poly.GetPoint(m_current.m_ind));
}

double FollowedPolyline::GetDistanceToEndMeters() const
{
  return GetTotalDistanceMeters() - GetDistanceFromStartMeters();
}

void FollowedPolyline::Swap(FollowedPolyline & rhs)
{
  m_poly.Swap(rhs.m_poly);
  m_segDistance.swap(rhs.m_segDistance);
  m_segProj.swap(rhs.m_segProj);
  swap(m_current, rhs.m_current);
  swap(m_nextCheckpointIndex, rhs.m_nextCheckpointIndex);
}

void FollowedPolyline::Update()
{
  size_t n = m_poly.GetSize();
  ASSERT_GREATER(n, 1, ());
  --n;

  m_segDistance.clear();
  m_segDistance.reserve(n);
  m_segProj.clear();
  m_segProj.reserve(n);

  double dist = 0.0;
  for (size_t i = 0; i < n; ++i)
  {
    m2::PointD const & p1 = m_poly.GetPoint(i);
    m2::PointD const & p2 = m_poly.GetPoint(i + 1);

    dist += mercator::DistanceOnEarth(p1, p2);

    m_segDistance.emplace_back(dist);
    m_segProj.emplace_back(p1, p2);
  }

  m_current = Iter(m_poly.Front(), 0);
}

template <class DistanceFn>
Iter FollowedPolyline::GetBestProjection(m2::RectD const & posRect,
                                         DistanceFn const & distFn) const
{
  CHECK_EQUAL(m_segProj.size() + 1, m_poly.GetSize(), ());
  // At first trying to find a projection to two closest route segments of route which is close
  // enough to |posRect| center. If |m_current| is right before intermediate point we can get |closestIter|
  // right after intermediate point (in next subroute).
  size_t const hoppingBorderIdx = min(m_segProj.size(), m_current.m_ind + 2);
  Iter const closestIter =
    GetClosestProjectionInInterval(posRect, distFn, m_current.m_ind, hoppingBorderIdx);
  if (closestIter.IsValid())
    return closestIter;

  // If a projection to the two closest route segments is not found tries to find projection to other route
  // segments of current subroute.
  return GetClosestProjectionInInterval(posRect, distFn, hoppingBorderIdx, m_nextCheckpointIndex);
}

FollowedPolyline::UpdatedProjection FollowedPolyline::GetBestMatchingProjection(m2::RectD const & posRect) const
{
  CHECK_EQUAL(m_segProj.size() + 1, m_poly.GetSize(), ());
  // At first trying to find a projection to two closest route segments of route which is close
  // enough to |posRect| center. If |m_current| is right before intermediate point we can get |closestIter|
  // right after intermediate point (in next subroute).
  size_t const hoppingBorderIdx = min(m_segProj.size(), m_current.m_ind + 3);
  auto const res = GetClosestMatchingProjectionInInterval(posRect, m_current.m_ind, hoppingBorderIdx);
  if (res.m_iter.IsValid() || res.m_closerToUnmatching)
    return UpdatedProjection{res.m_iter, res.m_closerToUnmatching};

  // If a projection to the 3 closest route segments is not found tries to find projection to other route
  // segments of current subroute.
  return GetClosestMatchingProjectionInInterval(posRect, hoppingBorderIdx, m_nextCheckpointIndex);
}

FollowedPolyline::UpdatedProjectionInfo FollowedPolyline::UpdateMatchingProjection(m2::RectD const & posRect)
{
  ASSERT(m_current.IsValid(), ());
  ASSERT_LESS(m_current.m_ind, m_poly.GetSize() - 1, ());

  auto const res = GetBestMatchingProjection(posRect);

  if (res.m_iter.IsValid())
    m_current = res.m_iter;
  return UpdatedProjectionInfo{res.m_iter.IsValid(), res.m_closerToUnmatching};
}

Iter FollowedPolyline::UpdateProjection(m2::RectD const & posRect)
{
  ASSERT(m_current.IsValid(), ());
  ASSERT_LESS(m_current.m_ind, m_poly.GetSize() - 1, ());

  Iter res;
  m2::PointD const currPos = posRect.Center();
  res = GetBestProjection(posRect, [&](Iter const & it)
  {
    return mercator::DistanceOnEarth(it.m_pt, currPos);
  });

  if (res.IsValid())
    m_current = res;
  return res;
}

void FollowedPolyline::SetUnmatchingSegmentIndexes(vector<size_t> && unmatchingSegmentIndexes)
{
  ASSERT(is_sorted(unmatchingSegmentIndexes.cbegin(), unmatchingSegmentIndexes.cend()), ());
  m_unmatchingSegmentIndexes = move(unmatchingSegmentIndexes);
}

double FollowedPolyline::GetDistFromCurPointToRoutePointMerc() const
{
  if (!m_current.IsValid())
    return 0.0;

  return m_poly.GetPoint(m_current.m_ind).Length(m_current.m_pt);
}

double FollowedPolyline::GetDistFromCurPointToRoutePointMeters() const
{
  if (!m_current.IsValid())
    return 0.0;

  return mercator::DistanceOnEarth(m_poly.GetPoint(m_current.m_ind), m_current.m_pt);
}

void FollowedPolyline::GetCurrentDirectionPoint(m2::PointD & pt, double toleranceM) const
{
  ASSERT(IsValid(), ());
  size_t currentIndex = min(m_current.m_ind + 1, m_poly.GetSize() - 1);
  m2::PointD point = m_poly.GetPoint(currentIndex);
  for (; currentIndex < m_poly.GetSize() - 1; point = m_poly.GetPoint(++currentIndex))
  {
    if (mercator::DistanceOnEarth(point, m_current.m_pt) > toleranceM)
      break;
  }

  pt = point;
}

FollowedPolyline::UpdatedProjection FollowedPolyline::GetClosestMatchingProjectionInInterval(m2::RectD const & posRect,
                                                          size_t startIdx, size_t endIdx) const
{
  CHECK_LESS_OR_EQUAL(endIdx, m_segProj.size(), ());
  CHECK_LESS_OR_EQUAL(startIdx, endIdx, ());
  Iter nearestIter;
  double minDist = std::numeric_limits<double>::max();
  double minDistUnmatching = minDist;

  m2::PointD const currPos = posRect.Center();

  for (size_t i = startIdx; i < endIdx; ++i)
  {
    m2::PointD const pt = m_segProj[i].ClosestPointTo(currPos);

    if (!posRect.IsPointInside(pt))
      continue;

    Iter it(pt, i);
    double const dp = mercator::DistanceOnEarth(it.m_pt, currPos);
    if (dp >= minDistUnmatching && dp >= minDist)
      continue;

    if (!std::binary_search(m_unmatchingSegmentIndexes.begin(), m_unmatchingSegmentIndexes.end(), it.m_ind))
    {
      if (minDist > dp) // Overwrites the best matching for matched segment.
      {
        minDist = dp;
        nearestIter = it;
      }
    }
    else
    {
      if (minDistUnmatching > dp)
        minDistUnmatching = dp;
    }
  }
  return UpdatedProjection{nearestIter, minDistUnmatching < minDist /* m_closerToUnmatching */};
}
}  //  namespace routing
