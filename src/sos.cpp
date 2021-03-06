/*
 * sos.cpp
 * Kaveh Pezeshki 1/18/2021
 * Implements a cascaded biquad filter for general purpose filtering
 */

#ifndef sos_h
#define sos_h

#include "sosmat.h" // contains sosmat, the matrix of second order sections to filter with

#define num_stages 6

class SOS
{
    private:
        Biquad stages[num_stages];

    public:
        SOS() {
            for (int stagenum = 0; stagenum < num_stages; stagenum++) {
                stages[stagenum] = Biquad();
                stages[stagenum].setcoeffs(sosmat[stagenum]);
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


