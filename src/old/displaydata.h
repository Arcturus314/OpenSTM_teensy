struct displayData {
    /* Status codes
        - 1: 10V error
        - 2: 5V error
        - 3: 33V error
        - 4: crash detected
    */

    int current;
    int xpos;
    int ypos;
    int zpos;
    int zposStepper;
    int stepperInc;
    int status;
};
