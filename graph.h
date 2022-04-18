#pragma once

#include <cstdlib>
#include <vector>

#include "ranges.h"

namespace graph {

using VertexId = size_t;
using EdgeId = size_t;

template <typename Weight>
struct Edge {
    VertexId from;
    VertexId to;
    Weight weight;
};

template <typename Weight>
class DirectedWeightedGraph {
private:
    using IncidenceList = std::vector<EdgeId>;
    using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;

public:
    DirectedWeightedGraph() = default;

public:
    explicit DirectedWeightedGraph(size_t vertex_count);

    EdgeId AddEdge(const Edge<Weight>& edge);

    [[nodiscard]] size_t GetVertexCount() const;

    [[nodiscard]] size_t GetEdgeCount() const;

    const Edge<Weight>& GetEdge(EdgeId edge_id) const;

    [[nodiscard]] IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

    std::vector<Edge<Weight>>& GetEdges();

    [[nodiscard]] std::vector<std::vector<EdgeId>>& GetIncidenceList();

private:
    std::vector<Edge<Weight>> edges_;
    std::vector<IncidenceList> incidence_lists_;
};

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count) : incidence_lists_(vertex_count) {}

template <typename Weight>
EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight>& edge) {
    edges_.push_back(edge);
    const EdgeId id = edges_.size() - 1;
    incidence_lists_.at(edge.from).push_back(id);
    return id;
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
    return incidence_lists_.size();
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
    return edges_.size();
}

template <typename Weight>
const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
    return edges_.at(edge_id);
}

template <typename Weight>
typename DirectedWeightedGraph<Weight>::IncidentEdgesRange DirectedWeightedGraph<Weight>::GetIncidentEdges(
    VertexId vertex) const {
    return ranges::AsRange(incidence_lists_.at(vertex));
}

template <typename Weight>
std::vector<Edge<Weight>>& DirectedWeightedGraph<Weight>::GetEdges() {
    return edges_;
}

template <typename Weight>
std::vector<std::vector<EdgeId>>& DirectedWeightedGraph<Weight>::GetIncidenceList() {
    return incidence_lists_;
}
}  // namespace graph