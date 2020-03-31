#include <bits/stdc++.h>
using namespace std;

string routingTableFile("output/routingTable.txt");
string forwardingTableFile("output/forwardingTable.txt");
string pathsFile("output/pathsTable.txt");

bool optimist = true;
string flag = "hop"; 

struct Edge {
    int u, v, delay, vcidCounter;
    bool isActive;
    double bandwidth, remBandwidth;
    Edge(int u, int v, int delay, double bandwidth) : 
        u(u), v(v), delay(delay), isActive(true),
        bandwidth(bandwidth), remBandwidth(bandwidth), vcidCounter(0) {}
};

struct Node {
    int id;
    map<pair<int,int>, pair<int,int>> forwardingTable;
    Node(int id) : id(id) {}
};

struct Connection {
    int u, v, path;
    double bmin, bavg, bmax;
    vector<int> vcids;
    Connection(int u, int v, double bmin, double bavg, double bmax) :
        u(u), v(v), bmin(bmin), bavg(bavg), bmax(bmax), path(0)  {}
};

// Routing
void bellmanFord(vector<vector<Edge*>> &adj, map<pair<int,int>, vector<Edge*>> &shortestPath) {
    int n = adj.size();
    int opt[n][n][n], par[n][n][n];
    
    // Initialise opt and par for k = 0
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            par[i][j][0] = -1;
            if (i == j) {
                opt[i][j][0] = 0;
            }
            else {
                opt[i][j][0] = INT_MAX / 2;
            }
        }
    }
    // Bellman-Ford
    for (int k = 1; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                opt[i][j][k] = opt[i][j][k-1];
                par[i][j][k] = par[i][j][k-1];
                for (int v = 0; v < n; v++) {
                    if (adj[v][j] == NULL) continue;
                    int newCost = opt[i][v][k-1];
                    newCost += (flag == "hop" ? 1 : adj[v][j]->delay);
                    if (newCost < opt[i][j][k]) {
                        opt[i][j][k] = newCost;
                        par[i][j][k] = v;
                    }
                }
            }
        }
    }
    // for (int i = 0; i < n; i++) {
    //     for (int j = 0; j < n; j++) {
    //         cout << opt[i][j][n-1] << " ";
    //     }
    //     cout << endl;
    // }
    // return;
    // Decode shortest path from par[][][]
    vector<Edge*> path;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            path.clear();
            int p = j, np;
            for (int k = n - 1; k >= 0; k--) {
                np = par[i][p][k];
                if (np == -1) break;
                if (np == p) continue;
                path.push_back(adj[np][p]);
                p = np; 
            }
            reverse(path.begin(), path.end());
            shortestPath[{i, j}] = path;
        }
    }
    return;
}

void findAlternatePath(int u, int v, vector<vector<Edge*>> &adj, vector<Edge*> &path, map<pair<int,int>, vector<Edge*>> &altPath) {
    for (auto link : path) {
        link->isActive = false;
    }
    // Use Djisktra's to find new shortest path from u to v;
    int n = adj.size();
    multiset<tuple<int,int,int>> pq;
    vector<int> dist(n, INT_MAX), vis(n, 0), par(n, -1);
    pq.insert({0, u, -1});
    dist[u] = 0;
    vis[u] = 1;
    par[u] = -1;
    while (!pq.empty()) {
        int cost = get<0> (*pq.begin());
        int i = get<1> (*pq.begin());
        int p = get<2> (*pq.begin());
        pq.erase(pq.begin());
        if (cost > dist[i]) continue;
        dist[i] = cost;
        par[i] = p;
        for (int j = 0; j < n; j++) {
            if (adj[i][j] == NULL || !adj[i][j]->isActive || vis[j]) continue;
            int newCost = cost + (flag == "hop" ? 1 : adj[i][j]->delay);
            if (newCost < dist[j]) pq.insert({newCost, j, i});
        }
        vis[i] = 1;
    }
    vector<Edge*> newPath;
    int np = par[v], p = v;
    while (np != -1) {
        newPath.push_back(adj[np][p]);
        p = np;
        np = par[np];
    }
    reverse(newPath.begin(), newPath.end());
    altPath[{u, v}] = newPath;

    for (auto link : path) {
        link->isActive = true;
    }
    return;
}

