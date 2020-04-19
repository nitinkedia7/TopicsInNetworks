#include <bits/stdc++.h>
using namespace std;

/* Packet class implements the attributes of a packet*/
class Packet {
    public :
    int arrivalTime;            //arrival time of packet
    int transmissionTime;       //transmission completion Time
    int outputPort;             //destined output port 

    /* Constructor to initialize member variables*/
    Packet(int arrivalTime, int outputPort) {
        this->arrivalTime = arrivalTime;
        this->outputPort = outputPort;
    }
};

class BaseSwitch {
    public:
    int numberOfPorts;
    int inputBufferSize;
    int outputBuffferSize;
    double packetGenProb;
    int maxTimeSlots;
    long long cumulativeLinkUtilisation; 
    double averageLinkUtilization;
    long long cumulativePacketDelay;
    double avgPacketDelay;  
    double stddevPacketDelay;
    long dropPacketCount;
    double dropRatio;
    string outputFileName;
    string queueType;

    vector<queue<Packet>> inputPorts;
    vector<queue<Packet>> outputPorts;
    vector<Packet> transmittedPackets; 

    BaseSwitch(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName) {
        this->numberOfPorts = n;
        this->inputBufferSize = bufferSize;
        this->outputBuffferSize = 1;
        this->packetGenProb = packetGenProb;
        this->maxTimeSlots = maxTimeSlots;
        this->cumulativeLinkUtilisation = 0;
        this->cumulativePacketDelay = 0;
        this->dropPacketCount = 0;
        this->outputFileName = outputFileName;
        this->dropRatio = 0.0;
        inputPorts.resize(n);
        outputPorts.resize(n);
    }
    
    bool randomWithProb(double p) {
        double rndDouble = ((double) rand()) / RAND_MAX;
        return rndDouble <= p;
    }    

    virtual void generateInputPackets(int slotNumber) {
        for (int i = 0 ;i< numberOfPorts; i++) {
            if (inputPorts[i].size() == inputBufferSize){
                dropPacketCount++;
                continue;   
            }

            if (randomWithProb(packetGenProb)) {
            //if (1) {
                // cout << "PACKET GEN1" << endl;
                int targetOutputPort = rand() % numberOfPorts;
                Packet newPacket = Packet(slotNumber,targetOutputPort);
                inputPorts[i].push(newPacket);
            }
        }
    }

    virtual void schedule() {
        cout << "ERROR : VIRTUAL SCHEDULE RAN" << endl;
        // Dummy virtual function to be defined for each type of queue
    }

    void transmission(int slotNumber) {
        // cout << 'T';
        for (int i = 0; i < outputPorts.size(); i++) {
            if (outputPorts[i].empty()) continue;
            // cout << "PACKET TRANSMITTED" << endl;
            Packet packet = outputPorts[i].front();
            packet.transmissionTime = slotNumber;
            transmittedPackets.push_back(packet);
            outputPorts[i].pop();
        }
    }
    
    void metricsCalculation() {
        int noOfTransmittedPacket = transmittedPackets.size();

        for (int i = 0; i < noOfTransmittedPacket; i++) {
            cumulativePacketDelay += (transmittedPackets[i].transmissionTime - transmittedPackets[i].arrivalTime + 1);
        }
        avgPacketDelay = ((double) cumulativePacketDelay) / noOfTransmittedPacket;

        long long variancePacketDelay = 0;
        for(int i = 0 ; i < noOfTransmittedPacket; i++) {
            variancePacketDelay += pow(transmittedPackets[i].transmissionTime - transmittedPackets[i].arrivalTime - avgPacketDelay, 2);
        }
        stddevPacketDelay = sqrt(((double)variancePacketDelay) / noOfTransmittedPacket);

        averageLinkUtilization = (1.0 * cumulativeLinkUtilisation) / (numberOfPorts * maxTimeSlots);

        dropRatio = 1.0 * dropPacketCount / ( dropPacketCount + (int) transmittedPackets.size());

        cout<< noOfTransmittedPacket << " : " << avgPacketDelay << " : " << stddevPacketDelay << " : " << averageLinkUtilization <<endl;
    }


