/**
 * Example program to test VOLDEMORT
 * Contact: Swee Balachandran (swee.balachandran@nianet.org)
 * 
 * 
 * Copyright (c) 2011-2016 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 */
import gov.nasa.larcfm.ICAROUS.*;
import gov.nasa.larcfm.MISSION.*;
import java.io.*;

public class Voldemort_test{
    
    public static void main(String args[]){

	boolean verbose   = false;
	String px4port = null;
	String bcastgroup = null;
	int sitlport      = 0;
	int bcastport     = 0;
	int comport       = 0;

	// Process input arguments
	for(int i=0;i<args.length && args[i].startsWith("-");i++){
	    if(args[i].startsWith("-v")){
		verbose = true;
	    }

	    else if(args[i].startsWith("--px4")){
		px4port = args[++i];
	    }

	    else if(args[i].startsWith("--sitl")){
		sitlport = Integer.parseInt(args[++i]);
	    }

	    else if(args[i].startsWith("--com")){
		comport = Integer.parseInt(args[++i]);		
	    }

	    else if(args[i].startsWith("--bc")){
		bcastgroup = args[++i];
		bcastport  = Integer.parseInt(args[++i]);
	    }

	    else if(args[i].startsWith("-")) {
		System.out.println("Invalid option "+args[i]);
		System.exit(0);
	    }
	}

	AircraftData FlightData    = new AircraftData();
	
	
	Interface SITLInt    = new Interface(Interface.SOCKET,
					   null,
					   sitlport,
					   0,
					   FlightData);
	
	Interface COMInt   = new Interface(Interface.SOCKET,
					   null,
					   comport,
					   0,
					   FlightData);

	Interface BCASTInt = new Interface(Interface.SOCKET,
					   bcastgroup,
					   0,
					   bcastport,
					   FlightData);
	
	Demo test = new Demo();
	
	Aircraft uasQuad  = new Aircraft(SITLInt,COMInt,FlightData,test);

	uasQuad.error.setConsoleOutput(verbose);
		
	COM com_module    = new COM("Communications",uasQuad);
	BCAST bcast_module  = new BCAST("Broadcast",uasQuad,BCASTInt);
	
	com_module.error.setConsoleOutput(verbose);
	uasQuad.error.setConsoleOutput(verbose);
	test.error.setConsoleOutput(true);			
	    	
	bcast_module.start();
	
	try{
	    Thread.sleep(1000);
	}catch(InterruptedException e){
	    System.out.println(e);
	}

	com_module.start();	
	
    }

}