// Virtual Circuit setup
double calcReqBandwidth(Connection *req) {
    if (optimist) {
        return min(req->bmax, req->bavg + 0.25 * (req->bmax - req->bmin));
    }
    else {
        return req->bmax;
    }
}

void setupVirtualCircuit(Connection *req, int pathChoice, vector<Edge*> &path, vector<Node *> &nodes) {
    int b = calcReqBandwidth(req);
    req->path = pathChoice;

    // 1 -> 2 -> 3 -> 4
    int prevVcid = -1, curVcid;
    int prevNode = -1;
    for (auto link : path) {
        link->remBandwidth -= b;
        curVcid = link->vcidCounter;
        req->vcids.push_back(curVcid);
        link->vcidCounter++;
        
        int u = link->u;
        nodes[u]->forwardingTable.insert({{prevNode, prevVcid}, {link->v, curVcid}});
        prevVcid = curVcid;
        prevNode = u;
    }
    nodes[req->v]->forwardingTable.insert({{prevNode, prevVcid},{-1, -1}});
    return;
}

bool canUsePath(Connection *req, vector<Edge*> &path) {
    double b = calcReqBandwidth(req);
    for (auto link : path) {
        if (link->remBandwidth < b) {
            return false;     
        } 
    }
    return true;
}

int admissionControl(
    Connection *req,
    map<pair<int,int>, vector<Edge*>> &shortestPath,
    map<pair<int,int>, vector<Edge*>> &altPath,
    vector<Node*> &nodes
) {
    int u = req->u, v = req->v;
    if (canUsePath(req, shortestPath[{u, v}])) {
        setupVirtualCircuit(req, 1, shortestPath[{u, v}], nodes);
        return 1;
    }
    else if (canUsePath(req, altPath[{u, v}])) {
        setupVirtualCircuit(req, 2, altPath[{u, v}], nodes);
        return 2;
    }
    else {
        return 0;
    }
}

// Print routing, forwarding and paths table
void clearFile(string &fileName) {
    ofstream fout;
    fout.open(fileName, ofstream::out | ofstream::trunc);
    fout.close();
}

void printPath(ofstream &fout, int source, int destination, vector<Edge*> &path, int &pathCost, int &pathDelay) {
    for (auto link : path) {
        fout << link->u << "->";
        pathDelay += link->delay;
        pathCost += (flag == "hop" ? 1 : link->delay); 
    }
    fout << destination << ",";
}

void printRoutingTable(int n, map<pair<int,int>, vector<Edge*>> &shortestPath, map<pair<int,int>, vector<Edge*>> &altPath) {
    clearFile(routingTableFile);
    ofstream fout;
    fout.open(routingTableFile);

    fout << "Source,Destination,Path,PathDelay,PathCost" << endl;
    int pathDelay, pathCost;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            if (shortestPath[{i, j}].size() > 0) {
                pathDelay = 0;
                pathCost = 0;
                fout << i << "," << j << ",";
                printPath(fout, i, j, shortestPath[{i, j}], pathCost, pathDelay);
                fout << pathDelay << "," << pathCost << endl;
            }
            if (altPath[{i, j}].size() > 0) {
                pathDelay = 0;
                pathCost = 0;
                fout << i << "," << j << ",";
                printPath(fout, i, j, altPath[{i, j}], pathCost, pathDelay);
                fout << pathDelay << "," << pathCost << endl;
            }
        }
    }

    fout.close();
} 

void printFowardingTable(int n, vector<Node*> &nodes) {
    clearFile(forwardingTableFile);
    ofstream fout;
    fout.open(forwardingTableFile);

    fout << "RouterId,InPort,InVCID,OutPort,OutVCID" << endl;
    for (int i = 0; i < n; i++) {
        for (auto entry : nodes[i]->forwardingTable) {
            fout << i << ",";
            fout << entry.first.first << ",";
            fout << entry.first.second << ",";
            fout << entry.second.first << ",";
            fout << entry.second.second << endl;
        }
    }

    fout.close();
}

