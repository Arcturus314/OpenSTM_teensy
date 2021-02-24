// Band stop chebyshev filter order=2 alpha1=0.00295 alpha2=0.00305
// 60Hz band stop assuming 20kHz sample frequency
// Generated from http://www.schwietering.com/jayduino/filtuino/index.php?characteristic=ch&passmode=bs&order=2&chebrip=-3&usesr=usesr&sr=20000&frequencyLow=59&noteLow=&frequencyHigh=61&noteHigh=&pw=pw&calctype=float&run=Send

// 8th order butterworth SOS coeffs from Matlab (60dB suppression)
//
// >> d.Coefficients

ans =

    0.9999   -1.9994    0.9999    1.0000   -1.9994    0.9998
    0.9999   -1.9994    0.9999    1.0000   -1.9994    0.9998
    0.9997   -1.9991    0.9997    1.0000   -1.9991    0.9994
    0.9997   -1.9991    0.9997    1.0000   -1.9991    0.9994


// 4th order butterworth SOS coeffs from Matlab (30dB suppression)
//
//>> d.Coefficients

ans =

    0.9998   -1.9992    0.9998    1.0000   -1.9992    0.9996
    0.9998   -1.9992    0.9998    1.0000   -1.9992    0.9996

class TIAFilter
{
	public:
		TIAFilter()
		{
			for(int i=0; i <= 4; i++)
				v[i]=0.0;
		}
	private:
		float v[5];
	public:
		float step(float x) //class II 
		{
			v[0] = v[1];
			v[1] = v[2];
			v[2] = v[3];
			v[3] = v[4];
			v[4] = (9.997137612869119172e-1 * x)
				 + (-0.99942780182161106151 * v[0])
				 + (3.99757275982202875397 * v[1])
				 + (-5.99686203706488285547 * v[2])
				 + (3.99871695293598206078 * v[3]);
			return 
				 1.000000 * v[0]
				+v[4]
				- 3.999290 * v[1]
				- 3.999290 * v[3]
				+5.998579 * v[2];
		}
};
