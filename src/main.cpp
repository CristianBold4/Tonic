#include <iostream>
#include "hash_table5.hpp"
#include "TriangleSampler.h"
#include "Utils.h"
#include <fstream>
#include <string>
#include <chrono>

void run_tonic_algo(std::string &dataset_path, TriangleSampler &algo, int update_steps, std::string &out_path) {

    std::ifstream file(dataset_path);
    std::string line;
    long n_line = 0;
    int u, v, t;

    std::ofstream out_file_steps(out_path + "_evolving_stream.csv", std::ios::app);
    std::string oracle_type_str = algo.edge_oracle_flag_ ? "Edges" : "Nodes";

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

            if (update_steps > 0 && n_line % update_steps == 0) {
                out_file_steps << "TonicINS,Alpha=" << algo.alpha_ << "-Beta=" << algo.beta_ << "," << algo.k_ << ","
                         << oracle_type_str << "," << algo.get_global_triangles() << "," << n_line << "\n";
            }
        }
        file.close();
    } else {
        std::cerr << "Error! Unable to open file " << dataset_path << "\n";
    }

    // -- EOS
    out_file_steps << "TonicINS,Alpha=" << algo.alpha_ << "-Beta=" << algo.beta_ << "," << algo.k_ << ","
                   << oracle_type_str << "," << algo.get_global_triangles() << "," << n_line << "\n";
    out_file_steps.close();

}

/* Use if POSIX basename() is unavailable */
char *base_name(char *s)
{
    char *start;

    /* Find the last '/', and move past it if there is one.  Otherwise, return
       a copy of the whole string. */
    /* strrchr() finds the last place where the given character is in a given
       string.  Returns NULL if not found. */
    if ((start = strrchr(s, '/')) == nullptr) {
        start = s;
    } else {
        ++start;
    }
    /* If you don't want to do anything interesting with the returned value,
       i.e., if you just want to print it for example, you can just return
       'start' here (and then you don't need dup_str(), or to free
       the result). */
    return start;
}

