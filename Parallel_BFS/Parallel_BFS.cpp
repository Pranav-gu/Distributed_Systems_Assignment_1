#include <bits/stdc++.h>
#include <mpi.h>
using namespace std;

int main(int argc, char *argv[])
{
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);
    MPI_Init(&argc, &argv);
    int size, rank, n = 0, m = 0, k = 0, exit_node = -1;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    

    unordered_map <int, int> nodes[100005];
    vector <int> reverse_nodes[100005], explorers;
    if (rank == 0){
        ifstream fin(argv[1]);
        int count = 0, num_blocked = 0;
        while (1){
            if (count == m+6 && m != 0)
                break;
            string input;
            vector <int> tokens;
            // getline(cin, input);
            getline(fin, input);
            stringstream ss(input);
            string token;
            while (ss >> token)
                tokens.push_back(stoi(token));
            if (count == 0){
                n = tokens[0];
                m = tokens[1];
            }
            if (count >= 1 && count <= m){
                nodes[tokens[0]][tokens[1]] = 1;
                if (tokens[2])
                    nodes[tokens[1]][tokens[0]] = 1;
            }
            if (count == m+1)
                k = tokens[0];
            if (count == m+2){
                for (int i = 0; i < tokens.size(); i++)
                    explorers.push_back(tokens[i]);
            }
            if (count == m+3)
                exit_node = tokens[0];
            if (count == m+4)
                num_blocked = tokens[0];
            if (count == m+5){
                map <int, int> nums;
                for (auto &it: tokens)
                    nums[it]++;
                vector <pair <int, int>> edges_to_remove;
                for (int i = 0; i < n; i++)
                {
                    for (auto &it: nodes[i]){
                        if (nums.find(it.first) != nums.end())
                            edges_to_remove.push_back({i, it.first});
                    }
                }
                for (auto &it: edges_to_remove){
                    nodes[it.first].erase(it.second);
                }
            }
            count++;
        }
        for (int i = 0; i < n; i++)
        {
            for (auto &it: nodes[i])
                reverse_nodes[it.first].push_back(i);
        }
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&exit_node, 1, MPI_INT, 0, MPI_COMM_WORLD);


    for (int i = 0; i < n; i++) {
        int size1 = (rank == 0) ? reverse_nodes[i].size() : 0;
        MPI_Bcast(&size1, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank != 0)
            reverse_nodes[i].resize(size1);
        MPI_Bcast(reverse_nodes[i].data(), size1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    

    vector <int> answer(n, -1);
    answer[exit_node] = 0;
    int level = 0;
    unordered_set <int> s, next_s;
    if (rank == exit_node%size)
        s.insert(exit_node);
    while (1)
    {
        int flag = s.size() > 0 ? 1: 0, global_flag;
        MPI_Allreduce(&flag, &global_flag, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        if (global_flag == 0)               // frontier set across all processors is empty.
            break;
        next_s.clear();
        for (auto &it: s)               // frontier nodes
        {
            for (auto &it1: reverse_nodes[it])  // neighbours of all frontier nodes
            {
                if (answer[it1] == -1){
                    if (it1%size != rank)
                        answer[it1] = level+1;
                    next_s.insert(it1);
                }
            }
        }

        vector<unordered_set<int>> send_buffers(size);
        vector<unordered_set<int>> recv_buffers(size);
        for (int neighbor : next_s) { 
            int owner = neighbor%size; // Assign owner processor
            send_buffers[owner].insert(neighbor);
        }

        // Exchange buffers between processors
        for (int i = 0; i < size; i++) {
            if (i != rank) {
                int send_size = send_buffers[i].size();
                int recv_size = 0;
                MPI_Sendrecv(&send_size, 1, MPI_INT, i, 0, &recv_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                vector<int> send_data(send_buffers[i].begin(), send_buffers[i].end());
                vector<int> recv_data(recv_size);
                MPI_Sendrecv(send_data.data(), send_size, MPI_INT, i, 1, recv_data.data(), recv_size, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                recv_buffers[i].insert(recv_data.begin(), recv_data.end());
            }
        }

        // Combine received neighbors into NS and update distances
        for (int i = 0; i < size; i++) {
            if (i != rank) {
                for (int neighbor : recv_buffers[i]) {
                    if (answer[neighbor] == -1) {
                        answer[neighbor] = level+1;
                        next_s.insert(neighbor);
                    }
                }
            }
            if (i == rank){
                for (int neighbor : send_buffers[i]) {
                    if (answer[neighbor] == -1) {
                        answer[neighbor] = level+1;
                        next_s.insert(neighbor);
                    }
                }
            }
        }
        // Update level and move to next frontier
        level++;
        s = next_s;
    }
    int size1 = (rank == 0) ? explorers.size() : 0;
    MPI_Bcast(&size1, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0)
        explorers.resize(size1);
    MPI_Bcast(explorers.data(), size1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == exit_node%size){
        ofstream fout(argv[2]);
        for (auto &it: explorers)
            fout << answer[it] << " ";
    }
    MPI_Finalize();
    return 0;
}