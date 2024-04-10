//
// Created by Cristian Boldrin on 09/03/24.
//

#include "../include/Utils.h"

bool Utils::read_node_oracle(std::string &oracle_filename, char delimiter, int skip,
                             emhash5::HashMap<int, int> &node_oracle) {

    std::ifstream file(oracle_filename);
    std::string line;
    int i = 0;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (i >= skip) {
                std::istringstream iss(line);
                std::string token;
                std::getline(iss, token, delimiter);
                int node = std::stoi(token);
                std::getline(iss, token, delimiter);
                int label = std::stoi(token);
                node_oracle.insert_unique(node, label);
            }
            i++;
        }
        file.close();
        return true;
    } else {
        std::cerr << "Error! Unable to open file " << oracle_filename << "\n";
        return false;
    }

}

bool Utils::read_edge_oracle(std::string &oracle_filename, char delimiter, int skip,
                             emhash5::HashMap<long, int> &edge_id_oracle) {


    std::ifstream file(oracle_filename);
    std::string line;
    int i = 0;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (i >= skip) {
                std::istringstream iss(line);
                std::string token;
                std::getline(iss, token, delimiter);
                int u = std::stoi(token);
                std::getline(iss, token, delimiter);
                int v = std::stoi(token);
                std::getline(iss, token, delimiter);
                int label = std::stoi(token);
                edge_id_oracle.insert_unique(TriangleSampler::edge_to_id(u, v), label);
            }
            i++;
        }
        file.close();
        return true;
    } else {
        std::cerr << "Error! Unable to open file " << oracle_filename << "\n";
        return false;
    }

}

void Utils::build_edge_exact_oracle(std::string &filepath, double percentage_retain, std::string &output_path) {

    std::cout << "Building edge oracle...\n";

    std::ifstream file(filepath);
    std::string line;

    emhash5::HashMap<Edge, int, hash_edge> oracle_heaviness;

    // -- graph
    emhash5::HashMap<int, std::unordered_set<int>> graph_stream;
    std::unordered_set<int> min_neighs;

    long total_T = 0.0;
    int u, v, t;

    if (file.is_open()) {
        long nline = 0;
        while (std::getline(file, line)) {
            nline++;
            std::istringstream iss(line);
            iss >> u >> v >> t;
            if (u == v) continue;
            if (graph_stream[u].find(v) != graph_stream[u].end() and graph_stream[v].find(u) != graph_stream[v].end()) {
                continue;
            }
            graph_stream[u].emplace(v);
            graph_stream[v].emplace(u);
            int du = (int) graph_stream[u].size();
            int dv = (int) graph_stream[v].size();
            int n_min = (du < dv) ? u : v;
            int n_max = (du < dv) ? v : u;
            min_neighs = graph_stream[n_min];
            int common_neighs = 0;
            for (auto neigh: min_neighs) {
                if (graph_stream[n_max].find(neigh) != graph_stream[n_max].end()) {
                    common_neighs++;
                    int entry_11 = (n_min < neigh) ? n_min : neigh;
                    int entry_12 = (n_min < neigh) ? neigh : n_min;
                    int entry_21 = (neigh < n_max) ? neigh : n_max;
                    int entry_22 = (neigh < n_max) ? n_max : neigh;

                    oracle_heaviness[{entry_11, entry_12}] += 1;
                    oracle_heaviness[{entry_21, entry_22}] += 1;
                }
            }

            int entry_31 = (u < v) ? u : v;
            int entry_32 = (u < v) ? v : u;
            oracle_heaviness[{entry_31, entry_32}] = common_neighs;
            total_T += common_neighs;

            if (nline % 3000000 == 0) {
                printf("Processed %ld edges | Counted %ld triangles\n", nline, total_T);
            }
        }

        // -- eof: sort results
        std::cout << "Sorting the oracle and retrieving the top " << percentage_retain << " values...\n";
        std::vector<std::pair<Edge, int>> sorted_oracle;
        sorted_oracle.reserve(oracle_heaviness.size());
        for (auto &elem: oracle_heaviness) {
            sorted_oracle.emplace_back(elem.first, elem.second);
        }

        std::sort(sorted_oracle.begin(), sorted_oracle.end(),
                  [](const std::pair<Edge, int> &a, const std::pair<Edge, int> &b) { return a.second > b.second; });

        // -- write results
        std::cout << "Done!\nWriting results...\n";
        int stop_idx = (int) (percentage_retain * (int) sorted_oracle.size());

        std::ofstream out_file(output_path);
        std::cout << "Total Triangles -> " << total_T << "\n";
        std::cout << "Oracle Size = " << sorted_oracle.size() << "\n";

        int cnt = 0;
        for (auto elem: sorted_oracle) {
            if (cnt > stop_idx) break;
            out_file << elem.first.first << " " << elem.first.second << " " << elem.second << "\n";
            cnt++;
        }

    } else {
        std::cerr << "Error! Unable to open oracle file " << filepath << "\n";
    }
}

