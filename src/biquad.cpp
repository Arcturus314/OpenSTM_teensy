/*
 * biquad.cpp
 * Kaveh Pezeshki 1/18/2021
 * Implements a simple biquad filter for general purpose filtering
 */

#ifndef biquad_h
#define biquad_h

#include "Arduino.h"

class Biquad
{

	private:
		double a[3];
        double b[3];
        double x0;
        double x1;
        double y0;
        double y1;
	public:
        /*!
         * \brief sets filter coefficients
         */
		void setcoeffs(const double *coeffs) {

            a[0] = coeffs[0];
            a[1] = coeffs[1];
            a[2] = coeffs[2];
            b[0] = coeffs[3];
            b[1] = coeffs[4];
            b[2] = coeffs[5];

            x0 = 0;
            x1 = 0;
            y0 = 0;
            y1 = 0;
		}
        /*!
         * \brief adds an element to the filter and returns the filtered output
         * @param in the input value
         * @return the filtered value
         */
		float filter(float in)
		{
            double result = in*a[0] + x0*a[1] + x1*a[2] - y0*b[1] - y0*b[2];

            x1 = x0;
            x0 = in;
            y1 = y0;
            y0 = result;

			return (float) result;
		}
};

#endif
