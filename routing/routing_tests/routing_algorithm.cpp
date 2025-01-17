#include "routing/routing_tests/routing_algorithm.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_graph.hpp"

#include "routing/maxspeeds.hpp"
#include "routing/routing_helpers.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace routing_test
{
void UndirectedGraph::AddEdge(Vertex u, Vertex v, Weight w)
{
  m_adjs[u].emplace_back(v, w);
  m_adjs[v].emplace_back(u, w);
}

size_t UndirectedGraph::GetNodesNumber() const
{
  return m_adjs.size();
}

void UndirectedGraph::GetIngoingEdgesList(Vertex const & v, std::vector<SimpleEdge> & adj)
{
  GetAdjacencyList(v, adj);
}

void UndirectedGraph::GetOutgoingEdgesList(Vertex const & v, std::vector<SimpleEdge> & adj)
{
  GetAdjacencyList(v, adj);
}

double UndirectedGraph::HeuristicCostEstimate(Vertex const & v, Vertex const & w)
{
  return 0.0;
}

void UndirectedGraph::GetAdjacencyList(Vertex v, std::vector<Edge> & adj) const
{
  adj.clear();
  auto const it = m_adjs.find(v);
  if (it != m_adjs.cend())
    adj = it->second;
}

void DirectedGraph::AddEdge(Vertex from, Vertex to, Weight w)
{
  m_outgoing[from].emplace_back(to, w);
  m_ingoing[to].emplace_back(from, w);
}

void DirectedGraph::GetIngoingEdgesList(Vertex const & v, std::vector<Edge> & adj)
{
  adj = m_ingoing[v];
}

void DirectedGraph::GetOutgoingEdgesList(Vertex const & v, std::vector<Edge> & adj)
{
  adj = m_outgoing[v];
}
}  // namespace routing_tests

namespace routing
{
using namespace std;

namespace
{
inline double TimeBetweenSec(geometry::PointWithAltitude const & j1,
                             geometry::PointWithAltitude const & j2, double speedMPS)
{
  ASSERT(speedMPS > 0.0, ());
  ASSERT_NOT_EQUAL(j1.GetAltitude(), geometry::kInvalidAltitude, ());
  ASSERT_NOT_EQUAL(j2.GetAltitude(), geometry::kInvalidAltitude, ());

  double const distanceM = mercator::DistanceOnEarth(j1.GetPoint(), j2.GetPoint());
  double const altitudeDiffM =
      static_cast<double>(j2.GetAltitude()) - static_cast<double>(j1.GetAltitude());
  return sqrt(distanceM * distanceM + altitudeDiffM * altitudeDiffM) / speedMPS;
}

/// A class which represents an weighted edge used by RoadGraph.
class WeightedEdge
{
public:
  WeightedEdge(geometry::PointWithAltitude const & target, double weight)
    : target(target), weight(weight)
  {
  }

  inline geometry::PointWithAltitude const & GetTarget() const { return target; }
  inline double GetWeight() const { return weight; }

private:
  geometry::PointWithAltitude const target;
  double const weight;
};

using Algorithm = AStarAlgorithm<geometry::PointWithAltitude, WeightedEdge, double>;

/// A wrapper around IRoadGraph, which makes it possible to use IRoadGraph with astar algorithms.
class RoadGraph : public Algorithm::Graph
{
public:

  explicit RoadGraph(IRoadGraph const & roadGraph)
    : m_roadGraph(roadGraph), m_maxSpeedMPS(KMPH2MPS(roadGraph.GetMaxSpeedKMpH()))
  {}

  void GetOutgoingEdgesList(Vertex const & v, std::vector<Edge> & adj) override
  {
    IRoadGraph::EdgeVector edges;
    m_roadGraph.GetOutgoingEdges(v, edges);

    adj.clear();
    adj.reserve(edges.size());

    for (auto const & e : edges)
    {
      ASSERT_EQUAL(v, e.GetStartJunction(), ());

      double const speedMPS = KMPH2MPS(
          m_roadGraph.GetSpeedKMpH(e, {true /* forward */, false /* in city */, Maxspeed()}));
      adj.emplace_back(e.GetEndJunction(), TimeBetweenSec(e.GetStartJunction(), e.GetEndJunction(), speedMPS));
    }
  }

  void GetIngoingEdgesList(Vertex const & v, std::vector<Edge> & adj) override
  {
    IRoadGraph::EdgeVector edges;
    m_roadGraph.GetIngoingEdges(v, edges);

    adj.clear();
    adj.reserve(edges.size());

    for (auto const & e : edges)
    {
      ASSERT_EQUAL(v, e.GetEndJunction(), ());

      double const speedMPS = KMPH2MPS(
          m_roadGraph.GetSpeedKMpH(e, {true /* forward */, false /* in city */, Maxspeed()}));
      adj.emplace_back(e.GetStartJunction(), TimeBetweenSec(e.GetStartJunction(), e.GetEndJunction(), speedMPS));
    }
  }

  double HeuristicCostEstimate(Vertex const & v, Vertex const & w) override
  {
    return TimeBetweenSec(v, w, m_maxSpeedMPS);
  }

private:
  IRoadGraph const & m_roadGraph;
  double const m_maxSpeedMPS;
};

TestAStarBidirectionalAlgo::Result Convert(Algorithm::Result value)
{
  switch (value)
  {
  case Algorithm::Result::OK: return TestAStarBidirectionalAlgo::Result::OK;
  case Algorithm::Result::NoPath: return TestAStarBidirectionalAlgo::Result::NoPath;
  case Algorithm::Result::Cancelled: return TestAStarBidirectionalAlgo::Result::Cancelled;
  }

  UNREACHABLE();
  return TestAStarBidirectionalAlgo::Result::NoPath;
}
}  // namespace

string DebugPrint(TestAStarBidirectionalAlgo::Result const & value)
{
  switch (value)
  {
  case TestAStarBidirectionalAlgo::Result::OK:
    return "OK";
  case TestAStarBidirectionalAlgo::Result::NoPath:
    return "NoPath";
  case TestAStarBidirectionalAlgo::Result::Cancelled:
    return "Cancelled";
  }

  UNREACHABLE();
  return string();
}

// *************************** AStar-bidirectional routing algorithm implementation ***********************
TestAStarBidirectionalAlgo::Result TestAStarBidirectionalAlgo::CalculateRoute(
    IRoadGraph const & graph, geometry::PointWithAltitude const & startPos,
    geometry::PointWithAltitude const & finalPos,
    RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path)
{
  RoadGraph roadGraph(graph);
  base::Cancellable cancellable;
  Algorithm::Params<> params(roadGraph, startPos, finalPos, {} /* prevRoute */,
                             cancellable);

  Algorithm::Result const res = Algorithm().FindPathBidirectional(params, path);
  return Convert(res);
}
}  // namespace routing
