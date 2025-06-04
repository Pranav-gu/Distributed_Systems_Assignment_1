#include <bits/stdc++.h>
#include <mpi.h>
using namespace std;


pair <int, int> move(const pair <int, int> &curr, char direction, int n, int m)
{
    pair <int, int> ret;
    if (direction == 'R'){
        ret.first = curr.first;
        ret.second = (curr.second+1)%m;
    }
    else if (direction == 'L'){
        ret.first = curr.first;
        ret.second = (curr.second+m-1)%m;
    }
    else if (direction == 'U'){
        ret.first = (curr.first+n-1)%n;
        ret.second = curr.second;
    }
    else if (direction == 'D'){
        ret.first = (curr.first+1)%n;
        ret.second = curr.second;
    }
    return ret;
}


struct Custom{
    int x;
    int y;
    char ch;
    int order;
};


int main(int argc, char *argv[])
{
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);
    MPI_Init(&argc, &argv);
    int size, rank, n, m, k, t;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    if (rank == 0){
        cin >> n >> m >> k >> t;
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&t, 1, MPI_INT, 0, MPI_COMM_WORLD);



    unordered_map <char, char> directions_2, directions_4;
    directions_2['U'] = 'R';
    directions_2['D'] = 'L';
    directions_2['L'] = 'U';
    directions_2['R'] = 'D';
    directions_4['U'] = 'D';
    directions_4['D'] = 'U';
    directions_4['L'] = 'R';
    directions_4['R'] = 'L';
    map <int, vector <pair <int, char>>> balls[n];

    if (rank == 0){
        for (int i = 0; i < k; i++)
        {
            int x, y;
            char ch;
            cin >> x >> y >> ch;
            balls[x][y].push_back({i, ch});
        }
    }


    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm new_comm_world = MPI_COMM_NULL;
    if(size > n)
    {
        MPI_Group world_group;
        MPI_Comm_group(MPI_COMM_WORLD, &world_group);
        MPI_Group new_group;
        int ranges[][3] = {{n, size-1, 1}};
        MPI_Group_range_excl(world_group, 1, ranges, &new_group);
        MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_comm_world);
        MPI_Group_free(&world_group);
        MPI_Group_free(&new_group);
    }
    else
    {
        MPI_Comm_dup(MPI_COMM_WORLD, &new_comm_world);
    }


    if (new_comm_world != MPI_COMM_NULL){
        MPI_Comm_rank(new_comm_world, &rank);
        MPI_Comm_size(new_comm_world, &size);

        // create a custom MPI Datatype
        Custom datatype;
        MPI_Datatype MPI_Custom;
        int blockLengths[4] = {1, 1, 1, 1};
        MPI_Aint displacements[4];
        MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_CHAR, MPI_INT};
        // Calculate displacements
        MPI_Aint baseAddress;
        MPI_Get_address(&datatype, &baseAddress);
        MPI_Get_address(&datatype.x, &displacements[0]);
        MPI_Get_address(&datatype.y, &displacements[1]);
        MPI_Get_address(&datatype.ch, &displacements[2]);
        MPI_Get_address(&datatype.order, &displacements[3]);
        for (int i = 0; i < 4; ++i) {
            displacements[i] -= baseAddress;
        }
        // Create and commit the custom MPI data type
        MPI_Type_create_struct(4, blockLengths, displacements, types, &MPI_Custom);
        MPI_Type_commit(&MPI_Custom);

        int num_rows = n/size;
        vector <vector <Custom>> send_data(size);
        for (int i = 0; i < n; i++)
        {
            for (auto &it: balls[i])
            {
                for (int x = 0; x < it.second.size(); x++)
                {
                    Custom send_x;
                    send_x.x = i;
                    send_x.y = it.first;
                    send_x.ch = it.second[x].second;
                    send_x.order = it.second[x].first;
                    if (i/num_rows < size)
                        send_data[i/num_rows].push_back(send_x);
                    else
                        send_data[size-1].push_back(send_x);
                }
            }
        }

        if (rank == 0){
            for (int i = 1; i < size; i++){
                int x = send_data[i].size();
                MPI_Send(&x, 1, MPI_INT, i, 0, new_comm_world);
                if (x > 0)
                    MPI_Send(send_data[i].data(), x, MPI_Custom, i, 0, new_comm_world);
            }
        }

        if (rank != 0){
            int x;
            MPI_Recv(&x, 1, MPI_INT, 0, 0, new_comm_world, MPI_STATUS_IGNORE);
            if (x > 0){
                send_data[rank].resize(x);
                MPI_Recv(send_data[rank].data(), x, MPI_Custom, 0, 0, new_comm_world, MPI_STATUS_IGNORE);
                for (int i = 0; i < send_data[rank].size(); i++)
                    balls[send_data[rank][i].x][send_data[rank][i].y].push_back({send_data[rank][i].order, send_data[rank][i].ch});   
            }
        }

        MPI_Barrier(new_comm_world);

        if (rank == 0){
            for (int i = 0; i < n; i++)
            {
                if (num_rows*rank <= i && i < (rank+1)*num_rows)
                    continue;
                // if ((i+1)/size != rank)
                if (rank == size-1 && (rank+1)*num_rows <= i && i < n)
                    continue;
                balls[i].clear();
            }
        }

        for (int i = 0; i < t; i++)
        {
            map <int, vector <pair <int, char>>> new_balls[n], send[n];
            int start = rank*num_rows, end = (rank+1)*num_rows;
            if (rank == size-1)
                end = n;
            for (int j = start; j < end; j++)
            {
                for (auto &it: balls[j])
                {
                    pair <int, int> coords = {j, it.first};
                    for (int x = 0; x < it.second.size(); x++){
                        pair <int, int> ret = move(coords, it.second[x].second, n, m);
                        // if ((ret.first+1)/size == rank)
                        if (num_rows*rank <= ret.first && ret.first < (rank+1)*num_rows)
                            new_balls[ret.first][ret.second].push_back({it.second[x].first, it.second[x].second});
                        else
                        {
                            if (rank == size-1 && num_rows*(rank+1) <= ret.first && ret.first < n)
                                new_balls[ret.first][ret.second].push_back({it.second[x].first, it.second[x].second});
                            else
                                send[ret.first][ret.second].push_back({it.second[x].first, it.second[x].second});
                        }
                    }
                }
            }


            for (int j = rank*num_rows; j < end; j++)
                balls[j] = new_balls[j];



            vector<Custom> recv_from_left;
            vector<Custom> recv_from_right;
            vector<Custom> send_to_left;
            vector<Custom> send_to_right;
            for (int j = 0; j < n; j++)
            {
                for (auto &it: send[j])
                {
                    int flag = 0;                                   // flag = 0 means the ball needs to be sent to the next row process
                    if (j == (rank*num_rows+n-1)%n)
                        flag = 1;
                    for (int x = 0; x < it.second.size(); x++)
                    {
                        Custom send_x;
                        send_x.x = j;
                        send_x.y = it.first;
                        send_x.ch = it.second[x].second;
                        send_x.order = it.second[x].first;
                        if (flag)
                            send_to_left.push_back(send_x);
                        else
                            send_to_right.push_back(send_x);
                    }
                }
            }

            int send_data_left = send_to_left.size(), send_data_right = send_to_right.size(), recv_data_left = 0, recv_data_right = 0;
            if (size > 1){
                MPI_Send(&send_data_left, 1, MPI_INT, (rank+size-1)%size, 0, new_comm_world);
                if (send_data_left)
                    MPI_Send(send_to_left.data(), send_data_left, MPI_Custom, (rank+size-1)%size, 0, new_comm_world);
            }

            if (size > 1){
                MPI_Recv(&recv_data_right, 1, MPI_INT, (rank+1)%size, 0, new_comm_world, MPI_STATUS_IGNORE);
                if (recv_data_right){
                    recv_from_right.resize(recv_data_right);
                    MPI_Recv(recv_from_right.data(), recv_data_right, MPI_Custom, (rank+1)%size, 0, new_comm_world, MPI_STATUS_IGNORE);
                }
            }

            if (size > 1){
                MPI_Send(&send_data_right, 1, MPI_INT, (rank+1)%size, 0, new_comm_world);
                if (send_data_right)
                    MPI_Send(send_to_right.data(), send_data_right, MPI_Custom, (rank+1)%size, 0, new_comm_world);
            }


            if (size > 1){
                MPI_Recv(&recv_data_left, 1, MPI_INT, (rank+size-1)%size, 0, new_comm_world, MPI_STATUS_IGNORE);
                if (recv_data_left){
                    recv_from_left.resize(recv_data_left);
                    MPI_Recv(recv_from_left.data(), recv_data_left, MPI_Custom, (rank+size-1)%size, 0, new_comm_world, MPI_STATUS_IGNORE);
                }
            }

            for (int x = 0; x < recv_data_right; x++)
                balls[recv_from_right[x].x][recv_from_right[x].y].push_back({recv_from_right[x].order, recv_from_right[x].ch});                

            for (int x = 0; x < recv_data_left; x++)
                balls[recv_from_left[x].x][recv_from_left[x].y].push_back({recv_from_left[x].order, recv_from_left[x].ch});                

            for (int j = start; j < end; j++)
            {
                for (auto &it: balls[j])
                {
                    if (it.second.size() == 2) {
                        vector<pair <int, char>> temp = it.second;
                        for (pair <int, char> &val: temp)
                            val.second = directions_2[val.second];
                        it.second = temp;
                    }
                    else if (it.second.size() == 4) {
                        vector<pair <int, char>> temp = it.second;
                        for (pair <int, char> &val: temp)
                            val.second = directions_4[val.second];
                        it.second = temp;
                    }
                }
            }
        }


        if (rank != 0){
            vector <Custom> send_data;
            int start = rank*num_rows, end = (rank+1)*num_rows;
            if (rank == size-1)
                end = n;
            for (int i = start; i < end; i++)
            {
                for (auto &it: balls[i])
                {
                    for (int x = 0; x < it.second.size(); x++)
                    {
                        Custom send_x;
                        send_x.x = i;
                        send_x.y = it.first;
                        send_x.ch = it.second[x].second;
                        send_x.order = it.second[x].first;
                        send_data.push_back(send_x);
                    }
                }
            }
            int size1 = send_data.size();
            MPI_Send(&size1, 1, MPI_INT, 0, 0, new_comm_world);
            MPI_Send(send_data.data(), size1, MPI_Custom, 0, 0, new_comm_world);
        }
        else{
            for (int i = 1; i < size; i++){
                int size1 = 0;
                MPI_Recv(&size1, 1, MPI_INT, i, 0, new_comm_world, MPI_STATUS_IGNORE);
                vector <Custom> recv_data(size1);
                MPI_Recv(recv_data.data(), size1, MPI_Custom, i, 0, new_comm_world, MPI_STATUS_IGNORE);
                for (int x = 0; x < size1; x++)
                    balls[recv_data[x].x][recv_data[x].y].push_back({recv_data[x].order, recv_data[x].ch});
            }

            vector <pair <int, pair <pair <int, int>, char>>> pairs;
            for (int i = 0; i < n; i++)
            {
                for (auto &it: balls[i]){
                    for (auto &it1: it.second)
                        pairs.push_back({it1.first, {{i, it.first}, it1.second}});
                }
            }
            sort(pairs.begin(), pairs.end(), [](auto &x, auto &y){
                if (x.first < y.first)
                    return true;
                return false;
            });
            for (auto &it: pairs)
                cout << it.second.first.first << " " << it.second.first.second << " " << it.second.second << endl;
        }
        MPI_Barrier(new_comm_world);
        MPI_Type_free(&MPI_Custom);
    }
    MPI_Finalize();
    return 0;
}