    void printMetrics() {
        ofstream fout;
        fout.open(outputFileName, ios::app);
        fout << numberOfPorts << "," << packetGenProb << "," << queueType << "," << inputBufferSize << "," << "NA" << "," ;
        fout<< dropRatio << ",";
        fout << avgPacketDelay << "," << stddevPacketDelay << "," << averageLinkUtilization << "," << "NA" << endl;
        fout.close();
    }

    void run() {
        for (int slotNumber = 0; slotNumber < maxTimeSlots; slotNumber++) {
            transmission(slotNumber);
            schedule();
            generateInputPackets(slotNumber);
        }
    }
};

class INQ : public BaseSwitch {
    public:
    
    INQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName)
        : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName) {
            queueType = "INQ";
        }

    void schedule() {
        // cout << "INQ Schedule" << endl;
        vector<vector<int>> options(numberOfPorts);
    
        // For each output port, find the input ports that target it
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputPorts[i].empty()) continue;
            int outputPort = inputPorts[i].front().outputPort;
            options[outputPort].push_back(i); 
        }

        vector<int> inputOutputMapping(numberOfPorts, -1);

        // Now, each output port randomly selects an input port
        for (int i = 0; i < numberOfPorts; i++) {
            if (options[i].size() == 0) continue;
            int j = rand() % options[i].size();
            inputOutputMapping[options[i][j]] = i;    
        }
        options.clear();
        
        // Transfer packets from input to output port using the mapping obtained above
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputOutputMapping[i] == -1) continue;
            Packet p = inputPorts[i].front();
            inputPorts[i].pop();
            
            int outputPortId = inputOutputMapping[i];
            if (outputPorts[outputPortId].size() < outputBuffferSize) {
                outputPorts[outputPortId].push(p);
            }
        }
        // Update link utilisation parameters
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputOutputMapping[i] != -1) {
                cumulativeLinkUtilisation++;
            }
        }
    }
};

class KOUQ : public BaseSwitch {
    public:
    int K;
    double K_ratio;
    long dropOutputPort;
    double dropProbablity;

    KOUQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots, double K_ratio, string outputFileName) 
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName) {
        this->K = K_ratio * n;
        this->K_ratio = K_ratio; 
        this->dropOutputPort = 0;
        this->queueType = "KOUQ";
        this->outputBuffferSize = bufferSize;
    }

    void schedule(){
        vector<vector<Packet>> options(numberOfPorts);
        
        // For each output port, find the input ports that target it
        for (int i = 0; i < numberOfPorts; i++){
            if (inputPorts[i].empty()) continue;
            Packet headOfLine = inputPorts[i].front();
            inputPorts[i].pop();
            int outputPort = headOfLine.outputPort;
            options[outputPort].push_back(headOfLine); 
        }
        
        // Now, each output port randomly selects atmost K packets
        for (int i = 0; i < numberOfPorts; i++) {
            if (options[i].size() > K) {
                this->dropOutputPort++;
                random_shuffle(options[i].begin(),options[i].end());
                dropPacketCount += (int)options[i].size() - K;
            }
            for(int j = 0; j < min(K,(int) options[i].size());j++){
                if (outputPorts[i].size() == outputBuffferSize){
                    dropPacketCount +=  min(K,(int) options[i].size()) - j;
                    break;
                } 
                cumulativeLinkUtilisation++;
                outputPorts[i].push(options[i][j]);
            }   
        }
        options.clear();
    }

    void calcDropprobability() {
        dropProbablity = (100.0 * dropOutputPort) / (numberOfPorts * maxTimeSlots);
    }

    void printMetrics() {
        ofstream fout;
        fout.open(outputFileName, ios::app);
        fout << numberOfPorts << "," << packetGenProb << "," << queueType << ",";
        fout << inputBufferSize << "," << K_ratio << ",";
        fout<< dropRatio << ",";
        fout << avgPacketDelay << "," << stddevPacketDelay << "," << averageLinkUtilization << "," << dropProbablity << endl;
        fout.close();
    }
};

