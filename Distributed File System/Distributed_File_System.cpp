#include <bits/stdc++.h>
#include <mpi.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
using namespace std;


map <int, int> failed_nodes;                               // only for the metadata server. stores the nodes that have failed.

// list_file command code
int list_file(map <int, vector <int>> &chunks, map <int, int> &failed_nodes)
{
    for (auto &it: chunks)
    {
        int count = it.second.size();
        for (int i = 0; i < it.second.size(); i++){
            if (failed_nodes.find(it.second[i]) != failed_nodes.end())
                count--;
        }
        cout << it.first << " " << count << " ";
        for (int i = 0; i < it.second.size(); i++){
            if (failed_nodes.find(it.second[i]) == failed_nodes.end())
                cout << it.second[i] << " ";
        }
        cout << endl;
    }
    return 1;
}



struct Custom{
    char filename[256];
    int chunk_id;
    char chunk[40];
};




// upload command code 
int receive_upload_data(int rank, map <pair <int, string>, string> &store, MPI_Datatype &MPI_Custom)
{
    Custom recv_x;
    int size1;
    while (1)
    {
        MPI_Recv(&size1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (size1 == -1)
            break;
        else if (size1 == -2)
            return -1;
        MPI_Recv(&recv_x, 1, MPI_Custom, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        recv_x.chunk[32] = '\0';
        string filename = recv_x.filename, chunk = recv_x.chunk;
        store[{recv_x.chunk_id, filename}] = chunk;
    }
    return 1;
}

int upload(vector <string> &tokens, map <string, map <int, vector <int>>> &chunks, set <pair <int, int>> &s, int size, MPI_Datatype &MPI_Custom, map <int, int> &failed_nodes)
{
    string filename = tokens[1], filepath = tokens[2], fileData;
    ifstream file(filepath, ios::binary | ios::ate);
    if (!file.is_open()){
        for (int i = 1; i < size; i++){
            int x = -2;
            MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        return -1;
    }

    // Read the file into memory
    streamsize fileSize = file.tellg();
    file.seekg(0, ios::beg);
    fileData.resize(fileSize);
    file.read(fileData.data(), fileSize);

    Custom send_x;
    strcpy(send_x.filename, filename.c_str());
    int offset;

    
    for (int i = 0; i < fileData.length(); i += 32)
    {
        offset = min(32, (int)fileData.length()-i);
        string x = fileData.substr(i, offset);


        vector <pair <int, int>> pairs;
        int count = 0;
        for (auto &it: s)
        {
            int freq = it.first, id = it.second;
            if (failed_nodes.find(id) != failed_nodes.end())
                continue;
            count++;
            MPI_Send(&offset, 1, MPI_INT, id, 0, MPI_COMM_WORLD);
            pairs.push_back({freq, id});
            if (count == 3)
                break;
        }

        if (count == 0){
            for (int i = 1; i < size; i++){
                int x = -2;
                MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            return -1;   
        }

        strcpy(send_x.chunk, x.c_str());
        send_x.chunk_id = i/32;

        for (auto &it: pairs){
            s.erase({it.first, it.second});
            s.insert({it.first+1, it.second});

            MPI_Send(&send_x, 1, MPI_Custom, it.second, 0, MPI_COMM_WORLD);
            chunks[filename][i/32].push_back(it.second);
        }
    }

    for (int i = 1; i < size; i++){
        int x = -1;
        MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    return 1;
}



// retrieve command code
int send_data_retrieve(int rank, map <pair <int, string>, string> &store, MPI_Datatype &MPI_Custom)
{
    Custom recv_x;
    while (1)
    {
        int size1;
        MPI_Recv(&size1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (size1 == -1)
            break;
        else if (size1 == -2)
            return -1;
        MPI_Recv(&recv_x, 1, MPI_Custom, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        string filename = recv_x.filename;
        string s = store[{recv_x.chunk_id, filename}];
        int size2 = s.length();
        MPI_Send(&size2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(s.data(), size2, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
    return 1;
}


int retrieve(map <int, vector <int>> &chunks, string &filename, int size, MPI_Datatype &MPI_Custom, map <int, int> &failed_nodes)
{
    string ans = "";
    for (auto &it: chunks)
    {
        int num_nodes = it.second.size();
        if (num_nodes == 0){
            for (int i = 1; i < size; i++){
                int x = -2;
                MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            return -1;
        }

        // find a storage node that has not failed yet.
        int proc_id = -1;
        for (int i = 0; i < it.second.size(); i++){
            if (failed_nodes.find(it.second[i]) == failed_nodes.end()){
                proc_id = it.second[i];
                break;
            }
        }

        if (proc_id == -1){
            for (int i = 1; i < size; i++){
                int x = -2;
                MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            return -1;
        }

        int execute_query = 1;
        Custom send_x;
        // send_x.filename = filename;
        strcpy(send_x.filename, filename.c_str());
        send_x.chunk_id = it.first;
        
        MPI_Send(&execute_query, 1, MPI_INT, proc_id, 0, MPI_COMM_WORLD);
        MPI_Send(&send_x, 1, MPI_Custom, proc_id, 0, MPI_COMM_WORLD);
        string ret;
        int string_size;
        MPI_Recv(&string_size, 1, MPI_INT, proc_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        ret.resize(string_size);
        MPI_Recv(ret.data(), string_size, MPI_CHAR, proc_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < string_size; i++)
            ans.push_back(ret[i]);
    }
    for (int i = 1; i < size; i++){
        int x = -1;
        MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    cout << ans << endl;
    return 1;
}







// heartbeat feature implementation

// Global flags for node status
map<int, bool> node_status; // Metadata server tracks this
mutex node_status_mutex;
atomic<bool> running(true); // To gracefully stop threads

// Heartbeat sender for storage nodes
void send_heartbeat(int rank) {
    while (running) {
        MPI_Send(nullptr, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD);
        this_thread::sleep_for(chrono::seconds(1)); // Send every 3 seconds
    }
}



// Metadata server's MPI heartbeat receiver
void receive_heartbeats(int size) {
    MPI_Status status;
    map<int, chrono::steady_clock::time_point> last_heartbeat;
    while (running) {
        int flag = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            int source = status.MPI_SOURCE;
            MPI_Recv(nullptr, 0, MPI_BYTE, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            lock_guard<mutex> lock(node_status_mutex);
            node_status[source] = true;
            last_heartbeat[source] = chrono::steady_clock::now();
        }
        for (int i = 1; i < size; i++) {
            lock_guard<mutex> lock(node_status_mutex);
            auto now = chrono::steady_clock::now();
            if (node_status[i] && chrono::duration_cast<chrono::seconds>(now - last_heartbeat[i]).count() > 1) {
                node_status[i] = false;
                failed_nodes[i]++;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}




int failover(int rank) {
    running = false;
    return 1;
}





int recover(int rank) {
    running = true;
    thread heartbeat_thread(send_heartbeat, rank);
    heartbeat_thread.detach();
    return 1;
}




int main(int argc, char *argv[])
{
    ios_base::sync_with_stdio(0);
    // cin.tie(0);
    // cout.tie(0);
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    Custom datatype;
    MPI_Datatype MPI_Custom;
    int blockLengths[3] = {256, 1, 35};
    MPI_Aint displacements[3];
    MPI_Datatype types[3] = {MPI_CHAR, MPI_INT, MPI_CHAR};
    // Calculate displacements
    MPI_Aint baseAddress;
    MPI_Get_address(&datatype, &baseAddress);
    MPI_Get_address(&datatype.filename, &displacements[0]);
    MPI_Get_address(&datatype.chunk_id, &displacements[1]);
    MPI_Get_address(&datatype.chunk, &displacements[2]);
    for (int i = 0; i < 3; ++i) {
        displacements[i] -= baseAddress;
    }
    // Create and commit the custom MPI data type
    MPI_Type_create_struct(3, blockLengths, displacements, types, &MPI_Custom);
    MPI_Type_commit(&MPI_Custom);



    map <string, map <int, vector <int>>> chunks;    // only for metadata server. stores the chunks of a file. 
    // Input -> filename, output -> map of all the chunks of the file.
    // the map stores the chunk itself and outputs the nodes where the chunk is kept. Input -> chunk string, output -> all nodes where chunk is present
    
    set <pair <int, int>> s;                        // only for the metadata server. stores the number of chunks stored in each node
    map <pair <int, string>, string> store;                        // for all the storage nodes. stores the chunks available with each node.

    if (rank == 0){
        for (int i = 1; i < size; i++)
            s.insert({0, i});
    }


    thread heartbeat_thread;
    if (rank == 0) {
        // Metadata server: Start heartbeat monitoring
        thread receiver_thread(receive_heartbeats, size);
        receiver_thread.detach();
    } 
    else {
        // Storage nodes: Start sending heartbeats
        heartbeat_thread = thread(send_heartbeat, rank);
        heartbeat_thread.detach();
    }


    while(1)
    {
        vector <string> tokens;
        if (rank == 0){
            string input;
            getline(cin, input);
            stringstream ss(input);
            if (input.length() == 0)
                continue;
            string token;
            while (ss >> token)
                tokens.push_back(token);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        int tokens_len = tokens.size();
        MPI_Bcast(&tokens_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        tokens.resize(tokens_len);
        for (int i = 0; i < tokens_len; i++)
        {
            int size1 = (rank == 0) ? tokens[i].length() : 0;
            MPI_Bcast(&size1, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (rank != 0)
                tokens[i].resize(size1);
            MPI_Bcast(tokens[i].data(), size1, MPI_CHAR, 0, MPI_COMM_WORLD);
        }
        if (tokens[0] == "upload"){
            int ret;
            if (rank == 0){
                if (chunks.find(tokens[1]) != chunks.end())
                {
                    for (int i = 1; i < size; i++)
                    {
                        int x = -2;
                        MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    }
                    ret = -1;
                    cout << ret << endl;
                    continue;
                }
                ret = upload(tokens, chunks, s, size, MPI_Custom, failed_nodes);
            }
            else{
                ret = receive_upload_data(rank, store, MPI_Custom);
            }
            if (rank == 0){
                cout << ret << endl;
                list_file(chunks[tokens[1]], failed_nodes);
            }
            if (ret == -1)
                continue;   
        }
        else if (tokens[0] == "retrieve"){
            int ret;
            if (rank == 0)
            {
                if (chunks.find(tokens[1]) == chunks.end())
                {
                    for (int i = 1; i < size; i++)
                    {
                        int x = -2;
                        MPI_Send(&x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    }
                    ret = -1;
                    cout << ret << endl;
                    continue;
                }
                ret = retrieve(chunks[tokens[1]], tokens[1], size, MPI_Custom, failed_nodes);
            }
            else{
                ret = send_data_retrieve(rank, store, MPI_Custom);
            }
        }
        else if (tokens[0] == "search"){
            int ret, size1;
            vector <int> chunk_ids, nodes;
            if (rank == 0)
            {
                if (chunks.find(tokens[1]) == chunks.end())
                {
                    ret = -1;
                    cout << ret << endl;
                }
                else{
                    // map <string, map <int, vector <int>>> chunks
                    for (auto &it: chunks[tokens[1]])
                    {
                        int chunk_id = it.first;
                        for (int i = 0; i < it.second.size(); i++)
                        {
                            if (failed_nodes.find(it.second[i]) == failed_nodes.end()){
                                chunk_ids.push_back(chunk_id);
                                nodes.push_back(it.second[i]);
                                break;
                            }
                        }
                        if (chunk_ids.back() != chunk_id){
                            ret = -1;
                            cout << ret << endl;
                            break;
                        }
                    }
                }
                ret = 0;
            }
            MPI_Bcast(&ret, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (ret == -1)
                continue;

            size1 = nodes.size();
            MPI_Bcast(&size1, 1, MPI_INT, 0, MPI_COMM_WORLD);
            chunk_ids.resize(size1);
            nodes.resize(size1);
            MPI_Bcast(chunk_ids.data(), size1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(nodes.data(), size1, MPI_INT, 0, MPI_COMM_WORLD);


            int flag = -1;
            map <int, int> chunk_node_mp;
            for (int i = 0; i < size1; i++){
                if (nodes[i] == rank)
                    flag = 1;
                chunk_node_mp[chunk_ids[i]] = nodes[i];
            }
            string str = "";
            if (rank != 0)
            {
                for (int i = 0; i < chunk_ids.size(); i++)
                {
                    if (nodes[i] == rank){
                        string x = store[{i, tokens[1]}];
                        int size2 = x.length();
                        MPI_Send(&size2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                        MPI_Send(x.data(), x.length(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                    }
                }
            }
            else{
                for (int i = 0; i < chunk_ids.size(); i++)
                {
                    string x = "";
                    int size2;
                    MPI_Recv(&size2, 1, MPI_INT, nodes[i], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    x.resize(size2);
                    MPI_Recv(x.data(), size2, MPI_CHAR, nodes[i], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for (int j = 0; j < size2; j++)
                        str.push_back(x[j]);
                }
            }
            if (rank == 0)
            {
                vector <int> pos;
                for (int i = 0; i < str.length(); i++)
                {
                    int offset = min(tokens[2].length(), str.length()-i);
                    if (offset < tokens[2].length())
                        break;
                    if (str.substr(i, tokens[2].length()) == tokens[2] && (i+tokens[2].length() == str.length() || str[i+tokens[2].length()] == ' ') && (i == 0 || (i > 0 && str[i-1] == ' '))){
                        pos.push_back(i);
                    }
                }
                cout << pos.size() << endl;
                for (int i = 0; i < pos.size(); i++)
                    cout << pos[i] << " ";
                cout << endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
        else if (tokens[0] == "list_file" && rank == 0){
            if (chunks.find(tokens[1]) == chunks.end()){
                cout << -1 << endl;
                continue;
            }
            list_file(chunks[tokens[1]], failed_nodes);
        }
        else if (tokens[0] == "failover"){
            int node = 0, ret = -2;
            for (int i = tokens[1].length()-1; i >= 0; i--)
                node += pow(10, tokens[1].length()-1-i)*(tokens[1][i]-'0');

            if (node >= size || node <= 0){
                if (rank == 0)
                    cout << -1 << endl;
                continue;
            }
            
            if (rank == 0 && failed_nodes.find(node) != failed_nodes.end()){
                cout << -1 << endl;
                ret = -1;
            }
            MPI_Bcast(&ret, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (ret == -1)
                continue;

            if (rank == node){
                ret = failover(rank);
                cout << ret << endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
        else if (tokens[0] == "recover"){
            int node = 0, ret = -2;
            for (int i = tokens[1].length()-1; i >= 0; i--)
                node += pow(10, tokens[1].length()-1-i)*(tokens[1][i]-'0');

            if (node >= size || node <= 0){
                if (rank == 0)
                    cout << -1 << endl;
                continue;
            }

            if (rank == 0 && failed_nodes.find(node) == failed_nodes.end()){
                cout << -1 << endl;
                ret = -1;
            }
            MPI_Bcast(&ret, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (ret == -1)
                continue;

            if (rank == node){
                ret = recover(rank);
                cout << ret << endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == node){
                MPI_Send(&ret, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
            if (rank == 0){
                MPI_Recv(&ret, 1, MPI_INT, node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret == 1)
                    failed_nodes.erase(node);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
        else if (tokens[0] == "exit"){
            running = false;
            MPI_Type_free(&MPI_Custom);
            MPI_Finalize();
            exit(0);
        }
        else if (tokens[0] == "available"){
            if (rank == 0){
                for (int i = 1; i < size; i++)
                {
                    if (failed_nodes.find(i) == failed_nodes.end())
                        cout << i << " ";
                }
                cout << endl;
            }
        }
        else{
            if (rank == 0){
                cout << -1 << endl;
            }
        }
    }
    MPI_Type_free(&MPI_Custom);
    MPI_Finalize();
    return 0;
}