void printPathsFile(
    vector<Connection*> &requests,
    map<pair<int,int>, vector<Edge*>> &shortestPath,
    map<pair<int,int>, vector<Edge*>> &altPath,
    int path1Count, int path2Count, int noPathCount
) {
    clearFile(pathsFile);
    ofstream fout;
    fout.open(pathsFile);
    
    // Print setup success count
    fout << path1Count << " connections use shortest path." << endl;
    fout << path2Count << " connections use alternate path." << endl;
    fout << noPathCount << " connections could not be setup." << endl;
    // Print paths used by circuits
    fout << "ConnectionId,Source,Destination,Path,VCIDlist,PathCost" << endl;
    int connectionsSetup = 0;
    for (int i = 0; i < requests.size(); i++) {
        if (requests[i]->path == 0) continue;
        connectionsSetup++;
        
        int u = requests[i]->u, v = requests[i]->v;
        int pathCost = 0, pathDelay = 0;
        fout << i << "," << u << "," << v << ",";
        if (requests[i]->path == 1) {
            printPath(fout, u, v, shortestPath[{u, v}], pathCost, pathDelay); // Path
        }
        else {
            printPath(fout, u, v, altPath[{u, v}], pathCost, pathDelay); // Path
        }
        for (int j = 0; j < requests[i]->vcids.size(); j++) {
            fout << requests[i]->vcids[j];
            fout << (j == requests[i]->vcids.size() - 1 ? "," : "->");
        }
        fout << pathCost << endl;
    }

    fout.close();
}

int main(int argc, char **argv) {
    if (argc != 5) {
        throw invalid_argument("Format: ./a.out topologyFile connectionFile flag p\n");
    }
    string topologyFile(argv[1]);
    string connectionsFile(argv[2]);
    flag = argv[3];
    if (flag != "hop" && flag != "dist") {
        throw invalid_argument("flag must be either \"hop\" or \"dist\"\n");
    }

    string usePessimistic(argv[4]);
    if (usePessimistic != "0" && usePessimistic != "1") {
        throw invalid_argument("usePessimistic must be either \"0\" or \"1\"\n");
    }   
    optimist = (usePessimistic == "0" ? true : false);

    ifstream fin;
    try {
        // Routing
        fin.open(topologyFile);
        int n, m;
        fin >> n >> m;
        vector<Node*> nodes(n);
        for (int i = 0; i < n; i++) {
            nodes[i] = new Node(i);
        }
        vector<vector<Edge*>> adj(n, vector<Edge*> (n, NULL));

        int u, v, d, b;
        float r;
        for (int i = 0; i < m; i++) {
            fin >> u >> v >> d >> b >> r;
            adj[u][v] = new Edge(u, v, d, b);
            adj[v][u] = new Edge(v, u, d, b);
        }
        fin.close();
        map<pair<int,int>, vector<Edge*>> shortestPath, altPath;
        bellmanFord(adj, shortestPath);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                findAlternatePath(i, j, adj, shortestPath[{i, j}], altPath);
            }
        }
        printRoutingTable(n, shortestPath, altPath);

        // Connections
        fin.open(connectionsFile);
        int c;
        fin >> c;
        vector<Connection*> connections(c);
        int bmin, bavg, bmax;
        int path1 = 0, path2 = 0, noPath = 0; 
        for (int i = 0; i < c; i++) {
            fin >> u >> v >> bmin >> bavg >> bmax;
            connections[i] = new Connection(u, v, bmin, bavg, bmax);
            int result = admissionControl(connections[i], shortestPath, altPath, nodes);
            if (result == 1) path1++;
            else if (result == 2) path2++;
            else noPath++;
        } 
        cout << path1 << " connections use shortest path." << endl;
        cout << path2 << " connections use alternate path." << endl;
        cout << noPath << " connections could not be setup." << endl;
        printFowardingTable(n, nodes);
        printPathsFile(connections, shortestPath, altPath, path1, path2, noPath);
        fin.close();
    } catch (exception e){
        throw e;
    }
}