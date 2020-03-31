import java.util.*;

public class PassengerGenerator implements Runnable{
    public long selfTime ;
    public double arrivalMean;
    public PassengerQueue queues[];
    public int noOfQueues;
    public int totalNoOfGeneratedPassengers;

    PassengerGenerator(double arrivalMean,PassengerQueue queues[] ,int noOfQueues){
        this.selfTime = 0;
        this.arrivalMean = arrivalMean;
        this.noOfQueues = noOfQueues;
        this.queues = queues;
        this.totalNoOfGeneratedPassengers = 0;
    }

    public void run(){
            totalNoOfGeneratedPassengers++; 
            Passenger newPassenger =  new Passenger(selfTime);
            long interArrivalTime = Airport.getPoissonRandom(arrivalMean);
            selfTime += interArrivalTime;
            assert(interArrivalTime>=1) : "Inter Arrival time of generator < 1";
            
            Random random = new Random();
            int selectedQueue = random.nextInt(noOfQueues);
            queues[selectedQueue].addPassenger(newPassenger);
    }
}
