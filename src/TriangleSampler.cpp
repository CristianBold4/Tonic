//
// Created by Cristian Boldrin on 09/03/24.
//

#include "TriangleSampler.h"

TriangleSampler::TriangleSampler(int random_seed, long k, double alpha, double beta) :
        gen_(random_seed), dis_(0.0, 1.0), t_(0), k_(k) {

    printf("Starting Tonic Algo - alpha %.3f, beta = %.3f | Memory Budget = %ld\n", alpha, beta, k);
    WR_size_ = (long) (k_ * alpha);
    H_size_ = (long) ((k_ - WR_size_) * beta);
    SL_size_ = k_ - WR_size_ - H_size_;
    waiting_room_ = new edge[WR_size_];
    heavy_edges_ = FixedSizePQ<heavy_edge, heavy_edge_cmp>(H_size_);
    light_edges_sample_ = new edge[SL_size_];
    num_edges_ = 0;
    printf("WR size = %ld, H size = %ld, SL size = %ld\n", WR_size_, H_size_, SL_size_);
    subgraph_ = emhash5::HashMap<int, emhash5::HashMap<int, bool>>();
}


TriangleSampler::~TriangleSampler() {
    delete[] waiting_room_;
    delete[] light_edges_sample_;
    subgraph_.clear();
}

void TriangleSampler::set_edge_oracle(emhash5::HashMap<long, int> &edge_oracle) {
    edge_id_oracle_ = edge_oracle;
    edge_oracle_flag_ = true;
}

void TriangleSampler::set_node_oracle(emhash5::HashMap<int, int> &node_oracle) {
    node_oracle_ = node_oracle;
}

int TriangleSampler::get_heaviness(const int u, const int v) {
    if (edge_oracle_flag_) {
        auto id_it = edge_id_oracle_.find(edge_to_id(u, v));
        if (id_it != edge_id_oracle_.end()) {
            return id_it->second;
        } else {
            return -1;
        }
    } else {
        auto u_it = node_oracle_.find(u);
        if (u_it != node_oracle_.end()) {
            auto v_it = node_oracle_.find(v);
            if (v_it != node_oracle_.end()) {
                return std::min(u_it->second, v_it->second);
            }
        }
        return -1;
    }
}

inline double TriangleSampler::next_double() {
    return dis_(gen_);
}

int TriangleSampler::get_num_nodes() const {
    return (int) subgraph_.size();
}

int TriangleSampler::get_num_edges() const {
    return num_edges_;
}

void TriangleSampler::get_nodes(std::vector<int> &nodes) const {
    nodes.clear();
    for (const auto &it: subgraph_) {
        nodes.push_back(it.first);
    }
}

void TriangleSampler::get_local_nodes(std::vector<int> &nodes) const {
    nodes.clear();
    for (const auto &it: local_triangles_cnt_) {
        nodes.push_back(it.first);
    }
}

void TriangleSampler::add_edge(const int u, const int v, bool det) {
    num_edges_++;
    subgraph_[u].emplace_unique(v, det);
    subgraph_[v].emplace_unique(u, det);

}

void TriangleSampler::remove_edge(const int u, const int v) {
    num_edges_--;
    subgraph_[u].erase(v);
    subgraph_[v].erase(u);
}

inline unsigned long long TriangleSampler::get_edges_processed() const {
    return t_;
}

double TriangleSampler::get_global_triangles() const {
    return global_triangles_cnt_;
}

double TriangleSampler::get_local_triangles(const int u) const {
    auto u_it = local_triangles_cnt_.find(u);
    if (u_it != local_triangles_cnt_.end()) {
        return u_it->second;
    } else {
        return 0.0;
    }
}

void TriangleSampler::count_triangles(const int src, const int dst) {
    emhash5::HashMap<int, bool> *u_neighs, *v_neighs;
    auto u_it = subgraph_.find(src);
    if (u_it == subgraph_.end()) {
        return;
    }
    u_neighs = &u_it->second;
    int du = (int) u_neighs->size();

    auto v_it = subgraph_.find(dst);
    if (v_it == subgraph_.end()) {
        return;
    }
    v_neighs = &v_it->second;
    int dv = (int) v_neighs->size();
    int u = src;
    int v = dst;

    if (du > dv) {
        v = src;
        u = dst;
        emhash5::HashMap<int, bool> *u_neighs_tmp = u_neighs;
        u_neighs = v_neighs;
        v_neighs = u_neighs_tmp;
    }

    double cum_cnt = 0.0;

    for (const auto &it: *u_neighs) {
        int w = it.first;
        auto vw_it = v_neighs->find(w);
        if (vw_it != v_neighs->end()) {
            // -- triangle {u, v, w} discovered
            double increment_T = 1.0;
            if (SL_cur_ > SL_size_) {
                bool vw_light = !vw_it->second;
                bool wu_light = !it.second;
                if (vw_light && wu_light) {
                    increment_T = ((double) (SL_cur_) / SL_size_) * ((double) ((SL_cur_ - 1.0))) / (SL_size_ - 1.0);
                } else if (vw_light || wu_light) {
                    increment_T = ((double) (SL_cur_) / SL_size_);
                }
            }

            cum_cnt += increment_T;
            auto w_it = local_triangles_cnt_.find(w);
            if (w_it != local_triangles_cnt_.end()) {
                w_it->second += increment_T;
            } else {
                local_triangles_cnt_.insert_unique(w, increment_T);
            }

        }
    } // end for

    // -- update counters
    if (cum_cnt > 0) {
        global_triangles_cnt_ += cum_cnt;
        auto u_local_it = local_triangles_cnt_.find(u);
        if (u_local_it != local_triangles_cnt_.end()) {
            u_local_it->second += cum_cnt;
        } else {
            local_triangles_cnt_.insert_unique(u, cum_cnt);
        }
        auto v_local_it = local_triangles_cnt_.find(v);
        if (v_local_it != local_triangles_cnt_.end()) {
            v_local_it->second += cum_cnt;
        } else {
            local_triangles_cnt_.insert_unique(v, cum_cnt);
        }
    }
}

