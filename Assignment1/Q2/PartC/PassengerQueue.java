import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

public class PassengerQueue{
    ReentrantLock accessLock;
    Queue<Passenger> queue;
    int capacity;

    PassengerQueue(int capacity){
        this.accessLock = new ReentrantLock();
        this.queue  = new LinkedList<>(); 
        this.capacity = capacity;
    }

    int returnSize() {
        return queue.size();
    }

    boolean addPassenger(Passenger passenger){
        accessLock.lock();
        if (queue.size() < capacity) {
            queue.add(passenger);
            accessLock.unlock();
            return true;
        }
        else {
            accessLock.unlock();
            return false;
        }
    }

    Passenger extractPassenger(long selfTime){
        accessLock.lock();
        Passenger passenger = queue.poll();
        if(passenger != null){
            passenger.queueLeavingTime = selfTime;
        }
        accessLock.unlock();
        return passenger;
    }
}