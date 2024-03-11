//
// Created by Cristian Boldrin on 09/03/24.
//

#ifndef TONIC_TRIANGLESAMPLER_H
#define TONIC_TRIANGLESAMPLER_H

#include "hash_table5.hpp"
#include "ankerl/unordered_dense.h"
#include "HeavyEdgesPQ.h"
#include <iostream>
#include <string>
#include <random>

#define edge std::pair<int, int>
#define heavy_edge std::pair<edge, int>
# define MAX_ID_NODE 30000000

// -- heavy edge comparator -> return lightest edge
struct heavy_edge_cmp {
    bool operator()(const heavy_edge &a, const heavy_edge &b) const {
        return a.second < b.second;
    }
};

inline unsigned long long edge_to_id(const int u, const int v) {
    int nu = (u < v ? u : v);
    int nv = (u < v ? v : u);
    return static_cast<unsigned long long>(MAX_ID_NODE) * static_cast<unsigned long long>(nu) +
           static_cast<unsigned long long>(nv);
}

class TriangleSampler {

private:

    emhash5::HashMap<int , emhash5::HashMap<int, bool>> subgraph_;

    // -- oracles
    emhash5::HashMap<int, int> node_oracle_;
    emhash5::HashMap<long, int> edge_id_oracle_;

    // -- sets for storing edges
    edge* waiting_room_;
    FixedSizePQ<heavy_edge, heavy_edge_cmp> heavy_edges_;
    edge* light_edges_sample_;

    std::mt19937 gen_;
    std::uniform_real_distribution<double> dis_;

    // -- member variables
    unsigned long long t_;
    long k_;
    long WR_size_;
    long H_size_;
    long SL_size_;
    bool edge_oracle_flag_ = false;

    // -- current counters
    long WR_cur_ = 0;
    long H_cur_ = 0;
    long SL_cur_ = 0;

    int num_edges_;

    // -- triangle estimates
    double global_triangles_cnt_ = 0.0;
    emhash5::HashMap<int, double> local_triangles_cnt_;

    int get_heaviness(const int u, const int v);

    void add_edge(const int u, const int v, bool det);

    void remove_edge(const int u, const int v);

    void count_triangles(const int u, const int v);

    bool sample_edge(const int u, const int v);

    inline double next_double();


public:

    TriangleSampler(int random_seed, long k, double alpha, double beta);

    ~TriangleSampler();

    void set_edge_oracle(emhash5::HashMap<long, int> &edge_oracle);

    void set_node_oracle(emhash5::HashMap<int, int> &node_oracle);

    void process_edge(const int u, const int v);

    int get_nodes() const;

    int get_edges() const;

    double get_local_triangles(const int u) const;

    emhash5::HashMap<int, double> get_local_counts() const;

    double get_global_triangles() const;

    inline unsigned long long get_edges_processed() const;


};


#endif