bool TriangleSampler::sample_edge(const int src, const int dst) {

    int u = src;
    int v = dst;

    if (src > dst) {
        u = dst;
        v = src;
    }

    if (H_cur_ < H_size_) {
        // -- insert current edge into H
        H_cur_++;
        int current_heaviness = get_heaviness(u, v);
        heavy_edges_.push({{u, v}, current_heaviness});
        return true;
    } else {
        // -- H is full -> retrieve the lightest heavy edge between current and lightest in H
        if (SL_cur_ < SL_size_) {
            edge uv_sample = {u, v};
            int current_heaviness = get_heaviness(u, v);
            bool is_det = false;
            if (current_heaviness > -1) {
                auto lightest_heavy_edge = heavy_edges_.top();
                int lightest_heaviness = lightest_heavy_edge.second;
                if (current_heaviness > lightest_heaviness ||
                    (current_heaviness == lightest_heaviness && next_double() < 0.5)) {
                    // -- replace the lightest heavy edge with current edge
                    heavy_edges_.pop();
                    heavy_edges_.push({{u, v}, current_heaviness});
                    is_det = true;
                    subgraph_[lightest_heavy_edge.first.first][lightest_heavy_edge.first.second] = false;
                    subgraph_[lightest_heavy_edge.first.second][lightest_heavy_edge.first.first] = false;
                    uv_sample = lightest_heavy_edge.first;
                }
            }

            light_edges_sample_[SL_cur_++] = uv_sample;
            return is_det;
        } else if (WR_cur_ < WR_size_) {
            waiting_room_[WR_cur_++] = {u, v};
            return true;
        } else {
            // -- all sets are full -> resort to sampling
            SL_cur_++;
            // -- pop oldest and insert current edge in WR
            int oldest_idx = (int) (WR_cur_ % WR_size_);
            WR_cur_++;

            edge oldest_edge = waiting_room_[oldest_idx];
            waiting_room_[oldest_idx] = {u, v};
            edge uv_sample = oldest_edge;
            int current_heaviness = get_heaviness(uv_sample.first, uv_sample.second);
            if (current_heaviness > -1) {
                auto lightest_heavy_edge = heavy_edges_.top();
                int lightest_heaviness = lightest_heavy_edge.second;
                if (current_heaviness > lightest_heaviness ||
                    (current_heaviness == lightest_heaviness && next_double() < 0.5)) {
                    // -- replace the lightest heavy edge with current edge
                    heavy_edges_.pop();
                    heavy_edges_.push({{u, v}, current_heaviness});
                    subgraph_[lightest_heavy_edge.first.first][lightest_heavy_edge.first.second] = false;
                    subgraph_[lightest_heavy_edge.first.second][lightest_heavy_edge.first.first] = false;
                    uv_sample = lightest_heavy_edge.first;
                }
            }

            double p = (double) (SL_size_) / (double) SL_cur_;
            if (next_double() < p) {
                // -- edge is sampled
                subgraph_[uv_sample.first][uv_sample.second] = false;
                subgraph_[uv_sample.second][uv_sample.first] = false;
                // -- evict edge uniformly at random
                std::uniform_int_distribution<int> dis_SL(0, (int) SL_size_ - 1);
                int replace_idx = dis_SL(gen_);
                edge uv_replace = light_edges_sample_[replace_idx];
                remove_edge(uv_replace.first, uv_replace.second);
                light_edges_sample_[replace_idx] = uv_sample;
            } else {
                // -- edge is not resampled, just remove it from subgraph
                remove_edge(uv_sample.first, uv_sample.second);
            }
        }

        return true;

    }
}

void TriangleSampler::process_edge(const int u, const int v) {
    count_triangles(u, v);
    bool is_det = sample_edge(u, v);
    add_edge(u, v, is_det);
    t_++;
}
