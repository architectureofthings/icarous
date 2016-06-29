package gov.nasa.larc.ICAROUS;
import com.MAVLink.enums.*;

public class FMS_Thread implements Runnable{

    public Thread t;
    public String threadName;
    public AircraftData SharedData;
    public AircraftData apState;
    public ICAROUS_Interface AP;
    
    public FMS_Thread(String name,AircraftData Input, ICAROUS_Interface apInterface){
	threadName       = name;
	SharedData       = Input;
	AP               = apInterface;
	apState          = new AircraftData(AircraftData.NO_MESSAGES);
    }

    public void run(){
		
	synchronized(SharedData){
	    SharedData.GetDataFromMessages();
	    apState.CopyAircraftStateInfo(SharedData);
	}
	
	this.FlightManagement();
    }

    
    public void start(){
	System.out.println("Starting "+threadName);
	t = new Thread(this,threadName);
	t.start();
    }

	
    public void FlightManagement(){
	
	/*
	ICAROUS functionalities go in here.
	 - use synchronized codeblocks to access shared data.
	 - use Px4.SendCommand(targetsystem, targetcomponent, confirmation, command, 
                                param1,...,param7)
	   to send commands to the autopilot.
	*/

	/* --------- Example Mission ---------- */
	
	// Set mode to guided (4 is the custom mode in the arudpilot code)
	AP.SetMode(4);
	
	// Arm the throttles
	AP.SendCommand(0,0,0,MAV_CMD.MAV_CMD_COMPONENT_ARM_DISARM,
		       1,0,0,0,0,0,0);

	// Takeoff at current location
	AP.SendCommand(0,0,0,MAV_CMD.MAV_CMD_NAV_TAKEOFF,
		       1,0,0,0, (float) apState.lat, (float) apState.lon, (float) apState.alt + 1.0f);
    }

    
}
