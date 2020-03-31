# How to run
1. Navigate to a specific subfolder say ```cd PartA```
2. Compile using command ```javac Airport.java```
3. Run the file using command ```java Airport```
4. Enter the arrival mean rate, service mean rate and the simulation time.
5. The simulated results value as well as the theoretical values will be displayed in the terminal.
See ```sample.txt``` for a sample run.

# Assumptions
1. Given arrival mean rate and service mean rate are sufficent enough (of the order 0.0x) to generate a significant time gap to run simulation.
For example, given arrival mean rate 0.02 the average gap between arrival of passengers comes out to be 50 allowing simulation to converge.
But if meanArrivalRate = 2 then meanInterArrivalTime = 1/2 = 0.5 which occurs frequently will be heavily distorted by rounding to either 0 or 1.

2. The simulation time entered should be long enough to generate sufficent number of passengers to allow the simulation to converge.
For example, if meanArrivalRate = 0.02 then meanInterArrivalTime = 1 / 0.02 = 50. So, for 1000 passengers, simulation time should be 50,000.

3. For simulation purpose it is expected that the given arrival mean rate should be less than the service mean rate to have a stable system else that will lead to an unstable system resulting in arbitary theortical value.
