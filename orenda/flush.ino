

/**
 * Flush system.
 * 
 * Turn off heater
 * 
 * Recirculate off
 * Run pump1, 2, 3.
 */


#include "orenda.h"


static bool neverSawWater = true;  // Never saw the water marker
static bool pump1Finished = false; // No more water from reservior

//static lcDirection direction = lcDirectionUnknown;
static unsigned long lastWater = 0;
static unsigned long lastEven  = 0;
static unsigned long lastPump2 = 0;



void flushSetup() {
   Particle.function("flushWater", flushWaterCommand);
}

static int flushWaterCommand(String command) {
  int power = parsePower(command);
    
  if (power == -1) return -1;
    
  // Stopping flush
  if (power == 0) {
    powerDown();
    return 1;
  }
    
  // Start and setup
    
  heaterAndPumpsOff();
  neverSawWater = true;
  lastWater = lastEven = lastPump2 = millis();
  pump1Finished = false;   
  
  setState(orendaFlush);
    
  return 1;
}

 
 /**
  * Called every 200ms during flush
  */ 
 
static lcDirection lastDirection = lcDirectionUnknown;
 
 
void flushProcess(double lcValue, lcDirection direction, bool chamberF) {
  unsigned int now = millis();
  
  Particle.publish("flush/chamber", chamberF ? "full" : "empty", 0, PRIVATE);
  
  if (direction == lcDirectionUp) {
    lastWater = now;
    
  } if (direction == lcDirectionEven) {
    // We want the load cell to be unchanging for 8 seconds as an exit condition
    if (direction != lcDirectionEven) {
      lastEven = now;
     } 
  }  
  
  
  Particle.publish("flush/direction", String(lcValue) + " "  + lcDirectionName(direction), 0, PRIVATE);
  
  // Always run pump 3
  digitalWrite(pump3, HIGH);
  
  // Note that because pump2 is continually draining, the water sensor
  // might not get triggered.   For this reason, if the direction is up, also
  // set lastWater above.
  
  if (chamberF) {
    // Definitely water there in chamber
    neverSawWater = false;  
    lastWater = now;
    
    if (pump1Finished) {
      pump1Finished = false;
      Particle.publish("flush/pump1", "water again", 0, PRIVATE);
    }
    
  } else if (!pump1Finished) {
    // Run for at least 40 seconds since water was last seen, then indicate pump1 is finished  
        
    if ((now - lastWater) > (40 * 1000)) {
      // Expired
      Particle.publish("flush/pump1", "now finished", 0, PRIVATE);
      pump1Finished = true;
    }
  }
  
 
  
  Particle.publish("flush/pump1", pump1Finished ? "finished" : "not finished", 0, PRIVATE);
  digitalWrite(pump1, pump1Finished ? LOW : HIGH);

  // Run pump2 if we saw water in the last 40 seconds, and the load cell is not 
  // increasing
  
  if ((direction != lcDirectionUp) && !pump1Finished) {
    //Particle.publish("flush/on2/lastWater", String(now - lastWater), 0, PRIVATE); 
    digitalWrite(pump2, HIGH);  
    lastPump2 = now;
  } else {
    ///Particle.publish("flush/off2/lastWater", String(now - lastWater), 0, PRIVATE); 
    digitalWrite(pump2, LOW);  
  }   
  
  
  // Consider finish
  
  // Finished when:
  //   No water in chamber
  //   Load cell reading is staying the same for at least 8 seconds. 
  //   It's been 8 seconds since pump2 turned off
  //   Pump 1 has finished
  
  if (!chamberF && direction == lcDirectionEven && pump1Finished) {
    if (((now - lastEven) > 8000) && ((now - lastPump2) > 8000)) {
      Particle.publish("flush/finish", "flush complete");
      powerDown();
    }
  }
}
 
 

 
