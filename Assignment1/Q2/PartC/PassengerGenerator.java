import java.util.*;

public class PassengerGenerator implements Runnable{
    public long selfTime ;
    public double arrivalMean;
    public PassengerQueue queues[];
    public int noOfQueues;
    public int totalNoOfGeneratedPassengers;
    public int droppedPassengerCount;

    PassengerGenerator(double arrivalMean,PassengerQueue queues[] ,int noOfQueues){
        this.selfTime = 0;
        this.arrivalMean = arrivalMean;
        this.noOfQueues = noOfQueues;
        this.queues = queues;
        this.totalNoOfGeneratedPassengers = 0;
        this.droppedPassengerCount = 0;
    }

    public void run(){
            Passenger newPassenger =  new Passenger(selfTime);
            totalNoOfGeneratedPassengers++;
            // long interArrivalTime = (long) Math.round(Math.log(1-y)*(-arrivalMean)); 
            long interArrivalTime = Airport.getPoissonRandom(arrivalMean);
            selfTime += interArrivalTime;
            
            List<Integer> queueOrder = new ArrayList<Integer>();
            queueOrder.add(0);
            queueOrder.add(1);
            queueOrder.add(2);
            Collections.shuffle(queueOrder);
            for (int queueId : queueOrder) {
                if (queues[queueId].addPassenger(newPassenger)) {
                    return;
                }
            }
            // If all the above queues are full, the passenger is dropped.
            droppedPassengerCount++;
    }
}