void Utils::build_node_oracle(std::string &filepath, double percentage_retain, std::string &output_path) {

    std::cout << "Building node oracle...\n";

    std::ifstream file(filepath);
    std::string line;

    emhash5::HashMap<int, int> node_map;
    // std::unordered_map<int, int> node_map;

    int u, v, t;

    if (file.is_open()) {
        long nline = 0;
        while (std::getline(file, line)) {
            nline++;
            std::istringstream iss(line);
            iss >> u >> v >> t;
            if (u == v) continue;

            if (node_map.find(u) != node_map.end())
                node_map[u] += 1;
            else
                node_map[u] = 1;

            if (node_map.find(v) != node_map.end())
                node_map[v] += 1;
            else
                node_map[v] = 1;

            if (nline % 3000000 == 0) {
                printf("Processed %ld edges\n", nline);
            }
        }

        // -- eof: sort results
        std::cout << "Sorting the oracle and retrieving the top " << percentage_retain << " values...\n";
        // convert node map to vector of pairs
        std::vector <std::pair<int, int>> sorted_oracle;
        sorted_oracle.reserve(node_map.size());
        for (auto &elem: node_map) {
            sorted_oracle.emplace_back(elem.first, elem.second);
        }

        // std::vector<std::pair<int, int>> sorted_oracle(node_map.begin(), node_map.end());
        std::sort(sorted_oracle.begin(), sorted_oracle.end(),
                  [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.second > b.second; });

        // -- write results
        std::cout << "Done!\nWriting results...\n";
        int stop_idx = (int) (percentage_retain * (int) sorted_oracle.size());

        std::ofstream out_file(output_path);
        std::cout << "Oracle Size = " << sorted_oracle.size() << "\n";

        int cnt = 0;
        for (auto elem: sorted_oracle) {
            if (cnt > stop_idx) break;
            out_file << elem.first << " " << elem.second << "\n";
            cnt++;
        }

    } else {
        std::cerr << "Error! Unable to open oracle file " << filepath << "\n";
    }
}

