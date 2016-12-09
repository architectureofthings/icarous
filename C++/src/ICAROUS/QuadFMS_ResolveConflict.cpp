/**
 * Quad Flight Management System
 *
 * Core flight management functions
 *
 * Contact: Swee Balachandran (swee.balachandran@nianet.org)
 *
 *
 * Copyright (c) 2011-2016 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 *
 * Notices:
 *  Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space Administration.
 *  All rights reserved.
 *
 * Disclaimers:
 *  No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER EXPRESSED,
 *  IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS, ANY
 *  IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT,
 *  ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED,
 *  WILL CONFORM TO THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN ENDORSEMENT BY GOVERNMENT
 *  AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS, RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
 *  RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY DISCLAIMS ALL WARRANTIES AND
 *  LIABILITIES REGARDING THIRD-PARTY SOFTWARE, IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:
 *   RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST THE UNITED STATES GOVERNMENT,
 *   ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE
 *   RESULTS IN ANY LIABILITIES, DEMANDS, DAMAGES,
 *   EXPENSES OR LOSSES ARISING FROM SUCH USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
 *   RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD HARMLESS THE UNITED STATES GOVERNMENT,
 *   ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
 *   RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE, UNILATERAL TERMINATION OF THIS AGREEMENT.
 */

#include "QuadFMS.h"

void QuadFMS_t::ResolveFlightPlanDeviation(){

	double xtrkDevGain ;
	double resolutionSpeed;
	double allowedDev;
	double elapsedTime;
	double Vs,Vf,V,sgn;
	double Trk;
	double headingNextWP;
	double dn,de;
	Position prevWP,nextWP,currentPos,cp;
	Plan currentFP;

	xtrkDevGain     = FlightData->paramData->getValue("XTRK_GAIN");
	resolutionSpeed = FlightData->paramData->getValue("RES_SPEED");
	allowedDev      = FlightData->paramData->getValue("XTRK_DEV");

	currentPos = FlightData->acState.positionLast();

	currentFP = FlightData->MissionPlan;
	elapsedTime = GetApproxElapsedPlanTime(currentFP,FlightData->nextMissionWP);
	prevWP = currentFP.point(FlightData->nextMissionWP - 1).position();
	nextWP = currentFP.point(FlightData->nextMissionWP).position();

	if(xtrkDevGain < 0){
		xtrkDevGain = -xtrkDevGain;
	}

	if(fabs(FlightData->crossTrackDeviation) <= 3*allowedDev){
		Vs = xtrkDevGain*FlightData->crossTrackDeviation;
		V  = resolutionSpeed;

		if(Vs >= 0){
			sgn = 1;
		}
		else{
			sgn = -1;
		}

		if(pow(fabs(Vs),2) >= pow(V,2)){
			Vs = V*sgn;
		}

		Vf = sqrt(pow(V,2) - pow(Vs,2));

		Trk = prevWP.track(nextWP);
		FlightData->manueverVn = Vf*cos(Trk) - Vs*sin(Trk);
		FlightData->manueverVe = Vf*sin(Trk) + Vs*cos(Trk);
		FlightData->manueverVu = 0;

		FlightData->manueverHeading = atan2(FlightData->manueverVe,FlightData->manueverVn)*180/M_PI;
		if(FlightData->manueverHeading < 0){
			FlightData->manueverHeading = 360 + FlightData->manueverHeading;
		}

		planType = MANEUVER;

	}
	else{
		headingNextWP = prevWP.track(nextWP);
		dn = FlightData->crossTrackOffset*cos(headingNextWP);
		de = FlightData->crossTrackOffset*sin(headingNextWP);

		cp = prevWP.linearEst(dn,de); // closest point on path

		FlightData->manueverHeading = currentPos.track(cp);
		if(FlightData->manueverHeading < 0){
			FlightData->manueverHeading = 360 + FlightData->manueverHeading;
		}

		double distance = currentPos.distanceH(cp);
		double ETA      = distance/resolutionSpeed;
		NavPoint wp1(currentPos,0);
		NavPoint wp2(cp,ETA);
		FlightData->ResolutionPlan.clear();
		FlightData->ResolutionPlan.add(wp1);
		FlightData->ResolutionPlan.add(wp2);

		planType      = TRAJECTORY;
		resumeMission = false;
	}
}

void QuadFMS_t::ResolveKeepInConflict(){
	Geofence_t fence = Conflict.GetKeepInConflict();
	NavPoint wp(fence.GetRecoveryPoint(),0);
	std::cout<<wp.position().toStringUnits("degree","degree","m")<<std::endl;
	NavPoint next_wp = FlightData->MissionPlan.point(FlightData->nextMissionWP);
	FlightData->ResolutionPlan.clear();
	FlightData->ResolutionPlan.add(wp);
	FlightData->nextResolutionWP = 0;

	if(fence.CheckWPFeasibility(wp.position(),next_wp.position())){
		FlightData->nextMissionWP++;
	}

	planType        = TRAJECTORY;
	resumeMission   = false;
	return;
}

void QuadFMS_t::ResolveKeepOutConflict(){
	double gridsize          = FlightData->paramData->getValue("GRIDSIZE");
	double buffer            = FlightData->paramData->getValue("BUFFER");
	double lookahead         = FlightData->paramData->getValue("LOOKAHEAD");
	double proximityfactor   = FlightData->paramData->getValue("PROXFACTOR");

	// Reroute flight plan
	SetMode(GUIDED); // Set mode to guided for quadrotor to hover before replanning

	std::vector<Vect3> TrafficPos;
	std::vector<Vect3> TrafficVel;

	Position currentPos = FlightData->acState.positionLast();
	Velocity currentVel = FlightData->acState.velocityLast();

	RRT_t RRT(FlightData->fenceList,currentPos,currentVel,TrafficPos,TrafficVel);

	Position nextWP = FlightData->MissionPlan.point(FlightData->nextMissionWP).position();
	int Nsteps = 500;

	for(int i=0;i<Nsteps;i++){
		RRT.RRTStep();
		if(RRT.CheckGoal(nextWP)){
			break;
		}
	}

	FlightData->ResolutionPlan = RRT.GetPlan();
	planType        = TRAJECTORY;
	resumeMission   = false;
	return;

}




