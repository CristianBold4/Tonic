#include <iostream>
#include "hash_table5.hpp"
#include "TriangleSampler.h"
#include "Utils.h"
#include <fstream>
#include <string>
#include <chrono>

void run_tonic_algo(std::string &dataset_path, TriangleSampler &algo) {

    std::ifstream file(dataset_path);
    std::string line;
    long n_line = 0;
    int u, v, t;

    if (file.is_open()) {
        while (true) {
            if (!std::getline(file, line)) break;
            std::istringstream iss(line);
            std::string token;
            std::getline(iss, token, ' ');
            u = std::stoi(token);
            std::getline(iss, token, ' ');
            v = std::stoi(token);
            std::getline(iss, token, ' ');
            t = std::stoi(token);
            algo.process_edge(u, v);
            if (++n_line % 5000000 == 0) {
                printf("Processed %ld edges || Estimated count T = %f\n", n_line, algo.get_global_triangles());
            }
        }
        file.close();
    } else {
        std::cerr << "Error! Unable to open file " << dataset_path << "\n";
    }
}

int main(int argc, char **argv) {

    if (argc != 9) {
        std::cerr << "Usage: Tonic (random_seed) (memory_budget) (alpha) (beta) "
                     "(dataset_path) (oracle_path) (oracle_type = [nodes, edges]) (output_path)\n";
        return 1;
    }

    // -- read arguments
    int random_seed = atoi(argv[1]);
    long memory_budget = atol(argv[2]);
    double alpha = atof(argv[3]);
    double beta = atof(argv[4]);
    // -- assert alpha, beta in (0, 1)
    if (alpha <= 0 or alpha >= 1 or beta <= 0 or beta >= 1) {
        std::cerr << "Error! Alpha and Beta must be in (0, 1)\n";
        return 1;
    }

    std::string dataset_path(argv[5]);
    std::string oracle_path(argv[6]);
    std::string oracle_type(argv[7]);
    std::string output_path(argv[8]);

    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    double time;
    bool edge_oracle_flag = false;
    TriangleSampler tonic_algo(random_seed, memory_budget, alpha, beta);
    int size_oracle;

    if (oracle_type == "nodes") {
        emhash5::HashMap<int, int> node_oracle;
        if (!Utils::read_node_oracle(oracle_path, ' ', 0, node_oracle)) return 1;
        time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start)).count()) / 1000;
        printf("Node Oracle successfully read in time %.3f! Size of the oracle = %d nodes\n",
               time, node_oracle.size());
        size_oracle = (int) node_oracle.size();
        tonic_algo.set_node_oracle(node_oracle);
    } else if (oracle_type == "edges") {
        edge_oracle_flag = true;
        emhash5::HashMap<long, int> edge_oracle;
        if (!Utils::read_edge_oracle(oracle_path, ' ', 0, edge_oracle)) return 1;
        time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start)).count()) / 1000;
        printf("Edge Oracle successfully read in time %.3f! Size of the oracle = %d edges\n",
               time, edge_oracle.size());
        size_oracle = (int) edge_oracle.size();
        tonic_algo.set_edge_oracle(edge_oracle);
    } else {
        std::cerr << "Error! Oracle type must be nodes or edges\n";
        return 1;
    }

    start = std::chrono::high_resolution_clock::now();
    run_tonic_algo(dataset_path, tonic_algo);
    time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start)).count()) / 1000;

    // -- print results
    printf("Tonic Algo successfully run in time %.3f! Estimated count T = %f\n", time,
           tonic_algo.get_global_triangles());
    // -- write results
    // -- global estimates
    std::ofstream out_file(output_path + "_global_count.txt", std::ios::app);
    // -- local estimates
    std::ofstream out_file_local(output_path + "_local_count.txt", std::ios::app);
    if (edge_oracle_flag) {
        out_file << "Tonic Algo with Edge Oracle: " << oracle_path << " of size: " << size_oracle
                 << ", Memory Budget = " << memory_budget << "\n";
        out_file_local << "Tonic Algo with Edge Oracle: " << oracle_path << " of size: " << size_oracle
                       << ", Memory Budget = " << memory_budget << "\n";
    } else {
        out_file << "Tonic Algo with Node Oracle: " << oracle_path << " of size: " << size_oracle
                 << ", Memory Budget = " << memory_budget << "\n";
        out_file_local << "Tonic Algo with Node Oracle: " << oracle_path << " of size: " << size_oracle
                       << ", Memory Budget = "
                       << memory_budget << "\n";
    }

    out_file << std::fixed << "Estimated Triangles = " << tonic_algo.get_global_triangles() << "\n" <<
             "Time = " << time << " s\n";
    out_file.close();

    std::vector<int> nodes;
    tonic_algo.get_local_nodes(nodes);
    for (int u : nodes) {
        double count = tonic_algo.get_local_triangles(u);
        out_file_local << u << " " << std::fixed << count << "\n";
    }
    out_file_local.close();

    return 0;

}