long Utils::run_exact_algorithm(std::string &dataset_filepath, std::string &output_path) {

    std::ifstream file(dataset_filepath);
    std::string line, su, sv;

    if (!file.is_open()) {
        std::cerr << "Error! Unable to open file " << dataset_filepath << "\n";
        return -1;
    }

    std::cout << "Running exact algorithm...\n";

    // - graph
    emhash5::HashMap<int, std::unordered_set<int>> graph_stream;

    int u, v, du, dv, n_min, n_max, timestamp;
    std::unordered_set<int> min_neighbors;

    long total_T = 0, nline = 0;
    // -- check self-loops

    while (std::getline(file, line)) {
        nline++;

        std::istringstream iss(line);
        iss >> u >> v >> timestamp;
        if (u == v) continue;
        if (graph_stream[u].find(v) != graph_stream[u].end() and graph_stream[v].find(u) != graph_stream[v].end()) {
            continue;
        }

        // -- add edge to graph stream
        graph_stream[u].emplace(v);
        graph_stream[v].emplace(u);

        // -- count triangles
        du = (int) graph_stream[u].size();
        dv = (int) graph_stream[v].size();
        n_min = (du < dv) ? u : v;
        n_max = (du < dv) ? v : u;
        min_neighbors = graph_stream[n_min];
        for (const auto &neigh: min_neighbors) {
            if (graph_stream[n_max].find(neigh) != graph_stream[n_max].end()) {
                // -- triangle {n_min, neigh, n_max} discovered
                total_T += 1;
            }
        }

        if (nline % 3000000 == 0) {
            printf("Processed %ld edges | Counted %ld triangles\n", nline, total_T);
        }


    }

    long num_nodes = (long) graph_stream.size();
    printf("Processed dataset with n = %ld, m = %ld\n", num_nodes, nline);
    // -- write results
    std::ofstream out_file(output_path, std::ios::app);
    out_file << "Ground Truth:" << "\n";
    out_file << "Nodes = " << num_nodes << "\n";
    out_file << "Edges = " << nline << "\n";
    out_file << "Triangles = " << total_T << "\n";
    out_file.close();
    return total_T;
}

void Utils::preprocess_data(const std::string &dataset_filepath, std::string &delimiter, int skip,
                            std::string &output_path) {

    std::cout << "Preprocessing Dataset...\n";
    std::ifstream file(dataset_filepath);
    std::string line, su, sv;

    // -- edge stream
    std::unordered_map<Edge, int, hash_edge> edge_stream;

    // - graph
    std::unordered_map<int, std::unordered_set<int>> graph_stream;

    int u, v, t;
    std::unordered_set<int> min_neighbors;

    if (file.is_open()) {

        long nline = 0;
        long num_nodes;
        long num_edges = 0;

        t = 0;
        while (std::getline(file, line)) {
            nline++;
            if (nline <= skip) continue;

            std::istringstream iss(line);
            std::getline(iss, su, delimiter[0]);
            std::getline(iss, sv, delimiter[0]);

            u = stoi(su);
            v = stoi(sv);

            // -- check self-loops
            if (u == v) continue;
            t++;
            int v1 = (u < v) ? u : v;
            int v2 = (u < v) ? v : u;
            std::pair uv = std::make_pair(v1, v2);
            // -- check for multiple edges
            if (graph_stream[u].find(v) != graph_stream[u].end() and graph_stream[v].find(u) != graph_stream[v].end()) {
                edge_stream[uv] = t;
                continue;
            }

            // -- add edge to graph stream
            graph_stream[u].emplace(v);
            graph_stream[v].emplace(u);
            num_edges++;
            edge_stream[uv] = t;

            if (nline % 3000000 == 0) {
                std::cout << "Processed " << nline << " edges...\n";
            }

        }

        // -- eof
        num_nodes = (int) graph_stream.size();
        printf("Preprocessed dataset with n = %ld, m = %ld\n", num_nodes, num_edges);
        std::cout << "Sorting edge map...\n";
        // -- create a vector that stores all the entries <K, V> of the map edge stream
        std::vector<std::pair<Edge, int>> ordered_edge_stream(edge_stream.begin(), edge_stream.end());
        // -- sort edge by increasing time
        std::sort(ordered_edge_stream.begin(), ordered_edge_stream.end(),
                  [](const std::pair<Edge, int> &a, const std::pair<Edge, int> &b) { return a.second < b.second; });

        // -- write results
        std::cout << "Done!\nWriting results...\n";
        std::ofstream out_file(output_path);

        int cnt = 0;
        for (auto elem: ordered_edge_stream) {
            // -- also, rescale the time (not meant for Tonic)
            out_file << elem.first.first << " " << elem.first.second << " " << ++cnt << "\n";
        }

        out_file.close();

    } else {
        std::cerr << "DataPreprocessing - Error! Graph filepath not opened.\n";
    }

}