int main(int argc, char **argv) {

    char* project = base_name(argv[0]);

    if (strcmp(project, "DataPreprocessing") == 0) {
        if (argc != 5) {
            std::cerr << "Usage: DataPreprocessing <dataset_path> <delimiter> <skip>"
                         " <output_path>\n";
            return 1;
        } else {
            std::string dataset_path(argv[1]);
            std::string delimiter (argv[2]);
            int skip = atoi(argv[3]);
            std::string output_path(argv[4]);
            auto start = std::chrono::high_resolution_clock::now();
            Utils::preprocess_data(dataset_path, delimiter, skip, output_path);
            auto stop = std::chrono::high_resolution_clock::now();
            double time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count()) / 1000;
            std::cout << "Dataset preprocessed in time: " << time << " s\n";
            return 0;
        }
    }

    // -- run exact
    if (strcmp(project, "RunExactAlgo") == 0) {
        if (argc < 3 or argc > 5) {
            std::cerr << "Usage: RunExactAlgo <preprocessed_dataset_path> <output_path> [output_local_triangles]\n";
            return 1;
        } else {
            std::string dataset_path(argv[1]);
            std::string output_path(argv[2]);
            long total_T = -1;
            auto start = std::chrono::high_resolution_clock::now();
            if (argc == 4) {
                std::string output_path_local(argv[3]);
                total_T = Utils::run_exact_algorithm(dataset_path, output_path, true, output_path_local);
            } else {
                total_T = Utils::run_exact_algorithm(dataset_path, output_path, false, output_path);
            }
            auto stop = std::chrono::high_resolution_clock::now();
            double time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count()) / 1000;
            if (total_T > -1)
                printf("Exact Algorithm successfully run in time %.3f! Total count T = %ld\n", time, total_T);
            return 0;
        }
    }

    // -- build oracle
    if (strcmp(project, "BuildOracle") == 0) {
        if (argc < 5 or argc > 6) {
            std::cerr << "Usage: BuildOracle <preprocessed_dataset_path> <type = [Exact, noWR, Node]>, <percentage_retain>,"
                         " <output_path>, [<wr_size>] \n";
            return 1;
        } else {
            std::string dataset_path(argv[1]);
            std::string type_oracle(argv[2]);
            double percentage_retain = atof(argv[3]);
            std::string output_path(argv[4]);
            auto start = std::chrono::high_resolution_clock::now();
            if (strcmp(type_oracle.c_str(), "Exact") == 0) {
                Utils::build_edge_exact_oracle(dataset_path, percentage_retain, output_path);
                auto stop = std::chrono::high_resolution_clock::now();
                double time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count()) / 1000;
                printf("Exact Edge Oracle successfully run in time %.3f!\n", time);

            } else if(strcmp(type_oracle.c_str(), "noWR") == 0) {
                int wr_size = atoi(argv[5]);
                Utils::build_edge_exact_nowr_oracle(dataset_path, percentage_retain, output_path, wr_size);
                auto stop = std::chrono::high_resolution_clock::now();
                double time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count()) / 1000;
                printf("Exact-noWR Edge Oracle successfully run in time %.3f!\n", time);
            } else if (strcmp(type_oracle.c_str(), "Node") == 0) {
                    Utils::build_node_oracle(dataset_path, percentage_retain, output_path);
                    auto stop = std::chrono::high_resolution_clock::now();
                    double time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count()) / 1000;
                    printf("Node Map successfully run in time %.3f!\n", time);

                } else {
                std::cerr << "Build Oracle - Error! Type of Oracle must be Exact or Node.\n";
                return 1;
            }

            return 0;
        }
    }

    // -- Tonic Algo
    if (strcmp(project, "Tonic") == 0) {
        if (argc < 9) {
            std::cerr << "Usage: Tonic <random_seed> <memory_budget> <alpha> <beta> "
                         "<dataset_path> <oracle_path> <oracle_type = [nodes, edges]> <output_path> [update_steps]\n";
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

        int update_steps = -1;
        if (argc == 10) {
            update_steps = atoi(argv[9]);
        }

        std::chrono::time_point start = std::chrono::high_resolution_clock::now();
        double time, time_oracle;
        bool edge_oracle_flag = false;
        TriangleSampler tonic_algo(random_seed, memory_budget, alpha, beta);
        int size_oracle;

        if (oracle_type == "nodes") {
            emhash5::HashMap<int, int> node_oracle;
            if (!Utils::read_node_oracle(oracle_path, ' ', 0, node_oracle)) return 1;
            time_oracle = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start)).count()) / 1000;
            printf("Node Oracle successfully read in time %.3f! Size of the oracle = %d nodes\n",
                   time_oracle, node_oracle.size());
            size_oracle = (int) node_oracle.size();
            tonic_algo.set_node_oracle(node_oracle);
        } else if (oracle_type == "edges") {
            edge_oracle_flag = true;
            emhash5::HashMap<long, int> edge_oracle;
            if (!Utils::read_edge_oracle(oracle_path, ' ', 0, edge_oracle)) return 1;
            time_oracle = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start)).count()) / 1000;
            printf("Edge Oracle successfully read in time %.3f! Size of the oracle = %d edges\n",
                   time_oracle, edge_oracle.size());
            size_oracle = (int) edge_oracle.size();
            tonic_algo.set_edge_oracle(edge_oracle);
        } else {
            std::cerr << "Error! Oracle type must be nodes or edges\n";
            return 1;
        }

        start = std::chrono::high_resolution_clock::now();
        run_tonic_algo(dataset_path, tonic_algo, update_steps, output_path);
        time = (double) ((std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start)).count()) / 1000;

        // -- print results
        printf("Tonic Algo successfully run in time %.3f! Estimated count T = %f\n", time,
               tonic_algo.get_global_triangles());
        // -- write results
        // -- global estimates
        std::ofstream out_file(output_path + "_global_count.csv", std::ios::app);
        std::string oracle_type_str = edge_oracle_flag ? "Edges" : "Nodes";

        //out_file << "Algo,Params,Oracle,SizeOracle,TimeOracle,MemEdges,GlobalTriangleCount,Time\n";
        out_file << "TonicINS,Alpha=" << alpha << "-Beta=" << beta << "," << oracle_type_str << "," << size_oracle << ","
                 << time_oracle << "," << memory_budget << "," << tonic_algo.get_global_triangles() << "," << time << "\n";

        out_file.close();

        // -- local triangles
        std::ofstream out_file_local(output_path + "_local_counts.csv", std::ios::app);
        std::vector<int> nodes;
        tonic_algo.get_local_nodes(nodes);
        for (int u: nodes) {
            double count = tonic_algo.get_local_triangles(u);
            out_file_local << u << "," << tonic_algo.k_ << "," << std::fixed << count << "\n";
        }
        out_file_local.close();

        std::cout << "Done!\n";
        return 0;
    }

    return 1;

}
