/*
 * sos.cpp
 * Kaveh Pezeshki 1/18/2021
 * Implements a cascaded biquad filter for general purpose filtering
 */

#ifndef sos_h
#define sos_h

#include "biquad.cpp"

#define num_stages 6

class SOS
{
    private:
        Biquad stages[num_stages];


        // derived from python script for stopfreq = 60, 120, 180hz
        //
        //
        //fs = 20000
        //
        //width = 1
        //stopfreq = 60
        //
        //wp = [(stopfreq-2)*2/fs, (stopfreq+2)*2/fs]
        //ws = [(stopfreq-1)*2/fs, (stopfreq+1)*2/fs]
        //
        //gpass = 1
        //gstop = 10
        //
        //system = signal.iirdesign(wp, ws, gpass, gstop)
        //systemsos = signal.iirdesign(wp, ws, gpass, gstop, output='sos')


        float sosmatrix_60hz[12][6] = {
            {0.9994278369285194,-1.9985006758765964,0.9994278369285192,1.0,-1.9986924420198302,0.9990474740641058},
            {1.0,-1.9996366803664347,0.9999999999999999,1.0,-1.9995359883222978,0.9999025797492159},
            {1.0,-1.9996527358153406,1.0,1.0,-1.9995614789843854,0.999905610848302},
            {0.9994278397200603,-1.9974355317133963,0.9994278397200601,1.0,-1.997627194714772,0.9990474787111134},
            {1.0,-1.99856289526627,0.9999999999999999,1.0,-1.9984598127660078,0.9999033382739722},
            {1.0,-1.9985950019528287,0.9999999999999999,1.0,-1.9985062388190329,0.9999048532579878},
            {0.9994278447191671,-1.9956607126169374,0.9994278447191673,1.0,-1.9958522036417599,0.9990474870329971},
            {1.0,-1.9967790306412045,1.0000000000000004,1.0,-1.9966730890588942,0.9999035919182435},
            {1.0,-1.996827176448654,1.0000000000000002,1.0,-1.9967414445389258,0.9999046012898868},
            {0.9994278519240604,-1.9931768491495883,0.9994278519240604,1.0,-1.9933680994280267,0.9990474990267987},
            {1.0,-1.9942857202449076,1.0000000000000002,1.0,-1.9941768311615946,0.9999037196218035},
            {1.0,-1.9943498874477295,0.9999999999999998,1.0,-1.9942673440170853,0.9999044760024318}
        };

    public:
        SOS() {
            for (int stagenum = 0; stagenum < num_stages; stagenum++) {
                stages[stagenum] = Biquad();
                stages[stagenum].setcoeffs(sosmatrix_60hz[stagenum]);
            }
        }

        float filter(float in) {
            float result = in;

            for (int stagenum = 0; stagenum < num_stages; stagenum++) {
                result = stages[stagenum].filter(result);
            }

            return result;
        }
};

#endif


