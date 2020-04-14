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
        int numberOfPorts;     //number of input ports in switch
        int inputBufferSize;        //size of input buffer in each input port
        int outputBuffferSize;
        double packetGenProb;
        int maxTimeSlots;
        long cumulativeLinkUtilisation;   

        vector<queue<Packet>> inputPorts;
        vector<queue<Packet>> outputPorts;
        vector<Packet> transmittedPackets; 

        BaseSwitch(int n, int bufferSize, double packetGenProb, int maxTimeSlots) {
            this->numberOfPorts = n;
            this->inputBufferSize = bufferSize;
            this->outputBuffferSize = 1;
            this->packetGenProb = packetGenProb;
            this->maxTimeSlots = maxTimeSlots;
            this->cumulativeLinkUtilisation = 0;
        }
        
        int randomWithProb(double p) {
            double rndDouble = (double) rand() / RAND_MAX;
            return rndDouble <= p;
        }    

        void generateInputPackets(long slotNumber) {
            for( auto port : inputPorts){
                if(port.size() == inputBufferSize)
                    continue;
                int gen = randomWithProb(packetGenProb);
                if(gen){
                    int targetOutputPort = rand()% (int)(outputPorts.size());
                    Packet newPacket = Packet(slotNumber,targetOutputPort);
                    port.push(newPacket);
                }
            }
        }

        void transmission(long slotNumber) {
            for (int i = 0; i < outputPorts.size(); i++) {
                if (outputPorts[i].empty()) continue;
                Packet packet = outputPorts[i].front();
                packet.transmissionTime = slotNumber;
                transmittedPackets.push_back(packet);
                outputPorts[i].pop();
            }
        }
};


class INQ : public BaseSwitch{
    
    INQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots):BaseSwitch(n,bufferSize,packetGenProb,maxTimeSlots){
    }

    void scheduleByINQ() {
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
            inputOutputMapping[j] = i;    
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

class KOUQ : public BaseSwitch{
    int K;
    long dropOutputPort;

    KOUQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots,int K): BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots){
        this->K = K;
        this->dropOutputPort = 0;
    }
    void scheduleByKOUQ(){
        // vector<vector<int>> options(numberOfPorts);
        vector<vector<Packet>> options(numberOfPorts);
        
        // For each output port, find the input ports that target it
        for (auto inputPort : inputPorts) {
            if (inputPort.empty()) continue;
            Packet headOfLine = inputPort.front();
            inputPort.pop();
            int outputPort = headOfLine.outputPort;
            options[outputPort].push_back(headOfLine); 
        }
        
         // Now, each output port randomly selects atmost K packets
        for (int i = 0; i < numberOfPorts; i++) {
            if (options[i].size() > K){
                this->dropOutputPort++;
                random_shuffle(options[i].begin(),options[i].end());
            }
            for(int j = 0; j < min(K,(int) options[i].size());j++){
                if(outputPorts[i].size()== outputBuffferSize)
                    break;
                cumulativeLinkUtilisation++;
                outputPorts[i].push(options[i][j]);
            } 
        }
        options.clear();
    }
};


class SwitchISLIP : public BaseSwitch {
    vector<vector<queue<Packet>>> inputPorts;
    vector<int> inputPortsBufferUsed;

    SwitchISLIP(int n, int bufferSize, double packetGenProb, int maxTimeSlots)
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots)
    {
        inputPorts.resize(n, vector<queue<Packet>> (n));
        inputPortsBufferUsed.resize(n, 0);
    }

    void generatePackets(long slotNumber) {
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputPortsBufferUsed[i] == inputBufferSize) continue;
            
            int gen = randomWithProb(packetGenProb);
            if (gen) {
                int targetOutputPort = rand() % outputPorts.size();
                Packet newPacket = Packet(slotNumber, targetOutputPort);
                inputPorts[i][targetOutputPort].push(newPacket);
                inputPortsBufferUsed[i]++;
            }
        }
    }

    void schedule() {
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
            vector<int> grantMap(numberOfPorts, -1);
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                for (int ip = 0; ip < numberOfPorts; ip++) {
                    if (!ipAvailable[op]) continue;
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

}

int main() {
    srand(time(0));
    // Check if transmission time has been stamped
}

