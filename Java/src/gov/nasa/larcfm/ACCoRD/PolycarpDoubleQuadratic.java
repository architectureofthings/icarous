/* 
 * PolycarpDoubleQuadratic - Determining if a quadratic in two variables ever passes below a threshold
 * 						   - for inputs in the unit box
 *
 * Contact: Anthony Narkawicz (anthony.narkawicz@nasa.gov), George Hagen (george.hagen@nasa.gov)
 * 
 * Copyright (c) 2015-2017 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 */
package gov.nasa.larcfm.ACCoRD;

import static gov.nasa.larcfm.ACCoRD.PolycarpQuadMinmax.quadratic;
import static gov.nasa.larcfm.ACCoRD.PolycarpQuadMinmax.quad_min_le_D_int;
import gov.nasa.larcfm.Util.Util;

public class PolycarpDoubleQuadratic {

	final static double AE = 0.00000001;

	public static boolean ae(double a, double b) {
		return (Math.abs(a-b)<AE);
	}

//	public static double sq(double a){
//		return a*a;
//	}

//	public static double discr(double a, double b, double c){
//		return b*b-4*a*c;
//	}

//	public static double root(double a, double b, double c, int eps) {
//		return (-b+eps*Math.sqrt(discr(a,b,c)))/(2*a);
//	}

	public static double quad(double a,double b,double c,double d,double ee,double f,double x,double y) {
		return a*Util.sq(x)+b*Util.sq(y)+c*x*y+d*x+ee*y+f;
	}

	public static double rev_disc(double a, double b,double c) {
		return Util.sq(c)-4*a*b;
	}

	public static double quad_min_y_to_x(double a,double c,double d,double y) {
		return -(c*y+d)/(2*a);
	}

	public static double quad_min_y(double a,double b,double c,double d,double ee) {
		if (ae(rev_disc(a,b,c),0)) {
			return 0;
		} else {
			return (2*a*ee-c*d)/rev_disc(a,b,c);
		}
	}

	public static double quad_min_x(double a,double b,double c,double d,double ee) {
		if (ae(rev_disc(a,b,c),0)) {
			return 0;
		} else {
			return (2*b*d-c*ee)/rev_disc(a,b,c);
		}
	}

	public static boolean quad_min_unit_box_init(double a,double b,double c,double d,double ee, double f,double D) {
		if (quad_min_le_D_int(a,c*1+d,quadratic(b,ee,f,1),0,1,D)) return true;
		if (quad_min_le_D_int(a,d,f,0,1,D)) return true;
		if (quad_min_le_D_int(b,c*1+ee,quadratic(a,d,f,1),0,1,D)) return true;
		if (quad_min_le_D_int(b,ee,f,0,1,D)) return true;
		double mx = quad_min_x(a,b,c,d,ee);
		double my = quad_min_y(a,b,c,d,ee);
		if ((0<=mx && mx<=1 && 0<=my && my<=1 && quad(a,b,c,d,ee,f,mx,my)<D)) return true;
		return false;
	}

	public static boolean quad_min_unit_box(double a,double b,double c,double d,double ee,double f,double D) {
		if (quad_min_le_D_int(a,c*1+d,quadratic(b,ee,f,1),0,1,D)) return true;
		if (quad_min_le_D_int(a,d,f,0,1,D)) return true;
		if (quad_min_le_D_int(b,c*1+ee,quadratic(a,d,f,1),0,1,D)) return true;
		if (quad_min_le_D_int(b,ee,f,0,1,D)) return true;
		double rev_disc_a_b_c = rev_disc(a,b,c);
		if (rev_disc_a_b_c>=0) return false;
		double mx = (2*b*d-c*ee);
		double my = (2*a*ee-c*d);
		if ((0<=mx*rev_disc_a_b_c &&
				mx*rev_disc_a_b_c<=rev_disc_a_b_c*rev_disc_a_b_c && 
				0<=my*rev_disc_a_b_c &&
				my*rev_disc_a_b_c<=rev_disc_a_b_c*rev_disc_a_b_c &&
				quad(a,b,c,d*rev_disc_a_b_c,ee*rev_disc_a_b_c,
						f*rev_disc_a_b_c*rev_disc_a_b_c,mx,my)<D*rev_disc_a_b_c*rev_disc_a_b_c)) return true;
		return false;
	}


}