class SwitchISLIP : public BaseSwitch {
    public:
    vector<vector<queue<Packet>>> inputPorts;
    vector<int> inputPortsBufferUsed;

    SwitchISLIP(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName)
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName)
    {
        inputPorts.clear();
        inputPorts.resize(n, vector<queue<Packet>> (n));
        inputPortsBufferUsed.resize(n);
        fill(inputPortsBufferUsed.begin(), inputPortsBufferUsed.end(), 0);
        queueType = "iSLIP";
    }

    void generateInputPackets(int slotNumber) {
        // cout << "G" << endl; 
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputPortsBufferUsed[i] == inputBufferSize){
                dropPacketCount++;
                continue;   
            }
            
            if (randomWithProb(packetGenProb)) {
                int targetOutputPort = rand() % outputPorts.size();
                Packet newPacket = Packet(slotNumber, targetOutputPort);
                inputPorts[i][targetOutputPort].push(newPacket);
                inputPortsBufferUsed[i]++;
            }
        }
    }

    void schedule() {
        // cout << 'S';
        vector<int> inputOutputMapping(numberOfPorts, -1); 
        vector<vector<int>> v(numberOfPorts, vector<int> (numberOfPorts, 0));
        for (int ip = 0; ip < numberOfPorts; ip++) {
            for (int op = 0; op < numberOfPorts; op++) {
                if (!inputPorts[ip][op].empty()) {
                    v[ip][op] = 1;
                }
            }
        }
        vector<int> ipAvailable(numberOfPorts, 1);
        vector<int> opAvailable(numberOfPorts, 1);
        
        bool changed = true;
        while (changed) {
            changed = false;
            vector<int> grantMap(numberOfPorts, -1); // op -> ip
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                for (int ip = 0; ip < numberOfPorts; ip++) {
                    if (!ipAvailable[ip]) continue;
                    if (v[ip][op] == 1) {
                        grantMap[op] = ip;
                        break;
                    }
                }
            }
            
            vector<int> acceptMap(numberOfPorts, -1); // ip -> op
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                
                int ip = grantMap[op];
                if (ip != -1 && acceptMap[ip] == -1) {
                    changed = true;
                    acceptMap[ip] = op;
                }
            }

            for (int ip = 0; ip < numberOfPorts; ip++) {
                if (acceptMap[ip] != -1) {
                    inputOutputMapping[ip] = acceptMap[ip];
                    ipAvailable[ip] = 0;
                    opAvailable[acceptMap[ip]] = 0; 
                }
            }
            grantMap.clear();
            acceptMap.clear();
        }
        v.clear();

        // Transfer packets from input to output port using the mapping obtained above
        for (int ip = 0; ip < numberOfPorts; ip++) {
            int op = inputOutputMapping[ip];
            if (op == -1) continue;
            Packet p = inputPorts[ip][op].front();
            inputPorts[ip][op].pop();
            inputPortsBufferUsed[ip]--;

            if (outputPorts[op].size() < outputBuffferSize) {
                outputPorts[op].push(p);
            }
        }

        // Update link utilisation parameters
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputOutputMapping[i] != -1) {
                cumulativeLinkUtilisation++;
            }
        }
    }
};

void printTableHeaders(string outputFileName) {
    ofstream fout;
    fout.open(outputFileName, ios::trunc);
    fout << "Number of Ports,Packet Generation Prob,Queue Type,Buffer Size,Knockout,Drop Ratio,Average Packet Delay,Std. Dev. of Packet Delay,Average Link Utilisation,KOUQ Drop Probability" << endl;
    fout.close();
}

