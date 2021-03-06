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


        double sosmatrix_60hz[6][6] = {
                                          {0.89087335,-1.78141916,0.89087335,1.,-1.99919001,0.99956485}, // 60hz 1
                                          {1.,-1.9996574,1.,1.,-1.99925215,0.99958806},                  // 60hz 2
                                          {0.89087335,-1.78045854,0.89087335,1.,-1.99811082,0.99957065}, // 120hz 1
                                          {1., -1.99860417,1.,1.,-1.99820027,0.99958225},                // 120hz 2
                                          {0.89087335,-1.77886533,0.89087335,1.,-1.99631785,0.99957259}, // 180hz 1
                                          {1.,-1.99684085,1.,1.,-1.99644231,0.99958032}                  // 180hz 2
                                      };

    public:
        SOS() {
            for (int stagenum = 0; stagenum < num_stages; stagenum++) {
                stages[stagenum] = Biquad();
                stages[stagenum].setcoeffs(sosmatrix_60hz[stagenum]);
            }
        }

        float filter(float in) {
            double result = in;

            for (int stagenum = 0; stagenum < num_stages; stagenum++) {
                result = stages[stagenum].filter(result);
            }

            return result;
        }
};

#endif