void runSimulation(int n, int l, double k, double prob, int timeslot, string scheduleType, string outputFile) {
    if (scheduleType == "INQ") {
        INQ switchInstance(n, l, prob, timeslot, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.printMetrics();
    }
    else if (scheduleType == "KOUQ") {
        KOUQ switchInstance(n, l, prob, timeslot, k, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.calcDropprobability();
        switchInstance.printMetrics();
    }
    else if (scheduleType == "iSLIP") {
        SwitchISLIP switchInstance(n, l, prob, timeslot, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.printMetrics();
    }
    else {
        cout << "ERROR: Invalid scheduling type " << scheduleType << endl;
    }
}

void getGraphs() {    
    vector<string> queueType = {"INQ", "KOUQ", "iSLIP"};

    // // 1. Change N
    string outputFile = "varyPortCount.csv";
    printTableHeaders(outputFile);
    // for (int i = 0; i < 3; i++) {
        for (int n = 4; n <= 32; n += 2) {
            runSimulation(n, 4, 0.6, 0.5, 10000, "iSLIP", outputFile);
        }    
    // }

    // 2. Change L
    outputFile = "varyBufferSize.csv";
    printTableHeaders(outputFile);
    // for (int i = 0; i < 3; i++) {
        for (int l = 1; l <= 4; l++) {
            runSimulation(8, l, 0.6, 0.5, 10000, "iSLIP", outputFile);
        }
    // }

    // // 3. Change K in KOUQ only
    // outputFile = "varyKnockout.csv";
    // printTableHeaders(outputFile);
    // for (double k = 0.2; k <= 1.0; k += 0.1) {
    //     runSimulation(8, 4, k, 0.5, 10000, "KOUQ", outputFile);
    // }

    // 4. Change p
    outputFile = "varyGenProb.csv";
    printTableHeaders(outputFile);
    // for (int i = 0; i < 3; i++) {
        for (double p = 0.1; p <= 1.0; p += 0.1) {
            runSimulation(8, 4, 0.6, p, 10000, "iSLIP", outputFile);
        }
    // }
    return;
}

// void getSimulationResult(string outputfile){
//     printTableHeaders(outputfile);
    
//     // runSimulation(8, 4, 0.0, 0.5, 10000, "INQ", outputfile);
//     for (int n = 4; n <= 16; n += 2) {
//         for (int l = 2; l <= 4; l++) {
//             runSimulation(n, l, 0.0, 0.5, 10000, "INQ", outputfile);       
//         }
//     }

//     // runSimulation(8, 4, 0.6 * 8, 0.5, 10000, "KOUQ", outputfile);
//     for (int n = 4; n <= 16; n += 2) {
//         for (int l = 2; l <= 4 ; l++) {
//             for (double k = 0.6; k <= 1.0; k += 0.2) {
//                 runSimulation(n, l, k*n, 0.5, 10000, "KOUQ", outputfile);
//             }
//         }
//     }
    
//     // runSimulation(8, 4, 0.0, 0.5, 10000, "iSLIP", outputfile);
//     for (int n = 4; n <= 16; n += 2) {
//         for (int l = 2; l <= 4; l++) {
//             runSimulation(n, l, 0.0, 0.5, 10000, "iSLIP", outputfile);       
//         }
//     }
// }

int main(int argc, char *argv[]) {
    srand(time(0));
    // Default values of switch parameters
    getGraphs();
    return 0;
    int switchPortCount = 8;
    int bufferSize = 4;
    double packetGenProb = 0.5;
    string queue = "INQ";
    int knockout = 0.6 * switchPortCount;
    string outputFile = "output.csv";
    int maxTimeSlots = 10000;

    for (int i = 1; i < argc; i += 2) {
        string parameter(argv[i] + 1);
        if (parameter == "N") {
            switchPortCount = atoi(argv[i+1]);
        }
        else if (parameter == "B") {
            bufferSize = atoi(argv[i+1]);
        }
        else if (parameter == "p") {
            packetGenProb = atof(argv[i+1]);   
        }
        else if (parameter == "queue") {
            queue = argv[i+1];
        }
        else if (parameter == "K") {
            knockout = atoi(argv[i+1]);
        }
        else if (parameter == "out") {
            outputFile = argv[i+1];
        }
        else if (parameter == "T") {
            maxTimeSlots = atoi(argv[i+1]);
        }
        else {
            cout << "Unknown parameter " << parameter << " skipping this and next paramter value";
        }
    } 
    printTableHeaders(outputFile);
    runSimulation(switchPortCount, bufferSize, knockout, packetGenProb, maxTimeSlots, queue, outputFile);
};
